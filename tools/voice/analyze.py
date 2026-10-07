#!/usr/bin/env python3
"""The analyzer: find the retail voice assets and distil a clean reference clip.

Stage one of the voice spike found that the retail install already separates the
dialogue per character, so there is nothing to diarize and nothing to separate:
the analyzer's whole job is to locate a character's lines inside the game's paks,
pick the best few, strip their silence, and hand one reference wav to the
synthesizer.

    analyze.py census    --game <installdir>          # who has how many lines
    analyze.py reference --game <installdir> --character tuvok --out ref.wav

It reads the game the player owns and writes only into a path you name. It never
writes into the installation. See docs/asset-doctrine.md and the record in
docs/evidence/voice-spike.md.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile

VOICE_RE = re.compile(r"^sound/(voice|vox_d)/([^/]+)/.*\.mp3$", re.IGNORECASE)


def find_base_ef(game_dir):
    """The engine looks for a child directory spelled exactly `baseEF`; the GOG
    install ships `BaseEF`. Match on the name, case-insensitively."""
    if os.path.basename(os.path.normpath(game_dir)).lower() == "baseef":
        return game_dir
    for root, dirs, _ in os.walk(game_dir):
        for d in dirs:
            if d.lower() == "baseef":
                return os.path.join(root, d)
    raise SystemExit(f"no baseEF directory under {game_dir}")


def find_paks(base):
    paks = sorted(
        os.path.join(base, f)
        for f in os.listdir(base)
        if f.lower().startswith("pak") and f.lower().endswith(".pk3")
    )
    if not paks:
        raise SystemExit(f"no pak*.pk3 under {base}")
    return paks


def voice_members(paks):
    """Every dialogue mp3, grouped: {(tree, character): [(member, size)]}. The
    tree is `voice` (shipped) or `vox_d` (the downloadable voice pack)."""
    seen = {}
    for pak in paks:
        with zipfile.ZipFile(pak) as z:
            for info in z.infolist():
                m = VOICE_RE.match(info.filename)
                if m:
                    key = (m.group(1).lower(), m.group(2).lower())
                    seen.setdefault(key, {}).setdefault(info.filename, info.file_size)
    # Normalise to {key: [(member, size)]}, deduped: a member may live in more than one pak.
    return {k: sorted(v.items()) for k, v in seen.items()}


def duration(path):
    out = subprocess.run(
        ["ffprobe", "-v", "error", "-show_entries", "format=duration",
         "-of", "default=noprint_wrappers=1:nokey=1", path],
        capture_output=True, text=True,
    ).stdout.strip()
    try:
        return float(out)
    except ValueError:
        return 0.0


def cmd_census(args):
    base = find_base_ef(args.game)
    members = voice_members(find_paks(base))
    # One character may appear in both trees; report the shipped `voice` tree, and
    # the pack separately, because they are different encodings of the same lines.
    for tree in ("voice", "vox_d"):
        chars = {c: ms for (t, c), ms in members.items() if t == tree}
        if not chars:
            continue
        total = sum(len(ms) for ms in chars.values())
        print(f"{tree}/: {len(chars)} characters, {total} lines")
        for c in sorted(chars, key=lambda c: -len(chars[c])):
            print(f"  {c:<12} {len(chars[c]):>4} lines")


def pick_lines(durs, cap, target, lo, hi):
    """Prefer lines inside the clone window; take one or two until the reference
    is near the target length. A clone wants a handful of seconds, not a monologue."""
    cand = sorted(((d, p) for p, d in durs.items() if lo <= d <= hi), reverse=True)
    if not cand:
        cand = sorted(((d, p) for p, d in durs.items()), reverse=True)
    chosen, total = [], 0.0
    for d, p in cand:
        if len(chosen) >= cap:
            break
        if chosen and total + d > target * 1.3:
            break
        chosen.append(p)
        total += d
    return chosen


def cmd_reference(args):
    base = find_base_ef(args.game)
    paks = find_paks(base)
    members = voice_members(paks)
    character = args.character.lower()
    tree = "voice" if ("voice", character) in members else "vox_d"
    lines = members.get((tree, character))
    if not lines:
        raise SystemExit(f"no voice lines for character {args.character!r} in {base}")

    work = tempfile.mkdtemp(prefix="voice-ref-")
    try:
        # Extract just this character's lines.
        dests = {}
        for pak in paks:
            with zipfile.ZipFile(pak) as z:
                names = set(z.namelist())
                for member, _ in lines:
                    if member in names and member not in dests:
                        dest = os.path.join(work, os.path.basename(member))
                        with z.open(member) as src, open(dest, "wb") as out:
                            shutil.copyfileobj(src, out)
                        dests[member] = dest
        durs = {member: duration(dest) for member, dest in dests.items()}
        total = sum(durs.values())
        print(f"{args.character} ({tree}/): {len(durs)} lines, {total/60:.1f} min of speech")
        chosen = pick_lines(durs, args.lines, args.target_sec, args.min_sec, args.max_sec)
        if not chosen:
            raise SystemExit("no usable lines")
        for m in chosen:
            print(f"  {durs[m]:5.2f}s  {m}")

        inputs = []
        for i, m in enumerate(chosen):
            inputs += ["-i", dests[m]]
        filt = "".join(f"[{i}:a]" for i in range(len(chosen)))
        filt += (
            f"concat=n={len(chosen)}:v=0:a=1,"
            "highpass=f=60,"
            "silenceremove=start_periods=1:start_silence=0.05:start_threshold=-45dB:"
            "stop_periods=-1:stop_silence=0.25:stop_threshold=-45dB"
        )
        subprocess.run(
            ["ffmpeg", "-y", "-hide_banner", "-loglevel", "error", *inputs,
             "-filter_complex", filt, "-ar", "16000", "-ac", "1",
             "-c:a", "pcm_s16le", args.out],
            check=True,
        )
        print(f"reference: {args.out}  {duration(args.out):.2f}s  "
              f"from {len(chosen)} line(s)")
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    common = argparse.ArgumentParser(add_help=False)
    common.add_argument("--game", required=True, help="the retail install directory")
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("census", parents=[common], help="report the per-character line inventory")

    ref = sub.add_parser("reference", parents=[common],
                         help="build one character's reference wav")
    ref.add_argument("--character", required=True)
    ref.add_argument("--out", required=True)
    ref.add_argument("--lines", type=int, default=2, help="max lines in the reference")
    ref.add_argument("--target-sec", type=float, default=10.0,
                     help="rough reference length to aim for")
    ref.add_argument("--min-sec", type=float, default=4.0)
    ref.add_argument("--max-sec", type=float, default=9.5)

    args = ap.parse_args()
    if args.cmd == "census":
        cmd_census(args)
    else:
        cmd_reference(args)


if __name__ == "__main__":
    sys.exit(main())
