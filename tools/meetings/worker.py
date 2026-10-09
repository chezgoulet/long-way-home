#!/usr/bin/env python3
"""The meeting worker: the async generator and the novelty classifier (M4).

The module does no I/O to a model. It writes a JSON-lines manifest of the meeting briefs the
simulation has queued (``ship/meetings/generate.jsonl``) and of the typed inputs to classify
(``ship/meetings/classify.jsonl``); this script drains them through ollama on the local socket and
writes back the two files the host reads:

  * the script, one line-oriented record per dialogue line (``D|`` header, ``L|`` lines), which the
    room plays in place of the authored skeleton while it is fresh; and
  * the classifier's verdicts and the call ledger (``CALLS|gen|novel`` then ``key|matched|outcome|note``).

Three rules are the point, and they are the design's, not this script's:

  * the model never writes ship state. This worker produces text and selects from the *enumerated*
    outcomes; the simulation applies them, as it applies a pill. Nothing here can change the ship.
  * a meeting plays from the authored skeleton when the model is absent, busy or broken. ``generate``
    fails soft: no script is written, and the module keeps the floor.
  * novelty is classification, not generation. Above the tuned threshold a branch plays and no
    generator is touched; below it exactly one live call is made, its answer is written down, and
    the exchange is promoted so a later run replays it with no call.

The async window is off the critical path, so ``generate`` may take seconds or minutes; nothing waits.

    worker.py generate --manifest F --script F --ledger F [--host URL] [--model M] [--max-attempts N]
    worker.py classify --manifest F --ledger F [--host URL] [--embed M] [--generator G]
                       [--threshold T] [--max-calls N] [--script F] [--transcript F]
    worker.py validate --script F [--json]

No model, no audio and no game data live in the repository; every output is player-local.
"""

import argparse
import json
import math
import os
import sys
import urllib.error
import urllib.request

SPEAKERS = ("room", "command", "engineering", "security", "sciences", "medical")
DELIVERIES = ("order", "report", "confession", "condolence", "flat")
# The delivery vocabulary must agree with module/ship/ship_core.cpp (DeliveryName). A drift is caught,
# not rendered silently.
DEFAULT_HOST = "http://127.0.0.1:11434"
DEFAULT_MODEL = "qwen2.5:3b"
DEFAULT_EMBED = "all-minilm"
DEFAULT_THRESHOLD = 0.55


# ---- the transport -------------------------------------------------------------------------------

def post_json(host, path, payload, timeout):
    data = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(host.rstrip("/") + path, data=data,
                                 headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=timeout) as fh:
        return json.load(fh)


def embed(host, model, text):
    r = post_json(host, "/api/embeddings", {"model": model, "prompt": text}, timeout=60)
    return r["embedding"]


def chat(host, model, prompt, timeout=600):
    r = post_json(host, "/api/chat", {
        "model": model, "stream": False, "format": "json",
        "options": {"temperature": 0.4, "num_predict": 700},
        "messages": [{"role": "user", "content": prompt}],
    }, timeout)
    return r.get("message", {}).get("content", "")


def cosine(a, b):
    s = sum(x * y for x, y in zip(a, b))
    na = math.sqrt(sum(x * x for x in a))
    nb = math.sqrt(sum(x * x for x in b))
    return s / (na * nb) if na and nb else 0.0


# ---- the ledger and verdicts the module reads ----------------------------------------------------

def read_ledger(path):
    calls = {"gen": 0, "novel": 0}
    verdicts = []
    if os.path.exists(path):
        with open(path, "r", encoding="utf-8") as fh:
            for line in fh:
                line = line.strip()
                if not line:
                    continue
                parts = line.split("|")
                if parts[0] == "CALLS" and len(parts) >= 3:
                    calls["gen"] = int(parts[1])
                    calls["novel"] = int(parts[2])
                elif not parts[0].startswith("#"):
                    verdicts.append(parts)
    return calls, verdicts


def write_ledger(path, calls, verdicts):
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, "w", encoding="utf-8") as fh:
        fh.write("CALLS|%d|%d\n" % (calls["gen"], calls["novel"]))
        for v in verdicts:
            fh.write("|".join(str(x) for x in v) + "\n")


