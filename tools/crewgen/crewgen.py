#!/usr/bin/env python3
"""Crew authoring: the scenario manifest's `crew` section -> the `maps/<map>.crew` file the
game module reads.

    crewgen.py check   SCENARIO_DIR [--data DIR]
    crewgen.py build   SCENARIO_DIR --out DIR [--data DIR]
    crewgen.py suggest --map tour/deck04 [--data DIR] [--scenario DIR] [--crew 6]

`check` validates the section and exits non-zero on any error. With --data it also checks the
section against the map itself, read from the owner's own installation: that navgoal posts name
navgoals the map has, that crew types exist in the game's NPC table, and that authored positions
sit on the map's navigation.

`build` runs the same checks and writes <out>/maps/<map>.crew.

`suggest` proposes a starting section from the map's own waypoints -- positions the navigator is
known to reach -- so a scenario starts from something that walks. It prints JSON; nothing is
written, and the result is a starting point to edit, not a design.

No game data enters this repository: the map is read where it is installed.
"""

import argparse
import glob
import json
import math
import os
import re
import struct
import sys
import zipfile

G3_MIN_CREW, G3_MAX_CREW = 5, 10
MAX_CREW, MAX_POSTS = 32, 256
NAME = re.compile(r"^[A-Za-z0-9_\-]+$")
NAV_REACH = 96.0  # an authored position further than this from every waypoint is off the graph
BOUNDS = {
    "reach_ms": "bound reach_ms",
    "ack_ms": "bound ack_ms",
    "stuck_ms": "bound stuck_ms",
    "stuck_strikes": "bound stuck_strikes",
    "sample_ms": "bound sample_ms",
    "coverage_percent": "bound coverage_percent",
    "save_bytes_per_npc": "budget save_bytes_per_npc",
}
# Named crew the game defines who are not placed on the Virtual Voyager decks' own rosters.
SUGGEST_TYPES = ["Renner", "Showers", "Green", "Lang", "Inya", "Kray", "Salma", "Pasty"]


class GameData:
    """Read-only view of the installation's paks. Later paks override earlier ones."""

    def __init__(self, data_dir):
        base = None
        for cand in [data_dir] + sorted(glob.glob(os.path.join(data_dir, "*"))):
            if os.path.isdir(cand) and glob.glob(os.path.join(cand, "*.pk3")):
                base = cand
                break
        if base is None:
            raise FileNotFoundError(f"no .pk3 files in {data_dir} or its subdirectories")
        self.paks = sorted(glob.glob(os.path.join(base, "*.pk3")), reverse=True)

    def read(self, name):
        want = name.lower()
        for pak in self.paks:
            with zipfile.ZipFile(pak) as z:
                for member in z.namelist():
                    if member.lower() == want:
                        return z.read(member)
        return None

    def entities(self, mapname):
        bsp = self.read(f"maps/{mapname}.bsp")
        if bsp is None:
            return None
        if bsp[:4] != b"IBSP":
            raise ValueError(f"maps/{mapname}.bsp is not an IBSP file")
        ofs, length = struct.unpack_from("<ii", bsp, 8)
        return parse_entities(bsp[ofs:ofs + length].decode("latin-1").rstrip("\0"))

    def npc_types(self):
        cfg = self.read("ext_data/NPCs.cfg")
        if cfg is None:
            return None
        return {m.group(1).lower() for m in re.finditer(r"^\s*([A-Za-z0-9_]+)\s*\{", cfg.decode("latin-1"), re.M)}


def parse_entities(text):
    """Entity key/value blocks from a BSP entity lump or a .map source (brush blocks are skipped)."""
    ents, depth, current = [], 0, None
    for line in text.splitlines():
        line = line.strip()
        if line == "{":
            depth += 1
            if depth == 1:
                current = {}
        elif line == "}":
            if depth == 1 and current is not None:
                ents.append(current)
                current = None
            depth = max(depth - 1, 0)
        elif depth == 1 and current is not None:
            m = re.match(r'"([^"]*)"\s+"([^"]*)"', line)
            if m:
                current[m.group(1)] = m.group(2)
    return ents


class ScenarioData:
    """The installation, with the scenario's own map sources in front of it: a scenario that
    brings maps/<name>.map is checked against that source, not against anything installed."""

    def __init__(self, scenario_dir, data):
        self.scenario_dir, self.data = scenario_dir, data

    def entities(self, mapname):
        local = os.path.join(self.scenario_dir, "maps", mapname + ".map")
        if os.path.isfile(local):
            with open(local, encoding="utf-8", errors="replace") as f:
                return parse_entities(f.read())
        return self.data.entities(mapname) if self.data else None

    def npc_types(self):
        return self.data.npc_types() if self.data else None

    def has_local(self, mapname):
        return os.path.isfile(os.path.join(self.scenario_dir, "maps", mapname + ".map"))


