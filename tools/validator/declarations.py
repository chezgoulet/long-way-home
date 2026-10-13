#!/usr/bin/env python3
"""The scenario declaration contract — the fields a scenario declares, and their laws.

`docs/scenario-manifest.md` is the human contract; this module is its machine form, and the single
place both the validator (`tools/validator/validate.py`) and the builder
(`tools/scenario/scenario_build.py`) read it from. What it owns:

  * the fourteen declaration fields and their types (`FIELDS`);
  * the closed vocabularies each one draws from, derived from the engine where the engine owns
    them (`systems`, shield arcs, departments, seats, deliveries) and written down here where the
    design is silent (breeding-condition states and operators, the effect kinds, warmth and
    visibility);
  * the two authoring laws, enforced mechanically (`check`):
      - **the post-not-name rule** — content addresses a post, a system id or a role, never a crew
        member (`docs/the-entry-point.md`, part three; `docs/scenario-atlas.md`); a manifest whose
        content names a person is refused (E010).
      - **the delivery direction per line** — a line declares how it is delivered, from the engine's
        own `Delivery` vocabulary, rather than that being assumed (E011).

Nothing here writes game state; it is the manifest half of the crossing, and the loader half is
`module/ship/scenario_core.cpp`.

Where the design does not fix a field's shape, the shape is ours and is recorded in
`docs/scenario-manifest.md` under "The declaration contract" and in the evidence for this pass
(`docs/evidence/the-manifest-contract.md`). A wrong number in a document is worse than a missing one,
so the vocabularies that the engine owns are read from the engine rather than copied.
"""

import os
import re

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))


def _read(rel):
    try:
        with open(os.path.join(ROOT, rel), encoding="utf-8") as f:
            return f.read()
    except OSError:
        return ""


# ---- vocabularies the engine owns, read from the engine -------------------------------------------------

def _systems():
    """The 23 system ids, by the human name `SPECS[]` gives them (`module/ship/ship_core.cpp`)."""
    text = _read("module/ship/ship_core.cpp")
    block = re.search(r"static const SystemSpec SPECS\[SYS_COUNT\]\s*=\s*\{(.*?)\n\};", text, re.S)
    names = re.findall(r'\{\s*"([^"]+)"', block.group(1)) if block else []
    return sorted(names)


def _arcs():
    """Canon's six shield arcs, lowercase, from `ShieldSectionName` (`module/ship/ship_core.cpp`)."""
    text = _read("module/ship/ship_core.cpp")
    m = re.search(r"const char \*ShieldSectionName\(ShieldSection[^)]*\)\s*\{\s*static const char \*const NAMES\[[^\]]*\]\s*=\s*\{([^}]*)\}", text, re.S)
    return sorted(re.findall(r'"([^"]+)"', m.group(1))) if m else []


def _seats():
    """The command seats and their canon record types, from the `SEATS[]` table (`ship_core.cpp`)."""
    text = _read("module/ship/ship_core.cpp")
    block = re.search(r"static const SeatSpec SEATS\[SEAT_COUNT\]\s*=\s*\{(.*?)\n\};", text, re.S)
    rows = re.findall(r'\{\s*"([^"]+)"\s*,\s*"([^"]+)"', block.group(1)) if block else []
    return rows  # [(seat name, canon record type)]


def _deliveries():
    """The engine's delivery vocabulary, minus `unmarked` (`DeliveryName`, `ship_core.cpp`)."""
    text = _read("module/ship/ship_core.cpp")
    m = re.search(r"const char \*DeliveryName\(uint8_t d\)\s*\{.*?=\s*\{([^}]*)\}", text, re.S)
    names = re.findall(r'"([^"]+)"', m.group(1)) if m else []
    return [n for n in names if n != "unmarked"]


SYSTEMS = _systems()
ARCS = _arcs()
SEAT_NAMES = [row[0] for row in _seats()]
CANON_NAMES = [row[1] for row in _seats()] + ["neelix", "kes"]
DELIVERIES = _deliveries()

