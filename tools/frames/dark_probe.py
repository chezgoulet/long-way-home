#!/usr/bin/env python3
"""The two-way dark probe: a named frame that is dark *and is meant to be* is checked, and so is a
named frame that has **stopped** being dark.

`tools/frames/frame.py floor` says whether a frame is legible; it cannot say whether a *dark* frame is
dark on purpose. `scripts/away-check.sh` has one such frame -- the stores post's outward view, whose
subject is the black debris field (`docs/evidence/the-away-site-exterior.md`) -- and it used to name it
in `ALLOWED_DARK` and fail only when a *different* frame was dark. That guard is one-way: it reports the
exemption working, and **can never report the exemption ceasing to be needed**. If a later pass seals the
opening and the frame becomes a lit wall, the check still passes and nothing says the outward view was
lost -- while, in the other direction, the frame that is dark because the sky is black would be reported
as a failure if the exemption were ever dropped.

This probe is the two-way guard:

  * the **named** frame **must read dark**; if it does not, the opening its subject stands for has been
    lost -- a lit surface where the sky was -- and the probe **fails**, naming it;
  * **no other** frame may read dark; a dark frame that is not the named one **fails**, naming it.

Both directions are demonstrated in `docs/evidence/the-two-way-fixes.md`: on the real frame set the named
frame is dark and the probe passes; on a set where that facing has become a wall the probe fails.

    tools/frames/dark_probe.py --named lwh_away_depot_stores_2.tga FRAME.tga [FRAME.tga ...]

Exit 0 when the named frame is dark and no other is; 1 otherwise; 2 if the measure itself could not run.
"""

import argparse
import subprocess
import sys
from pathlib import Path


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--band", default="0.10,0.55")
    ap.add_argument("--named", required=True,
                    help="the frame (basename) that must read dark -- the named opening")
    ap.add_argument("frames", nargs="+")
    a = ap.parse_args(argv)

    frame_py = Path(__file__).resolve().parent / "frame.py"
    run = subprocess.run([sys.executable, str(frame_py), "floor", "--band", a.band] + a.frames,
                         capture_output=True, text=True)
    if run.returncode != 0:
        sys.stderr.write(run.stderr or run.stdout)
        print("FAIL  the frame measure itself did not run", file=sys.stderr)
        return 2
    sys.stdout.write(run.stdout)

    dark = {Path(line.split()[0]).name for line in run.stdout.splitlines() if "DARK" in line.split()}
    named = Path(a.named).name
    ok = True

    if named in dark:
        print(f"PASS  the named frame reads dark as expected: {named} "
              f"(the opening it stands for is still open)")
    else:
        print(f"FAIL  the named frame {named} is NOT dark: the opening it stands for has been lost "
              f"(a lit surface where the sky was)")
        ok = False

    others = dark - {named}
    for o in sorted(others):
        print(f"FAIL  a frame reads dark and it is not the named one: {o}")
        ok = False
    if not others and named in dark:
        print(f"PASS  no frame other than the named one reads dark ({len(a.frames)} frame(s) measured)")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
