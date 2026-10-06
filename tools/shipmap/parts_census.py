#!/usr/bin/env python3
"""Check that every material a generated deck uses already exists in the game.

    parts_census.py --map build/ship/generated/deck12.map --game build/baseEF --census build/ship/deck12-census.json

The brief for deck 12 (docs/locations/deck12-environmental-control.brief.md, section 4) allows only
pieces the game already contains. A face naming a texture the game cannot resolve is the failure this
catches: it renders untextured or drops the surface. A material counts as present if it is an image
in a shipped pak, a shader a shipped `.shader` declares, one of our own shader aliases
(tools/shipmap/data), or a material in the reuse source (the census).

Exit 0 when every material resolves; 1 otherwise. Reads the game data read-only.
"""

import argparse
import glob
import json
import os
import re
import sys
import zipfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import stitch  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))


def used_textures(path):
    used = {}
    for keys, brushes in stitch.parse(path):
        for b in brushes:
            if not b.faces:
                continue
            for ln in b.lines:
                if ln.startswith("("):
                    m = stitch.FACE_TEXTURE.match(ln)
                    if m:
                        used[m.group(2)] = used.get(m.group(2), 0) + 1
                        break
    return used


def game_materials(game):
    images, shaders = set(), set()
    for pak in sorted(glob.glob(os.path.join(game, "pak*.pk3"))):
        with zipfile.ZipFile(pak) as z:
            for name in z.namelist():
                low = name.lower()
                if low.startswith("textures/") and low.rsplit(".", 1)[-1] in ("tga", "jpg", "png", "pcx"):
                    images.add(re.sub(r"\.(tga|jpg|png|pcx)$", "", re.sub(r"^textures/", "", low)))
                if low.startswith("scripts/") and low.endswith(".shader"):
                    text = z.read(name).decode("latin-1").lower()
                    for ln in text.splitlines():
                        ln = ln.strip()
                        if ln and not ln.startswith(("/", "{", "}", "\"")) and " " not in ln:
                            shaders.add(re.sub(r"^textures/", "", ln))
    return images, shaders


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--map", required=True)
    ap.add_argument("--game", required=True)
    ap.add_argument("--census", help="the reuse source's census (deck11-census.json)")
    a = ap.parse_args(argv)

    used = used_textures(a.map)
    images, shaders = game_materials(a.game)
    source = set()
    if a.census:
        source = {t.lower() for t in json.load(open(a.census)).get("textures", {})}
    ours = set()
    for p in glob.glob(os.path.join(HERE, "data", "*.shader")):
        for ln in open(p).read().lower().splitlines():
            s = ln.strip()
            if not s or s.startswith(("/", "{", "}")):
                continue
            tok = s.split()[0].rstrip("{").strip()
            if tok and tok not in ("{", "}"):
                ours.add(re.sub(r"^textures/", "", tok))

    missing = []
    for t in sorted(used):
        low = t.lower()
        if low not in images and low not in shaders and low not in ours and low not in source:
            missing.append(t)
    print("%s: %d distinct materials" % (a.map, len(used)))
    print("  present as shipped images:      %d" % len(images))
    print("  declared by shipped shaders:   %d" % len(shaders))
    print("  our own aliases:               %d" % len(ours))
    print("  in the reuse source census:    %d" % len(source))
    if missing:
        print("UNRESOLVED (%d): %s" % (len(missing), ", ".join(missing)))
        return 1
    print("PASS  every material already exists in the game; no new art")
    return 0


if __name__ == "__main__":
    sys.exit(main())