def load_jsonl(path):
    out = []
    with open(path, "r", encoding="utf-8") as fh:
        for n, line in enumerate(fh, 1):
            line = line.strip()
            if not line:
                continue
            try:
                out.append(json.loads(line))
            except json.JSONDecodeError as exc:
                raise SystemExit("%s:%d: not JSON: %s" % (path, n, exc))
    return out


def sanitize(s):
    """The module reads fields split on ``|`` and lines on newline: neither may appear in a value."""
    return str(s).replace("|", "/").replace("\n", " ").replace("\r", " ").strip()


# ---- the validator -------------------------------------------------------------------------------
#
# The script is a branch tree. Every branch must terminate in one of the enumerated outcomes and none
# may be a dead end; no line may contradict the record. Each fault is named, so a refusal points at
# the branch that caused it rather than at "the script is bad".

def validate_script(script, option_count, forbidden=()):
    faults = []
    branches = script.get("branches")
    if not isinstance(branches, list) or not branches:
        return ["no branches: the script is empty"]

    def terminates(branch, seen):
        if branch.get("id") in seen:
            return None
        seen = seen | {branch.get("id")}
        if branch.get("outcome") is not None:
            o = branch.get("outcome")
            if isinstance(o, int) and 1 <= o <= option_count:
                return o
            return None
        goto = branch.get("goto")
        if not goto:
            return None
        nxt = next((b for b in branches if b.get("id") == goto), None)
        if nxt is None:
            return None
        return terminates(nxt, seen)

    for i, b in enumerate(branches):
        name = b.get("id") or ("branch %d" % i)
        lines = b.get("lines") or []
        if not lines and not b.get("goto"):
            faults.append("dead-end %s: no lines and no outcome or continuation" % name)
        term = terminates(b, set())
        if term is None:
            o = b.get("outcome")
            if o is not None and not (isinstance(o, int) and 1 <= o <= option_count):
                faults.append("branch %s does not terminate in an enumerated outcome "
                              "(outcome %s, options %d)" % (name, o, option_count))
            else:
                faults.append("dead-end %s: it reaches no enumerated outcome" % name)
        for ln in lines:
            txt = str(ln.get("text", ""))
            for bad in forbidden:
                if bad and bad.lower() in txt.lower():
                    faults.append("branch %s contradicts the record: it says \"%s\"" % (name, bad))
    return faults


# ---- generate: the brief becomes a script --------------------------------------------------------

def build_prompt(brief):
    options = brief.get("options", [])
    lines = [
        "You are writing the dialogue for a staff meeting aboard the starship Voyager, alone and far",
        "from home. Keep every line short and in character: an order is terse, a report is flat, a",
        "confession is low. Do not invent options and do not describe anything beyond the dialogue.",
        "",
        "DECISION: " + str(brief.get("decision", "")),
        "TRIGGER: " + str(brief.get("trigger", "")),
    ]
    present = ", ".join("%s (%s)" % (p.get("name", "?"), p.get("post", "?")) for p in brief.get("present", []))
    lines.append("PRESENT: " + (present or "the room"))
    lines.append("")
    lines.append("ENUMERATED OUTCOMES (write dialogue for each; invent nothing new):")
    for o in options:
        lines.append("  %d. %s  -- cost: %s" % (o.get("index", 0), o.get("label", ""), o.get("cost", "")))
    lines.append("")
    lines.append("AUTHORED SKELETON (imitate its brevity; you may improve the wording):")
    for oc in brief.get("skeleton", []):
        for ln in oc.get("lines", []):
            lines.append("  %d. %s: %s" % (oc.get("index", 0), ln.get("speaker", "room"), ln.get("text", "")))
    lines.append("")
    lines.append("Write EXACTLY %d branches, one for each outcome 1..%d. Do not stop after one, and do "
                 "not use placeholder text." % (len(options), len(options)))
    lines.append('Return ONLY JSON of this shape: {"branches":[{"id":"b1","intent":"how a player might '
                 'ask for outcome 1","outcome":1,"lines":[{"speaker":"command","text":"Aye. Carry on.",'
                 '"delivery":"order"}]}]}')
    lines.append("Every branch MUST have an \"outcome\" from 1..%d and a non-empty \"text\". speaker is "
                 "one of: %s. delivery is one of: %s." % (len(options), ", ".join(SPEAKERS), ", ".join(DELIVERIES)))
    return "\n".join(lines)