def origin_of(ent):
    try:
        x, y, z = (float(v) for v in ent.get("origin", "0 0 0").split())
        return (x, y, z)
    except ValueError:
        return None


def dist(a, b):
    return math.sqrt(sum((p - q) ** 2 for p, q in zip(a, b)))


def is_vec3(v):
    return (isinstance(v, list) and len(v) == 3
            and all(isinstance(c, (int, float)) and not isinstance(c, bool) and math.isfinite(c) for c in v))


def is_num(v):
    return isinstance(v, (int, float)) and not isinstance(v, bool) and math.isfinite(v)


def validate(crew, data=None):
    """Returns (errors, warnings) for a manifest `crew` section."""
    errors, warnings = [], []
    err, warn = errors.append, warnings.append

    if not isinstance(crew, dict):
        return ["crew: must be an object"], []
    known = {"map", "max", "bounds", "posts", "members"}
    for key in crew:
        if key not in known:
            err(f"crew: unknown key '{key}'")

    mapname = crew.get("map")
    if not isinstance(mapname, str) or not mapname or mapname.endswith(".bsp") or mapname.startswith("maps/"):
        err("crew.map: required; the map as the game names it, e.g. 'tour/deck04' (no 'maps/', no '.bsp')")
        mapname = None

    members = crew.get("members", [])
    posts = crew.get("posts", [])
    if not isinstance(members, list):
        err("crew.members: must be a list")
        members = []
    if not isinstance(posts, list):
        err("crew.posts: must be a list")
        posts = []

    cap = crew.get("max", G3_MAX_CREW)
    if not isinstance(cap, int) or isinstance(cap, bool) or not 1 <= cap <= MAX_CREW:
        err(f"crew.max: must be an integer 1..{MAX_CREW}")
        cap = MAX_CREW
    if len(members) > cap:
        err(f"crew.members: {len(members)} declared but crew.max is {cap}")
    if len(posts) > MAX_POSTS:
        err(f"crew.posts: at most {MAX_POSTS}")

    bounds = crew.get("bounds", {})
    if not isinstance(bounds, dict):
        err("crew.bounds: must be an object")
        bounds = {}
    for key, value in bounds.items():
        if key not in BOUNDS:
            err(f"crew.bounds: unknown key '{key}'")
        elif not isinstance(value, int) or isinstance(value, bool) or value < 0:
            err(f"crew.bounds.{key}: must be a non-negative integer")

    names = set()
    for i, m in enumerate(members):
        where = f"crew.members[{i}]"
        if not isinstance(m, dict):
            err(f"{where}: must be an object")
            continue
        for key in m:
            if key not in {"name", "type", "at", "yaw"}:
                err(f"{where}: unknown key '{key}'")
        name = m.get("name")
        if not isinstance(name, str) or not NAME.match(name):
            err(f"{where}.name: required; letters, digits, '_' and '-' only")
        elif name.lower() in names:
            err(f"{where}.name: duplicate '{name}'")
        else:
            names.add(name.lower())
        has_type, has_at = "type" in m, "at" in m
        if has_type != has_at:
            err(f"{where}: give both 'type' and 'at' to spawn a member, or neither to name an NPC the map places")
        if has_type and (not isinstance(m["type"], str) or not NAME.match(m["type"])):
            err(f"{where}.type: must be an NPC type name")
        if has_at and not is_vec3(m["at"]):
            err(f"{where}.at: must be [x, y, z]")
        if "yaw" in m and not is_num(m["yaw"]):
            err(f"{where}.yaw: must be a number")

    post_names = set()
    for i, p in enumerate(posts):
        where = f"crew.posts[{i}]"
        if not isinstance(p, dict):
            err(f"{where}: must be an object")
            continue
        for key in p:
            if key not in {"name", "at", "navgoal", "yaw", "radius", "holder", "priority"}:
                err(f"{where}: unknown key '{key}'")
        name = p.get("name")
        if not isinstance(name, str) or not NAME.match(name):
            err(f"{where}.name: required; letters, digits, '_' and '-' only")
        elif name.lower() in post_names:
            err(f"{where}.name: duplicate '{name}'")
        else:
            post_names.add(name.lower())
        if ("at" in p) == ("navgoal" in p):
            err(f"{where}: give exactly one of 'at' (a position) or 'navgoal' (a targetname in the map)")
        if "at" in p and not is_vec3(p["at"]):
            err(f"{where}.at: must be [x, y, z]")
        if "navgoal" in p and (not isinstance(p["navgoal"], str) or not NAME.match(p["navgoal"])):
            err(f"{where}.navgoal: must be a targetname")
        if "yaw" in p and not is_num(p["yaw"]):
            err(f"{where}.yaw: must be a number")
        if "radius" in p and (not isinstance(p["radius"], int) or isinstance(p["radius"], bool) or p["radius"] <= 0):
            err(f"{where}.radius: must be a positive integer")
        if "priority" in p and (not isinstance(p["priority"], int) or isinstance(p["priority"], bool)):
            err(f"{where}.priority: must be an integer")
        holder = p.get("holder")
        if holder is not None:
            if not isinstance(holder, str) or not NAME.match(holder):
                err(f"{where}.holder: must be a member name or NPC type")
            elif members and holder.lower() not in names and not any(
                    isinstance(m, dict) and str(m.get("type", "")).lower() == holder.lower() for m in members):
                err(f"{where}.holder: '{holder}' is not a declared member or a declared member's type")

    if members and not G3_MIN_CREW <= len(members) <= G3_MAX_CREW:
        warn(f"crew.members: {len(members)} declared; the G3 bar is {G3_MIN_CREW}..{G3_MAX_CREW}")
    if members and posts and len(posts) < len(members):
        warn(f"crew: {len(members)} members but only {len(posts)} posts; the surplus will be spares")

    if data is not None and mapname and not errors:
        check_against_map(crew, mapname, data, err, warn)
    return errors, warnings