# A vocabulary the engine owns that came back empty means the extractor (or the engine) changed, and
# every value would then read as out-of-vocabulary. Fail loudly rather than cry wolf.
for _name, _list in (("systems", SYSTEMS), ("shield arcs", ARCS), ("command seats", SEAT_NAMES),
                     ("delivery directions", DELIVERIES)):
    if not _list:
        raise RuntimeError(f"the declaration schema could not read the engine's {_name} "
                           f"(module/ship/ship_core.cpp); the extractor needs updating")

# The design's own: `docs/crew-manifest.md` names nine departments, and `DepartmentByName` matches on
# the leading word (`module/ship/ship_core.h:80-100`).
DEPARTMENTS = ["command", "operations", "engineering", "security", "science", "medical", "flight",
               "support", "civilians"]
STATIONS = ["engineering", "tactical", "operations", "conn", "sickbay"]

# ---- vocabularies the design leaves to us, decided once and written down --------------------------------

# Breeding conditions read the ship's own state (`docs/the-derivation.md`, "Breeding conditions"). Each
# entry names a readable state fact and the operator a condition applies to it.
BREEDING_STATES = {
    "deuterium": "number", "antimatter": "number", "batteries": "number", "torpedoes": "number",
    "spare_parts": "number", "materials": "number", "rations": "number", "probes": "number",
    "patients": "number", "refugees": "number", "resentment": "number", "borg_awareness": "number",
    "power_shortfall": "number", "coreless": "bool", "intruders": "number",
    "minutes_of_air": "number", "deck_life_support": "number", "system_condition": "number",
}
OPS = ["<", "<=", ">", ">=", "==", "!="]

# The effect kinds a cost or a residue writes, each an existing primitive from
# `docs/the-derivation.md` (cost to answer / to ignore / residue). `$post` is resolved by the
# simulation, never named here — that is the post-not-name law holding inside an effect too.
EFFECTS = {
    "write_off":        {"target": "str", "kind": "int?"},
    "damage_system":    {"system": "str", "amount": "number"},
    "shield_hit":       {"section": "str", "amount": "number"},
    "ignite":           {"deck": "int", "amount": "number"},
    "breach":           {"deck": "int", "amount": "number"},
    "kill":             {"post": "str", "cause": "str?"},
    "sicken":           {"post": "str", "condition": "str", "magnitude": "str?"},
    "promote":          {"post": "str"},
    "take_survivors":   {"count": "int"},
    "fabricate_parts":  {"count": "int"},
    "fabricate_rations": {"days": "int"},
    "air_source":       {"deck": "int", "source": "str"},
    "run_holodeck":     {"use": "str", "post": "str"},
}
CONDITIONS = ["wounded", "exhausted", "sick", "grieving", "frightened", "steadied"]
MAGNITUDES = ["slight", "clear", "sharp"]
HOLODECK_USES = ["recreation", "training", "therapy", "forensic"]

# Warmth is the human beat (docs/scenario-atlas.md: "a scenario with no warmth is a hazard, not a
# story"); the beats are the console's own warmth verbs (`holo`, `quarters`, `funeral`, `promote`,
# `rations`, `airponics`, `promise`, `manner`, `wall`).
WARMTH = ["holodeck", "quarters", "funeral", "promote", "rations", "airponics", "promise", "manner",
          "wall"]

# Authority a position holds (`docs/access-and-authority.md`; `MayOperate` / `MayCallAlert` /
# `MayCommand` / `DelegatedTo` / `OverrideActive` / `OodMayOrder`).
AUTHORITY = ["operate", "call_alert", "command", "delegate", "override", "ood"]

# What a position is told (`docs/scenario-atlas.md`: "the information they receive"): the log, the
# brief, a station's read, a state fact, or a deck.
INFORMATION = ["log", "brief", "console", "station"] + sorted(BREEDING_STATES)

# The declaration fields, in the order `docs/scenario-manifest.md` presents them.
FIELDS = ["canon_root", "breeding_conditions", "systems", "staff", "cost", "residue", "visibility",
          "warmth", "positions", "decision", "lines"]

# Keys that are not declaration fields but are part of the manifest's own frame.
FRAME_KEYS = ["name", "maps", "scripts", "inhabited", "asset_roots", "crew"]