def cmd_generate(args):
    briefs = load_jsonl(args.manifest)
    calls, verdicts = read_ledger(args.ledger)
    script_lines = []
    generated = 0
    for brief in briefs:
        option_count = len(brief.get("options", []))
        branches = None
        attempts = 0
        for _ in range(max(1, args.max_attempts)):
            attempts += 1
            try:
                content = chat(args.host, args.model, build_prompt(brief))
                parsed = json.loads(content)
            except (urllib.error.URLError, OSError, ValueError, KeyError) as exc:
                print("model unavailable or invalid: %r" % (exc,))
                break
            faults = validate_script(parsed, option_count, brief.get("forbidden", []))
            if faults:
                print("rejected: " + "; ".join(faults))
                continue
            branches = parsed["branches"]
            generated += 1
            print("valid   : %s: %d branch(es) accepted" % (brief.get("kindName", brief.get("kind")), len(branches)))
            break
        calls["gen"] += attempts
        if branches is None:
            # The model was absent, or every attempt was rejected. Write no script for this brief: the
            # module keeps the authored skeleton as the floor, which is the design's guarantee.
            print("soft    : %s keeps the authored skeleton" % brief.get("kindName", brief.get("kind")))
            continue

        by_outcome = {}
        for b in (branches or []):
            o = b.get("outcome")
            if isinstance(o, int) and 1 <= o <= option_count and b.get("lines"):
                by_outcome[o] = b["lines"]
        script_lines.append("D|%d|%d|%s" % (brief["kind"], brief["stateDigest"],
                                            sanitize(brief.get("decision", ""))))
        for oc in brief.get("skeleton", []):
            o = oc.get("index", 0)
            src = by_outcome.get(o) or oc.get("lines") or []
            for ln in src:
                script_lines.append("L|%d|%d|%s|%s|%s" % (
                    brief["kind"], o, ln.get("speaker", "room"), ln.get("delivery", "report"),
                    sanitize(ln.get("text", ""))))
    with open(args.script, "w", encoding="utf-8") as fh:
        fh.write("\n".join(script_lines) + ("\n" if script_lines else ""))
    write_ledger(args.ledger, calls, verdicts)
    print("== %d script line(s) written; %d generation call(s)" % (len(script_lines), calls["gen"]))
    return 0


# ---- classify: the novel answer ------------------------------------------------------------------

def live_answer(args, req):
    """Exactly ONE generator call: choose an enumerated outcome and answer in character."""
    options = req.get("intents", [])
    lines = [
        "At a staff meeting aboard Voyager, the captain said: \"%s\"" % req.get("text", ""),
        "Do NOT repeat the captain's words. Choose exactly ONE of these enumerated outcomes and have a",
        "crew member answer it in character, in one short line:",
    ]
    for o in options:
        lines.append("  %d. %s" % (o.get("index", 0), o.get("intent", "")))
    lines.append('Return ONLY JSON: {"outcome":<n>,"speaker":"<role>","delivery":"<delivery>","text":"<the reply>"}')
    content = chat(args.host, args.generator, "\n".join(lines))
    parsed = json.loads(content)
    outcome = parsed.get("outcome")
    if not isinstance(outcome, int) or not (1 <= outcome <= len(options)):
        raise ValueError("the model chose no enumerated outcome: %r" % (outcome,))
    return parsed, outcome