def check_against_map(crew, mapname, data, err, warn):
    ents = data.entities(mapname)
    if ents is None:
        err(f"crew.map: maps/{mapname} is neither a map source in the scenario nor a map in the game data")
        return
    navgoals = {e.get("targetname", "").lower() for e in ents if e.get("classname", "").startswith("waypoint_navgoal")}
    placed = {(e.get("NPC_targetname") or e.get("npc_targetname") or "").lower()
              for e in ents if e.get("classname", "").startswith("NPC_")}
    waypoints = [o for o in (origin_of(e) for e in ents if e.get("classname") in ("waypoint", "waypoint_small")) if o]
    types = data.npc_types()

    def on_graph(pos):
        return not waypoints or min(dist(pos, w) for w in waypoints) <= NAV_REACH

    if not waypoints:
        warn(f"crew.map: maps/{mapname} has no waypoints; nothing placed on it can navigate")
    for i, p in enumerate(crew.get("posts", [])):
        if "navgoal" in p and p["navgoal"].lower() not in navgoals:
            err(f"crew.posts[{i}].navgoal: the map has no waypoint_navgoal named '{p['navgoal']}'")
        if "at" in p and not on_graph(p["at"]):
            err(f"crew.posts[{i}].at: more than {NAV_REACH:.0f} units from any waypoint; nothing can walk there")
    for i, m in enumerate(crew.get("members", [])):
        if "type" in m:
            if types is not None and m["type"].lower() not in types:
                err(f"crew.members[{i}].type: '{m['type']}' is not in the game's NPC table")
            if not on_graph(m["at"]):
                err(f"crew.members[{i}].at: more than {NAV_REACH:.0f} units from any waypoint")
            if m["name"].lower() in placed:
                err(f"crew.members[{i}].name: the map already places an NPC named '{m['name']}'")
        elif m["name"].lower() not in placed:
            err(f"crew.members[{i}].name: the map places no NPC named '{m['name']}' (add 'type' and 'at' to spawn one)")


def num(v):
    return f"{v:g}"


def render(crew, source):
    """The .crew text for a validated section."""
    lines = [f"# Generated by tools/crewgen from {source}. Edit the manifest, not this file."]
    if "max" in crew:
        lines.append(f"crew max {crew['max']}")
    elif crew.get("members"):
        lines.append(f"crew max {max(len(crew['members']), 1)}")
    for key, value in crew.get("bounds", {}).items():
        lines.append(f"{BOUNDS[key]} {value}")
    for m in crew.get("members", []):
        line = f"member {m['name']}"
        if "type" in m:
            line += f" type {m['type']} at {' '.join(num(c) for c in m['at'])}"
            if "yaw" in m:
                line += f" yaw {num(m['yaw'])}"
        lines.append(line)
    for p in crew.get("posts", []):
        line = f"post {p['name']}"
        if "at" in p:
            line += f" at {' '.join(num(c) for c in p['at'])}"
        else:
            line += f" navgoal {p['navgoal']}"
        for key in ("yaw", "radius", "priority"):
            if key in p:
                line += f" {key} {num(p[key])}"
        if "holder" in p:
            line += f" holder {p['holder']}"
        lines.append(line)
    return "\n".join(lines) + "\n"


