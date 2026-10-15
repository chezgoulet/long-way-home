#!/usr/bin/env python3
"""Survey candidate source maps for a site archetype: brushes, entities, lights and bounds.

The stopped Item-2 pass measured four candidate sources for the **derelict interior** archetype
(`docs/outside-the-ship.md`: *re-dress `dn*`, `borg*`, `scav*`*) and cited the counts to a tool that was
not named precisely enough to re-run (`docs/evidence/the-archetype-survey.md` records the correction).
This is that instrument, landed so the next pass re-runs it rather than re-deriving it.

**It counts brushes, not faces.** A face-count proxy is easy to get and drifts: a brush with six faces is
still one brush, and a patch carries a different count again. The count here is the number of **brushes**
(the map's own unit) in every entity, which is what "how dense is this source" means.

    tools/away/survey_sources.py build/gdk/maps/**/scav2.map [MAP.map ...]

Prints one row per map: name, brushes, entities, lights, bounds, footprint and height (units and KiB).
"""

import argparse
import glob
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "shipmap"))
import stitch  # noqa: E402


def survey(path):
    lo = [1e9] * 3
    hi = [-1e9] * 3
    brushes = 0
    patches = 0
    entities = 0
    lights = 0
    for keys, brs in stitch.parse(str(path)):
        entities += 1
        if keys.get("classname") == "light":
            lights += 1
        for b in brs:
            if b.is_patch:
                patches += 1
                continue
            brushes += 1
            for i in range(3):
                lo[i] = min(lo[i], b.lo[i])
                hi[i] = max(hi[i], b.hi[i])
    return {"map": path.stem, "path": str(path), "brushes": brushes, "patches": patches,
            "entities": entities, "lights": lights, "lo": lo, "hi": hi}


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("maps", nargs="+", help="source .map files (globs allowed)")
    a = ap.parse_args(argv)

    paths = []
    for m in a.maps:
        paths.extend(sorted(glob.glob(m)) or ([m] if Path(m).is_file() else []))
    if not paths:
        print("no source maps matched", file=sys.stderr)
        return 1

    print("map            brushes patches entities lights   size (x y z, units)              footprint  height")
    for p in paths:
        s = survey(Path(p))
        sx, sy, sz = (s["hi"][i] - s["lo"][i] for i in range(3))
        print("%-14s %7d %7d %8d %6d  x %6.0f..%-6.0f y %6.0f..%-6.0f z %6.0f..%-6.0f  %4.1fx%-4.1f %4.1f"
              % (s["map"], s["brushes"], s["patches"], s["entities"], s["lights"],
                 s["lo"][0], s["hi"][0], s["lo"][1], s["hi"][1], s["lo"][2], s["hi"][2],
                 sx / 1024.0, sy / 1024.0, sz / 1024.0))
    return 0


if __name__ == "__main__":
    sys.exit(main())