def cmd_classify(args):
    reqs = load_jsonl(args.manifest)
    calls, verdicts = read_ledger(args.ledger)
    decided = {v[0]: v for v in verdicts}
    novel_calls = 0
    matched = 0
    for req in reqs:
        key = req["key"]
        if key in decided:
            print("cached  : %s -> branch %s (no call)" % (key, decided[key][2]))
            continue
        intents = req.get("intents", [])
        try:
            tv = embed(args.host, args.embed, req.get("text", ""))
            sims = []
            for it in intents:
                iv = embed(args.host, args.embed, it.get("intent", ""))
                sims.append((cosine(tv, iv), it.get("index")))
        except (urllib.error.URLError, OSError, ValueError, KeyError) as exc:
            print("classifier unavailable: %r; leaving \"%s\" novel" % (exc, req.get("text", "")))
            continue
        sims.sort(reverse=True)
        best = sims[0] if sims else (0.0, None)
        label = next((o.get("intent") for o in intents if o.get("index") == best[1]), "")
        if best[1] is not None and best[0] >= args.threshold:
            verdicts.append([key, 1, best[1], "matched %s at %.2f (threshold %.2f)" % (label, best[0], args.threshold)])
            decided[key] = verdicts[-1]
            matched += 1
            print("matched : \"%s\" -> branch %d (%.2f >= %.2f); no generator call" %
                  (req.get("text", ""), best[1], best[0], args.threshold))
        else:
            novel_calls += 1
            calls["novel"] += 1
            try:
                parsed, outcome = live_answer(args, req)
                answer = parsed.get("text", "")
                speaker = parsed.get("speaker", "room") if parsed.get("speaker") in SPEAKERS else "room"
                delivery = parsed.get("delivery", "report") if parsed.get("delivery") in DELIVERIES else "report"
            except (urllib.error.URLError, OSError, ValueError, KeyError, json.JSONDecodeError) as exc:
                print("live call failed for \"%s\": %r; the input stays novel" % (req.get("text", ""), exc))
                continue
            note = "novel: nearest %s at %.2f, below %.2f; one live call made; promoted to branch %d" % (
                label or "(none)", best[0], args.threshold, outcome)
            verdicts.append([key, 1, outcome, note])
            decided[key] = verdicts[-1]
            if args.script:
                with open(args.script, "a", encoding="utf-8") as fh:
                    fh.write("L|%d|%d|%s|%s|%s\n" % (req["kind"], outcome, speaker, delivery, sanitize(answer)))
            if args.transcript:
                os.makedirs(os.path.dirname(os.path.abspath(args.transcript)), exist_ok=True)
                with open(args.transcript, "a", encoding="utf-8") as fh:
                    fh.write("%s\t%d\t%s\t%s\n" % (req["kind"], outcome, speaker, sanitize(answer)))
            print("novel   : \"%s\" -> live call; answer %r; promoted to branch %d" %
                  (req.get("text", ""), answer, outcome))
    write_ledger(args.ledger, calls, verdicts)
    print("== %d matched, %d novel call(s), generation calls=%d" % (matched, novel_calls, calls["gen"]))
    if novel_calls > args.max_calls:
        print("FAIL  %d novel call(s) exceed the budget of %d" % (novel_calls, args.max_calls))
        return 1
    return 0


# ---- validate ------------------------------------------------------------------------------------

def cmd_validate(args):
    with open(args.script, "r", encoding="utf-8") as fh:
        script = json.load(fh)
    if args.json and "optionCount" not in script:
        print("refused: the script names no optionCount")
        return 1
    option_count = int(script.get("optionCount", len(script.get("options", []))))
    faults = validate_script(script, option_count, script.get("forbidden", []))
    for f in faults:
        print("REFUSED " + f)
    if faults:
        return 1
    print("PASS  every branch terminates in one of the %d enumerated outcomes; no dead ends" % option_count)
    return 0


# ---- main ----------------------------------------------------------------------------------------

def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    g = sub.add_parser("generate")
    g.add_argument("--manifest", required=True)
    g.add_argument("--script", required=True)
    g.add_argument("--ledger", required=True)
    g.add_argument("--host", default=DEFAULT_HOST)
    g.add_argument("--model", default=DEFAULT_MODEL)
    g.add_argument("--max-attempts", type=int, default=2)
    g.set_defaults(func=cmd_generate)

    c = sub.add_parser("classify")
    c.add_argument("--manifest", required=True)
    c.add_argument("--ledger", required=True)
    c.add_argument("--script", default="", help="append a promoted branch here")
    c.add_argument("--transcript", default="", help="write the promoted exchange here")
    c.add_argument("--host", default=DEFAULT_HOST)
    c.add_argument("--embed", default=DEFAULT_EMBED)
    c.add_argument("--generator", default=DEFAULT_MODEL)
    c.add_argument("--threshold", type=float, default=DEFAULT_THRESHOLD)
    c.add_argument("--max-calls", type=int, default=1, help="fail if more novel calls happen than this")
    c.set_defaults(func=cmd_classify)

    v = sub.add_parser("validate")
    v.add_argument("--script", required=True)
    v.add_argument("--json", action="store_true",
                   help="require the script to name its optionCount")
    v.set_defaults(func=cmd_validate)

    args = ap.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