class Finding:
    def __init__(self, code, where, detail):
        self.code, self.where, self.detail = code, where, detail


def _people(man):
    """The crew records the manifest may not name, and the canon command crew.

    Returns `(members, canon)`: the manifest's own member identifiers (matched case-insensitively —
    they are keys like `watch1`), and the canon command crew read from the engine's own seat table
    (matched as proper nouns, so the role word "the doctor" is not a name and is not refused).
    """
    members = set()
    crew = man.get("crew")
    if isinstance(crew, dict):
        for m in crew.get("members", []) or []:
            if isinstance(m, dict) and isinstance(m.get("name"), str):
                members.add(m["name"].lower())
    return members, list(CANON_NAMES)


def _names_member(text, members):
    """A member identifier, as a word — case-insensitively, because the identifiers are not prose."""
    if not isinstance(text, str):
        return None
    low = text.lower()
    for name in members:
        if re.search(r"(?<![a-z0-9_])" + re.escape(name) + r"(?![a-z0-9_])", low):
            return name
    return None


def _names_canon(text, canon):
    """A canon name, as a capitalised proper noun — `Tuvok`, not the role word `doctor`."""
    if not isinstance(text, str):
        return None
    for name in canon:
        proper = name[:1].upper() + name[1:]
        if re.search(r"(?<![A-Za-z0-9_])" + re.escape(proper) + r"(?![A-Za-z0-9_])", text):
            return name
    return None


def _names_person(text, members, canon):
    """A person named in free content: a member identifier or a canon proper noun."""
    return _names_member(text, members) or _names_canon(text, canon)


def _is_person(word, members, canon):
    """A word that is a person rather than a post — matched without case, since a post never is one."""
    if not isinstance(word, str):
        return None
    low = word.lower()
    for name in members:
        if low == name:
            return name
    for name in canon:
        if low == name.lower():
            return name
    return None


def _is_post(word, systems):
    """A post: a system id (the crew post vocabulary), a department, or a command seat."""
    w = str(word).lower()
    return w in systems or w in DEPARTMENTS or w in SEAT_NAMES


def _is_system_ref(word, systems):
    w = str(word).lower()
    if w in systems:
        return True
    base, _, arc = w.partition(".")
    return arc in ARCS and base in systems


def _short(val):
    s = repr(val)
    return s if len(s) <= 60 else s[:57] + "..."


def _check_effect(e, where, systems, out):
    if not isinstance(e, dict):
        out.append(Finding("E009", where, f"effect must be an object, got {_short(e)}"))
        return
    kind = e.get("effect")
    if kind not in EFFECTS:
        out.append(Finding("E009", where, f"unknown effect kind {_short(kind)}; known: {', '.join(sorted(EFFECTS))}"))
        return
    spec = EFFECTS[kind]
    for key, typ in spec.items():
        optional = typ.endswith("?")
        base = typ.rstrip("?")
        if key not in e:
            if not optional:
                out.append(Finding("E009", where, f"effect '{kind}' needs '{key}'"))
            continue
        val = e[key]
        if base == "number" and not isinstance(val, (int, float)) or isinstance(val, bool) and base == "number":
            out.append(Finding("E009", where, f"effect '{kind}'.{key} must be a number, got {_short(val)}"))
        elif base == "int" and (not isinstance(val, int) or isinstance(val, bool)):
            out.append(Finding("E009", where, f"effect '{kind}'.{key} must be an integer, got {_short(val)}"))
        elif base == "str" and not isinstance(val, str):
            out.append(Finding("E009", where, f"effect '{kind}'.{key} must be a string, got {_short(val)}"))
    if kind == "damage_system" and "system" in e and not _is_system_ref(e["system"], systems):
        out.append(Finding("E009", where, f"effect 'damage_system' names an unknown system {_short(e['system'])}"))
    if kind == "shield_hit" and e.get("section") not in ARCS:
        out.append(Finding("E009", where, f"effect 'shield_hit' names an unknown arc {_short(e.get('section'))}; known: {', '.join(ARCS)}"))
    if kind == "run_holodeck" and e.get("use") not in HOLODECK_USES:
        out.append(Finding("E009", where, f"effect 'run_holodeck' names an unknown use {_short(e.get('use'))}; known: {', '.join(HOLODECK_USES)}"))
    if kind == "sicken" and e.get("condition") not in CONDITIONS:
        out.append(Finding("E009", where, f"effect 'sicken' names an unknown condition {_short(e.get('condition'))}; known: {', '.join(CONDITIONS)}"))
    if kind in ("kill", "sicken", "promote", "run_holodeck"):
        p = e.get("post")
        if p is not None and not _is_post(p, systems):
            out.append(Finding("E009", where, f"effect '{kind}' names a post that is not a system, department or seat: {_short(p)}"))


