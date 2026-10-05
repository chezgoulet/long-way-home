#!/usr/bin/env python3
"""Generate a post table for a deck from its map source.

A post is where a crew member stands and what they face, so this inverts the obvious approach: it
starts from the deck's **work objects** -- the named, interactive or modelled fixtures a person would
have a reason to stand at -- and looks for navigation furniture near each one. Picking waypoints first
and asking what is closest produces posts that face trigger volumes and stand inside furniture, which
is precisely the failure the crew work is judged against ("unrealistic and unnatural actions for the
characters in their environment").

Three rules, each of which exists because breaking it looks wrong in-game:

  * the fixture must be a *named* work object (`func_usable`, `func_static`, `misc_model` with a
    `targetname`), never a trigger, a speaker or an unnamed prop;
  * a waypoint must sit at standing distance -- 24 to 96 units, arm's length to a few paces -- so the
    crew member is at the object rather than inside it or across the room;
  * facing is derived from the object, never chosen, and the pairing is recorded by name so it can be
    checked by eye on the deck.

Nothing is invented. When a work object has no furniture at standing distance, the tool says so and
the post is left out rather than placed at a guess.

Usage: gen-posts.py <deck.map> [--max-posts N] [--json out.json]
"""
import argparse
import collections
import hashlib
import json
import math
import re

WORK_CLASSES = ("func_usable", "func_static", "misc_model")
WORK_PREFIXES = ("func_usable", "func_static", "misc_model")
STANDING_MIN, STANDING_MAX = 24.0, 96.0


def entities(text):
    """Each entity's key/values and a position: its origin, or the centroid of its brush planes."""
    for block in re.findall(r"\{(.*?)\}", text, re.S):
        kv = dict(re.findall(r'"([^"]+)"\s+"([^"]*)"', block))
        if not kv.get("classname"):
            continue
        pts = [(float(a), float(b), float(c)) for a, b, c in
               re.findall(r"\(\s*(-?[\d.]+)\s+(-?[\d.]+)\s+(-?[\d.]+)\s*\)", block)]
        pos = None
        if "origin" in kv:
            try:
                pos = tuple(float(v) for v in kv["origin"].split()[:3])
            except ValueError:
                pos = None
        if pos is None and pts:
            pos = tuple(sum(p[i] for p in pts) / len(pts) for i in range(3))
        yield kv, pos


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("map_file")
    ap.add_argument("--max-posts", type=int, default=12)
    ap.add_argument("--json")
    a = ap.parse_args()

    text = open(a.map_file, encoding="utf-8", errors="replace").read()
    digest = hashlib.sha256(text.encode("utf-8", "replace")).hexdigest()[:16]

    waypoints, work, unplaced = [], [], collections.Counter()
    for kv, pos in entities(text):
        cls = kv["classname"]
        if pos is None:
            unplaced[cls] += 1
            continue
        if cls.split("_")[0] == "waypoint":
            waypoints.append(pos)
        elif cls.startswith(WORK_PREFIXES) and kv.get("targetname"):
            work.append((kv["targetname"], cls, pos))

    posts, unsupported = [], []
    for name, cls, pos in sorted(work):
        near = [w for w in waypoints if STANDING_MIN <= math.dist(pos, w) <= STANDING_MAX]
        if not near:
            unsupported.append((name, cls))
            continue
        stand = min(near, key=lambda w: math.dist(pos, w))
        angle = round(math.degrees(math.atan2(pos[1] - stand[1], pos[0] - stand[0])) % 360.0, 1)
        posts.append({
            "origin": [round(v, 1) for v in stand],
            "angle": angle,
            "faces": name,
            "faces_class": cls,
            "distance_to_faces": round(math.dist(pos, stand), 1),
            "anchored": False,
        })

    # one post per work object, closest stand first, and never two crew in the same spot
    posts.sort(key=lambda p: p["distance_to_faces"])
    chosen = []
    for p in posts:
        if len(chosen) >= a.max_posts:
            break
        if all(math.dist(p["origin"], q["origin"]) >= 48.0 for q in chosen):
            chosen.append(p)
    for i, p in enumerate(chosen, 1):
        p["id"] = "%s.post%02d" % (a.map_file.split("/")[-1].split(".")[0], i)

    result = {
        "deck": a.map_file.split("/")[-1],
        "generated_from_sha256_16": digest,
        "waypoints": len(waypoints),
        "named_work_objects": len(work),
        "posts_supported": len(posts),
        "posts_chosen": len(chosen),
        "work_objects_without_standing_room": [{"name": n, "class": c} for n, c in unsupported],
        "posts": chosen,
    }
    if a.json:
        open(a.json, "w").write(json.dumps(result, indent=2) + "\n")
    print("waypoints=%d named work objects=%d -> posts supported=%d, chosen=%d"
          % (len(waypoints), len(work), len(posts), len(chosen)))
    for p in chosen:
        print("  %-18s stand=%-26s angle=%-6s faces %-14s (%.1fu)"
              % (p["id"], str(p["origin"]), p["angle"], p["faces"], p["distance_to_faces"]))
    if unsupported:
        print("  without standing room: %s" % ", ".join(n for n, _ in unsupported))


if __name__ == "__main__":
    main()