def suggest(data, mapname, count):
    ents = data.entities(mapname)
    if ents is None:
        raise SystemExit(f"maps/{mapname} is neither a map source in the scenario nor a map in the game data")
    waypoints = [o for o in (origin_of(e) for e in ents if e.get("classname") == "waypoint") if o]
    # Keep clear of everything the map already puts somewhere: NPCs, spawners, the player, and
    # the navgoals its own scripts walk characters to.
    busy = [o for o in (origin_of(e) for e in ents
                        if e.get("classname", "").startswith(("NPC_", "waypoint_navgoal", "info_player"))) if o]
    free = [w for w in waypoints if all(dist(w, b) > 96 for b in busy)]
    if len(free) < 2 * count:
        raise SystemExit(f"only {len(free)} free waypoints on {mapname}; cannot place {count} crew and {count} posts")

    def spread(pool, n, seeds):
        chosen = []
        pool = list(pool)
        while len(chosen) < n and pool:
            best = max(pool, key=lambda w: (min([dist(w, c) for c in chosen + seeds] or [0.0]), w))
            chosen.append(best)
            pool.remove(best)
        return chosen

    centre = tuple(sum(w[i] for w in free) / len(free) for i in range(3))
    posts = spread(free, count, [])
    # Each member starts at the free waypoint nearest its post that is not itself a post, so the
    # first walk is short and stays on one stretch of the graph.
    rest = [w for w in free if w not in posts]
    starts = []
    for p in posts:
        s = min(rest, key=lambda w: (dist(w, p), w))
        starts.append(s)
        rest.remove(s)

    def facing(pos):
        return round(math.degrees(math.atan2(centre[1] - pos[1], centre[0] - pos[0])) % 360)

    # Only characters this installation actually defines.
    known = data.npc_types()
    types = [t for t in SUGGEST_TYPES if known is None or t.lower() in known]
    if not types:
        raise SystemExit("none of the suggested crew types are in this installation's NPC table")
    names = [f"watch{i + 1}" for i in range(count)]
    return {
        "map": mapname,
        "max": count,
        "members": [{"name": names[i], "type": types[i % len(types)],
                     "at": [round(c) for c in starts[i]], "yaw": facing(starts[i])} for i in range(count)],
        "posts": [{"name": f"post{i + 1}", "at": [round(c) for c in posts[i]], "yaw": facing(posts[i]),
                   "holder": names[i]} for i in range(count)],
    }


def load_section(scenario_dir):
    path = os.path.join(scenario_dir, "scenario.json")
    try:
        with open(path, encoding="utf-8") as f:
            manifest = json.load(f)
    except (OSError, json.JSONDecodeError) as e:
        raise SystemExit(f"{path}: {e}")
    if not isinstance(manifest, dict) or "crew" not in manifest:
        raise SystemExit(f"{path}: no 'crew' section")
    return manifest["crew"], path


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name in ("check", "build"):
        sp = sub.add_parser(name)
        sp.add_argument("scenario")
        sp.add_argument("--data", help="the installation (the directory holding BaseEF, or BaseEF itself)")
        if name == "build":
            sp.add_argument("--out", required=True, help="directory to write maps/<map>.crew under")
    sp = sub.add_parser("suggest")
    sp.add_argument("--data")
    sp.add_argument("--scenario", help="a scenario directory whose own maps/<map>.map is read first")
    sp.add_argument("--map", required=True)
    sp.add_argument("--crew", type=int, default=6)
    args = ap.parse_args(argv)

    try:
        data = GameData(args.data) if args.data else None
    except FileNotFoundError as e:
        raise SystemExit(str(e))

    if args.cmd == "suggest":
        if not 1 <= args.crew <= MAX_CREW:
            raise SystemExit(f"--crew must be 1..{MAX_CREW}")
        if not args.data and not args.scenario:
            raise SystemExit("suggest needs --data, --scenario, or both")
        source = ScenarioData(args.scenario, data) if args.scenario else data
        print(json.dumps({"crew": suggest(source, args.map, args.crew)}, indent=2))
        return 0

    crew, path = load_section(args.scenario)
    scoped = ScenarioData(args.scenario, data)
    local = isinstance(crew, dict) and isinstance(crew.get("map"), str) and scoped.has_local(crew["map"])
    errors, warnings = validate(crew, scoped if (data or local) else None)
    for w in warnings:
        print(f"warning: {w}")
    for e in errors:
        print(f"error: {e}")
    if errors:
        return 1
    if data is None and not local:
        print("note: no --data given; the section was not checked against the map itself")
    if args.cmd == "build":
        out = os.path.join(args.out, "maps", crew["map"] + ".crew")
        os.makedirs(os.path.dirname(out), exist_ok=True)
        with open(out, "w", encoding="utf-8") as f:
            f.write(render(crew, os.path.relpath(path)))
        print(f"wrote {out}")
    else:
        print(f"{path}: crew section OK ({len(crew.get('members', []))} members, {len(crew.get('posts', []))} posts)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