def check(man, out):
    """Validate the declaration fields, appending Findings to `out`.

    `out` is a list the caller owns; the caller maps each Finding onto its report. The return value is
    the number of fields the manifest actually declares, so the caller can say what was checked.
    """
    systems = set(SYSTEMS)
    members, canon = _people(man)
    declared = 0

    for key in man:
        if key not in FIELDS and key not in FRAME_KEYS:
            out.append(Finding("E009", "scenario.json", f"unknown manifest key '{key}'"))

    def content(text, where):
        """Apply the post-not-name law to a piece of authored content."""
        who = _names_person(text, members, canon)
        if who:
            out.append(Finding("E010", where,
                               f"content names a crew member ('{who}'): content addresses a post, a "
                               f"system id or a role, and the simulation resolves it to a person "
                               f"(docs/the-entry-point.md, part three)"))

    # canon_root -------------------------------------------------------------------------------------
    if "canon_root" in man:
        declared += 1
        if not isinstance(man["canon_root"], str) or not man["canon_root"].strip():
            out.append(Finding("E009", "canon_root", "must be a non-empty string"))
        else:
            content(man["canon_root"], "canon_root")

    # breeding_conditions ----------------------------------------------------------------------------
    if "breeding_conditions" in man:
        declared += 1
        conds = man["breeding_conditions"]
        if not isinstance(conds, list) or not conds:
            out.append(Finding("E009", "breeding_conditions", "must be a non-empty list of conditions"))
        else:
            for i, c in enumerate(conds):
                where = f"breeding_conditions[{i}]"
                if not isinstance(c, dict):
                    out.append(Finding("E009", where, f"must be an object, got {_short(c)}"))
                    continue
                st, op = c.get("state"), c.get("op")
                if st not in BREEDING_STATES:
                    out.append(Finding("E009", where, f"unknown state {_short(st)}; known: {', '.join(sorted(BREEDING_STATES))}"))
                if op not in OPS:
                    out.append(Finding("E009", where, f"operator must be one of {', '.join(OPS)}, got {_short(op)}"))
                if "value" not in c:
                    out.append(Finding("E009", where, "needs a 'value'"))
                elif st in BREEDING_STATES and BREEDING_STATES[st] == "number" and not isinstance(c["value"], (int, float)):
                    out.append(Finding("E009", where, f"state '{st}' compares a number, got {_short(c['value'])}"))

    # systems ----------------------------------------------------------------------------------------
    if "systems" in man:
        declared += 1
        syss = man["systems"]
        if not isinstance(syss, list) or not syss:
            out.append(Finding("E009", "systems", "must be a non-empty list of system ids"))
        else:
            for i, entry in enumerate(syss):
                where = f"systems[{i}]"
                # A bare id, or an object the scenario also stresses or switches off. The design says
                # "by our system IDs"; the optional load is ours, recorded in the doc.
                stress, off = None, False
                sid = entry
                if isinstance(entry, dict):
                    for k in entry:
                        if k not in ("id", "stress", "off"):
                            out.append(Finding("E009", where, f"unknown key '{k}'; expected id, stress or off"))
                    sid = entry.get("id")
                    stress, off = entry.get("stress"), entry.get("off", False)
                    if stress is not None and not isinstance(stress, (int, float)):
                        out.append(Finding("E009", where, f"'stress' must be a number, got {_short(stress)}"))
                    if not isinstance(off, bool):
                        out.append(Finding("E009", where, f"'off' must be true or false, got {_short(off)}"))
                if not _is_system_ref(sid, systems):
                    out.append(Finding("E009", where,
                                       f"unknown system {_short(sid)}; a shipped system name, or "
                                       f"<system>.<arc> for one of {', '.join(ARCS)}"))

    # staff ------------------------------------------------------------------------------------------
    if "staff" in man:
        declared += 1
        st = man["staff"]
        if not isinstance(st, list) or not st:
            out.append(Finding("E009", "staff", "must be a non-empty list of posts"))
        else:
            for i, p in enumerate(st):
                if not _is_post(p, systems):
                    out.append(Finding("E009", f"staff[{i}]",
                                       f"{_short(p)} is not a post: a system id, a department "
                                       f"({', '.join(DEPARTMENTS)}) or a command seat"))

    # cost -------------------------------------------------------------------------------------------
    if "cost" in man:
        declared += 1
        cost = man["cost"]
        if not isinstance(cost, dict):
            out.append(Finding("E009", "cost", "must be an object with 'answer' and 'ignore'"))
        else:
            for key in cost:
                if key not in ("answer", "ignore"):
                    out.append(Finding("E009", "cost", f"unknown branch '{key}'; expected 'answer' or 'ignore'"))
            for branch in ("answer", "ignore"):
                if branch not in cost:
                    out.append(Finding("E009", "cost", f"missing branch '{branch}'"))
                    continue
                b = cost[branch]
                where = f"cost.{branch}"
                if not isinstance(b, dict):
                    out.append(Finding("E009", where, "must be an object with 'text' and 'effects'"))
                    continue
                for k in b:
                    if k not in ("text", "effects"):
                        out.append(Finding("E009", where, f"unknown key '{k}'; expected 'text' or 'effects'"))
                content(b.get("text", ""), where + ".text")
                effs = b.get("effects", [])
                if not isinstance(effs, list):
                    out.append(Finding("E009", where + ".effects", "must be a list"))
                else:
                    for j, e in enumerate(effs):
                        _check_effect(e, f"{where}.effects[{j}]", systems, out)

    # residue ----------------------------------------------------------------------------------------
    if "residue" in man:
        declared += 1
        res = man["residue"]
        if not isinstance(res, list) or not res:
            out.append(Finding("E009", "residue", "must be a non-empty list of effects"))
        else:
            for j, e in enumerate(res):
                where = f"residue[{j}]"
                # residue may be an object effect or a plain string naming the lasting thing.
                if isinstance(e, str):
                    content(e, where)
                else:
                    _check_effect(e, where, systems, out)

    # visibility -------------------------------------------------------------------------------------
    if "visibility" in man:
        declared += 1
        vis = man["visibility"]
        if not isinstance(vis, list) or not vis:
            out.append(Finding("E009", "visibility", "must be a non-empty list"))
        else:
            for i, v in enumerate(vis):
                vv = str(v).lower()
                ok = vv in STATIONS or _is_system_ref(vv, systems) or re.fullmatch(r"deck:\d+", vv)
                if not ok:
                    out.append(Finding("E009", f"visibility[{i}]",
                                       f"{_short(v)} is not a surface: a station ({', '.join(STATIONS)}), "
                                       f"a system id, or deck:<n>"))

    # warmth -----------------------------------------------------------------------------------------
    if "warmth" in man:
        declared += 1
        wm = man["warmth"]
        if not isinstance(wm, list) or not wm:
            out.append(Finding("E009", "warmth", "must be a non-empty list of beats"))
        else:
            for i, w in enumerate(wm):
                if w not in WARMTH:
                    out.append(Finding("E009", f"warmth[{i}]", f"unknown beat {_short(w)}; known: {', '.join(WARMTH)}"))

    # positions --------------------------------------------------------------------------------------
    if "positions" in man:
        declared += 1
        poss = man["positions"]
        if not isinstance(poss, list) or not poss:
            out.append(Finding("E009", "positions", "must be a non-empty list"))
        else:
            for i, p in enumerate(poss):
                where = f"positions[{i}]"
                if not isinstance(p, dict):
                    out.append(Finding("E009", where, f"must be an object, got {_short(p)}"))
                    continue
                for k in p:
                    if k not in ("post", "information", "authority", "responsible_for", "decision"):
                        out.append(Finding("E009", where, f"unknown key '{k}'"))
                post = p.get("post")
                if post is None:
                    out.append(Finding("E009", where, "needs a 'post'"))
                elif not _is_post(post, systems):
                    who = _is_person(post, members, canon)
                    if who:
                        out.append(Finding("E010", where, f"position addresses a person ('{who}'); a position is a post"))
                    else:
                        out.append(Finding("E009", where, f"'{_short(post)}' is not a post"))
                for info in p.get("information", []) or []:
                    iv = str(info).lower()
                    if iv not in INFORMATION and iv not in STATIONS:
                        out.append(Finding("E009", where + ".information", f"unknown information {_short(info)}"))
                for a in p.get("authority", []) or []:
                    if a not in AUTHORITY:
                        out.append(Finding("E009", where + ".authority", f"unknown authority {_short(a)}; known: {', '.join(AUTHORITY)}"))
                for r in p.get("responsible_for", []) or []:
                    if not _is_post(r, systems):
                        out.append(Finding("E009", where + ".responsible_for", f"{_short(r)} is not a post or department"))
                content(p.get("decision", ""), where + ".decision")

    # decision ---------------------------------------------------------------------------------------
    if "decision" in man:
        declared += 1
        dec = man["decision"]
        if not isinstance(dec, dict):
            out.append(Finding("E009", "decision", "must be an object with 'question' and 'options'"))
        else:
            for k in dec:
                if k not in ("question", "options"):
                    out.append(Finding("E009", "decision", f"unknown key '{k}'"))
            content(dec.get("question", ""), "decision.question")
            opts = dec.get("options")
            if not isinstance(opts, list) or not opts:
                out.append(Finding("E009", "decision.options", "must be a non-empty list"))
            else:
                for j, o in enumerate(opts):
                    where = f"decision.options[{j}]"
                    if not isinstance(o, dict) or not isinstance(o.get("label"), str):
                        out.append(Finding("E009", where, "must be an object with a 'label'"))
                        continue
                    content(o["label"], where + ".label")
                    for k in o:
                        if k not in ("label", "cost"):
                            out.append(Finding("E009", where, f"unknown key '{k}'"))
                    for m, e in enumerate(o.get("cost", []) or []):
                        _check_effect(e, f"{where}.cost[{m}]", systems, out)

    # lines — the post-not-name law and the delivery direction, per line -------------------------------
    if "lines" in man:
        declared += 1
        lines = man["lines"]
        if not isinstance(lines, list) or not lines:
            out.append(Finding("E009", "lines", "must be a non-empty list of lines"))
        else:
            for i, ln in enumerate(lines):
                where = f"lines[{i}]"
                if not isinstance(ln, dict):
                    out.append(Finding("E009", where, f"must be an object, got {_short(ln)}"))
                    continue
                for k in ln:
                    if k not in ("to", "delivery", "text"):
                        out.append(Finding("E009", where, f"unknown key '{k}'"))
                to = ln.get("to")
                if to is None:
                    out.append(Finding("E009", where, "needs a 'to' (the post the line addresses)"))
                elif not _is_post(to, systems):
                    who = _is_person(to, members, canon)
                    if who:
                        out.append(Finding("E010", where, f"line addresses a person ('{who}'); a line addresses a post"))
                    else:
                        out.append(Finding("E009", where, f"'{_short(to)}' is not a post: a system id, a department or a seat"))
                if "delivery" not in ln:
                    out.append(Finding("E011", where, f"line declares no delivery direction; known: {', '.join(DELIVERIES)}"))
                elif ln["delivery"] not in DELIVERIES:
                    out.append(Finding("E011", where, f"unknown delivery {_short(ln['delivery'])}; known: {', '.join(DELIVERIES)}"))
                if not isinstance(ln.get("text"), str) or not ln.get("text", "").strip():
                    out.append(Finding("E009", where, "needs non-empty 'text'"))
                else:
                    content(ln["text"], where + ".text")

    return declared


def field_keys():
    """The declaration field names, for the builder and the coverage check."""
    return list(FIELDS)
