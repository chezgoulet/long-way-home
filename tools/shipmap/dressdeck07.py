#!/usr/bin/env python3
"""Re-dress deck 7 -- the auxiliary computer core, cargo and labs -- from `tour/deck11`.

    dressdeck07.py --deck11 build/gdk/maps/.../deck11.map --out build/ship/generated

The brief (docs/locations/deck07-auxcore.brief.md) names the reuse target: **copy `tour/deck11`**
"for the industrial read; the core column is the deck's spine", and change what makes this room
*this* room. It is deck 14's pattern, reused, not a fourth mechanism: decks 12, 13 and 14 are left
untouched (their checks are the regression test), and this is deck 14's copy with deck 7's program.

  * copies the *main engineering room* -- the dense cluster of detail brushes, with the room's own
    consoles, railings, wall panels, door hardware and props, "including any small untidiness in
    the original";
  * **reduces the vertical scale** (the room is compressed toward its floor; the depth deck is a
    taller working space than the deck 13 hall -- 240 against its 224);
  * **clears the raised core deck** (the brief's "one chamber"): deck 11's large raised machinery
    platform is dropped, leaving the base floor, the walls and the smaller furniture;
  * **swaps the warp core for an auxiliary core column**: the core cluster is dropped and a tall
    column of the room's own core materials is raised at the centre as the deck's spine, with the
    core panel at its base;
  * adds **two cargo islands** (the brief's change): low plinths to the sides with the game's own
    crates (`models/mapobjects/cargo/crate.md3`, the brief's section 4) stacked on them, each crate
    seated on its local floor;
  * adds an **overhead escape-pod hatch** on a short walkable platform (the brief's "escape-pod
    access overhead"), reached by 16-unit steps;
  * adds a **Jefferies tube mouth** at the far end from the door (west), low and awkward, with a
    traversal to the main computer core -- the fallback route;
  * keeps the deck's wiring (the lift edge, the status panel, the navigation furniture) and adds the
    **computer-core station marker** (`lwh_station_3`, SYS_COMPUTER_CORE) and the deck's
    **core-watch post**;
  * **re-lights the room by function**: deck 11's own ceiling lights are kept but dimmed (they are
    engineering-white and light the copied structure; deck 13's evidence records that the copy alone
    reads dark), and authored working light plus a blue glow at the core are added on top;
  * adds the **emergency lighting state**: red strips (`hall/hall_light_red`, a shader the game
    already has) held off until the module switches them on, and the normal strips they replace.

Every texture and model this tool emits already exists in the shipped game: the parts list is a
census over the source it copies (--census). No new art.

Writes only under the output directory. The stitcher (tools/shipmap/stitch.py) then treats deck07.map
like any other deck source.
"""

import argparse
import collections
import json
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "mapgen"))
sys.path.insert(0, HERE)
from mapgen import brush  # noqa: E402
import stitch  # noqa: E402

# ---- the room, and how it is changed -------------------------------------------------------------
# Main engineering's room, from the deck source itself: the same crop deck 12, 13 and 14 took. The
# detail brushes (max horizontal extent <= 300) have centres x -4012..-2730, y -3972..-2814; snapped
# to a 16-grid and padded. Everything outside is the deck's corridors: not this room, not copied.
RX0, RX1 = -4096, -2704
RY0, RY1 = -4032, -2784
FLOOR = -3996            # the room's lower walkway; z compression keeps this fixed
HEIGHT = 240             # the room after re-dress: taller than deck 13's plant hall (224) -- the
                         # depth deck, so the core column reads tall and the escape hatch has headroom
VSCALE = 0.80            # the reduction in vertical scale
TOP = FLOOR + HEIGHT
ZHI = FLOOR + (HEIGHT + 64) / VSCALE
WALL = 16
WALLTEX, WALL0, WALL1 = "lwh/deck07wall2", "lwh/deck07wall0", "lwh/deck07wall1"
FLOORTEX = "lwh/deck07floor"
# Deck-7 detail shaders: aliases of the room's own materials with surfaceparm detail. The copied
# furniture must be detail or the fourth full re-dress pushes the ship past q3map2's vis-cluster
# ceiling; the aliases are emitted by --detail-shader (tools/shipmap/data is not touched).
DETAIL_PREFIX = "lwh/detail07/"

CY = (RY0 + RY1) // 2
DOOR_Y, DOOR_HALF, DOOR_H = CY, 56, 128
TUBE_Y, TUBE_HALF, TUBE_H, TUBE_LEN = RY0 + 320, 32, 64, 288
TUBE_DEST = "tour/deck10"          # the main computer core above; the tube is the fallback route

# The auxiliary core column: the deck's spine, at the centre of the room. The room's own core
# materials (they were the warp core's) build it; it is massing, not a simulation (the brief's
# non-goal).
CORE_X, CORE_Y = (RX0 + RX1) // 2, CY
CORE_HALF = 88
# The core panel and the watch post: on the east face, between the column and the door.

# The two cargo islands: low plinths to the sides, crates stacked on them. The game's own crate.
CRATE_MODEL = "models/mapobjects/cargo/crate.md3"
CRATE_MINS = (-24, -24, -16)       # from the shipped decks' own placements (deck04, deck08)
CRATE_MAXS = (24, 24, 16)
ISL = 128                          # island half-extent
ISLANDS = ((RX0 + 416, RY0 + 416), (RX1 - 416, RY1 - 416))

# The overhead escape-pod hatch, on a short walkable platform in the north-west corner (the brief's
# "escape-pod access overhead"). No pod model ships with the game, so the hatch is authored geometry
# from the room's own materials and a marker records the access.
PLAT_X, PLAT_Y = RX0 + 416, RY1 - 416
PLAT_HALF, PLAT_H = 160, 48
STEP_N, STEP_W = 3, 64

KEEP_CLASSES = {
    "waypoint", "waypoint_small",
    "waypoint_navgoal", "waypoint_navgoal_1", "waypoint_navgoal_2", "waypoint_navgoal_4",
    "func_door", "func_static", "func_usable",
    "target_speaker", "trigger_multiple", "misc_model_breakable",
}
DROP_NAMES = {
    "englift", "torres_lookatcore", "demo_lookup", "altmunro", "telbox",
    "liftstart", "liftpos4", "turbomenu",
}
CORE_TEX = ("corefloor", "chrome", "glass1", "beamsides", "epshatch")
CORE_BOX = (-3560, -3060, -3620, -3080)
FLOOR_MIN_AREA = 96 * 96


def fmt(v):
    return str(int(v)) if v == int(v) else ("%.4f" % v).rstrip("0").rstrip(".")


def aslines(b):
    if isinstance(b, str):
        ls = b.split("\n")
        if ls and ls[0].strip() == "{":
            ls = ls[1:]
        if ls and ls[-1].strip() == "}":
            ls = ls[:-1]
        return ls
    return b


def clampf(v, a, b):
    return a if v < a else b if v > b else v


def compress_z(z):
    return FLOOR + (z - FLOOR) * VSCALE


def brush_corners(lines):
    lo, hi = [1e9] * 3, [-1e9] * 3
    for ln in lines:
        if not ln.startswith("("):
            continue
        for m in stitch.POINT.finditer(ln):
            for i in range(3):
                lo[i] = min(lo[i], float(m.group(i + 1)))
                hi[i] = max(hi[i], float(m.group(i + 1)))
    return lo, hi


def brush_tex(lines):
    for ln in lines:
        if ln.startswith("("):
            m = stitch.FACE_TEXTURE.match(ln)
            if m:
                return m.group(2).lower()
    return ""


class Solids:
    def __init__(self):
        self.boxes = []

    def add(self, lines):
        lo, hi = brush_corners(lines)
        if lo[0] <= hi[0]:
            self.boxes.append((lo, hi))

    def add_box(self, lo, hi):
        self.boxes.append(([float(v) for v in lo], [float(v) for v in hi]))

    def contains(self, p, pad=16.0):
        for lo, hi in self.boxes:
            if (lo[0] - pad <= p[0] <= hi[0] + pad and lo[1] - pad <= p[1] <= hi[1] + pad
                    and lo[2] - pad <= p[2] <= hi[2] + pad):
                return True
        return False

    def floor_at(self, x, y):
        """The highest walkable surface under (x, y): a large, low brush top, else the walkway."""
        best = FLOOR
        for lo, hi in self.boxes:
            if not (lo[0] - 8 <= x <= hi[0] + 8 and lo[1] - 8 <= y <= hi[1] + 8):
                continue
            if hi[2] <= FLOOR + HEIGHT - 64 and (hi[0] - lo[0]) * (hi[1] - lo[1]) >= FLOOR_MIN_AREA \
                    and hi[2] > best:
                best = hi[2]
        return best


def transform_line(ln):
    if not ln.startswith("("):
        return ln
    n = [0]

    def rep(m):
        n[0] += 1
        if n[0] > 3:
            return m.group(0)
        x = clampf(float(m.group(1)), RX0, RX1)
        y = clampf(float(m.group(2)), RY0, RY1)
        z = clampf(float(m.group(3)), FLOOR - 256, ZHI)
        return "( %s %s %s )" % (fmt(x), fmt(y), fmt(compress_z(z)))
    return stitch.POINT.sub(rep, ln)


def xform_brush(b):
    if b.is_patch:
        if not (RX0 <= b.lo[0] and b.hi[0] <= RX1 and RY0 <= b.lo[1] and b.hi[1] <= RY1):
            return None
        out = []
        for ln in b.lines:
            if ln.startswith("( ("):
                ln = stitch.PATCH_ROW.sub(
                    lambda m: "( %s %s %s %s %s )" % (m.group(1), m.group(2), fmt(compress_z(float(m.group(3)))),
                                                      m.group(4), m.group(5)), ln)
            out.append(ln)
        return out
    inside = (RX0 <= b.lo[0] and b.hi[0] <= RX1 and RY0 <= b.lo[1] and b.hi[1] <= RY1)
    if not b.is_box() and not inside:
        return None
    return [transform_line(ln) for ln in b.lines]


def compress_origin(v):
    try:
        x, y, z = (float(t) for t in v.split())
    except ValueError:
        return v
    z = clampf(compress_z(z), FLOOR + 8, TOP - 8)
    return "%s %s %s" % (fmt(x), fmt(y), fmt(z))


def in_core(b):
    cx, cy = (b.lo[0] + b.hi[0]) / 2, (b.lo[1] + b.hi[1]) / 2
    tex = ""
    for ln in b.lines:
        if ln.startswith("("):
            m = stitch.FACE_TEXTURE.match(ln)
            if m:
                tex = m.group(2).lower()
                break
    return ((b.zmax - b.zmin) > 96 and any(t in tex for t in CORE_TEX)
            and CORE_BOX[0] <= cx <= CORE_BOX[1] and CORE_BOX[2] <= cy <= CORE_BOX[3])


def census(ents):
    models, texs, classes = collections.Counter(), collections.Counter(), collections.Counter()
    for keys, brushes in ents:
        classes[keys.get("classname", "")] += 1
        if keys.get("model"):
            models[keys["model"]] += 1
        for b in brushes:
            for ln in b.lines:
                if ln.startswith("("):
                    m = stitch.FACE_TEXTURE.match(ln)
                    if m:
                        texs[m.group(2)] += 1
                        break
    return {"models": dict(models.most_common()), "textures": dict(texs.most_common()),
            "classes": dict(classes.most_common())}


class Build:
    def __init__(self, deck11_path):
        self.report = {
            "copied_from": os.path.basename(deck11_path),
            "room": {"x": [RX0, RX1], "y": [RY0, RY1], "floor": FLOOR, "height_before": None,
                     "height_after": HEIGHT, "vscale": VSCALE},
            "entities_kept": 0, "entities_dropped": 0, "brushes_kept": 0, "brushes_dropped": 0,
            "patches_dropped": 0, "core_brushes_dropped": 0, "platform_brushes_dropped": 0,
            "lights_kept": 0, "light_value_before": 0, "light_value_kept": 0,
            "waypoints": 0, "crates": [], "islands": [], "core_column": None, "hatch": None,
            "census": census(stitch.parse(deck11_path)),
        }
        self.world = []
        self.entities = []
        self.detail_textures = set()
        self.solids = Solids()
        self._copy_room(stitch.parse(deck11_path))
        self._shell()
        self._door_and_lobby()
        self._tube()
        self._core_column()
        self._cargo()
        self._escape_hatch()
        self._console()
        self._lighting()
        self._fixtures()
        self._wiring()
        self._waypoints()

    # -- the copy ---------------------------------------------------------------------------------
    def _copy_room(self, ents):
        tops = []
        for keys, brushes in ents:
            cls = keys.get("classname", "")
            if cls in ("worldspawn", "func_group"):
                self._copy_brushes(brushes, tops, world=True, detail=True)
                continue
            if cls == "light":
                # Deck 11's own ceiling lights light its copied structure; keep the ones inside the
                # room, but dim them so the authored working light reads on top. A light left outside
                # the sealed room is an entity in the void and a leak, so the crop still applies.
                try:
                    ox, oy, oz = (float(t) for t in keys.get("origin", "").split())
                except ValueError:
                    self.report["entities_dropped"] += 1
                    continue
                if not (RX0 - 64 <= ox <= RX1 + 64 and RY0 - 64 <= oy <= RY1 + 64 and oz <= -2048):
                    self.report["entities_dropped"] += 1
                    continue
                k = collections.OrderedDict(keys)
                k["origin"] = compress_origin(k["origin"])
                try:
                    before = int(float(k.get("light", "300")))
                except ValueError:
                    before = 0
                self.report["light_value_before"] += before
                try:
                    k["light"] = str(max(40, int(before * 0.7)))
                except ValueError:
                    pass
                self.report["light_value_kept"] += int(float(k.get("light", "0")))
                self.entities.append((k, []))
                self.report["lights_kept"] += 1
                continue
            if cls not in KEEP_CLASSES or keys.get("targetname", "") in DROP_NAMES \
                    or keys.get("script_targetname", "") in DROP_NAMES:
                self.report["entities_dropped"] += 1
                continue
            kept = self._copy_brushes(brushes, tops, world=False)
            if brushes and (not kept or all(brush_tex(l) in ("common/caulk", "common/origin") for l in kept)):
                self.report["entities_dropped"] += 1
                continue
            if not brushes and "origin" in keys:
                try:
                    ox, oy, oz = (float(t) for t in keys["origin"].split())
                except ValueError:
                    self.report["entities_dropped"] += 1
                    continue
                if not (RX0 - 64 <= ox <= RX1 + 64 and RY0 - 64 <= oy <= RY1 + 64 and oz <= -2048):
                    self.report["entities_dropped"] += 1
                    continue
            k = collections.OrderedDict(keys)
            if "origin" in k:
                k["origin"] = compress_origin(k["origin"])
            self.entities.append((k, kept))
            self.report["entities_kept"] += 1
        self.report["room"]["height_before"] = int(max(tops) - FLOOR) if tops else 0

    def _copy_brushes(self, brushes, tops, world, detail=False):
        kept = []
        for b in brushes:
            if b.zmin > -2048:
                continue
            if b.zmax <= ZHI + 128:
                tops.append(b.zmax)
            if in_core(b):
                self.report["core_brushes_dropped"] += 1
                continue
            # Deck 11's main engineering is a two-level room: a large raised core deck over most of
            # the chamber, ringed by consoles. The brief wants one chamber, so the *large* raised
            # slabs above the floor are dropped, leaving the base floor, the walls and the smaller
            # furniture. The shell seals the room regardless of what interior machinery is removed.
            if (compress_z(b.lo[2]) > FLOOR + 40 and compress_z(b.hi[2]) < TOP - 8
                    and (b.hi[0] - b.lo[0]) * (b.hi[1] - b.lo[1]) >= 50000):
                self.report["platform_brushes_dropped"] += 1
                continue
            lines = xform_brush(b)
            if lines is None:
                self.report["patches_dropped" if b.is_patch else "brushes_dropped"] += 1
                continue
            if detail and not b.is_patch:
                lines = self._as_detail(lines)
            self.report["brushes_kept"] += 1
            (self.world if world else kept).append(lines)
            self.solids.add(lines)
        return kept

    # -- detail geometry --------------------------------------------------------------------------
    def _as_detail(self, lines):
        """Point a room brush's faces at deck-7 detail shaders (same images, surfaceparm detail).
        Detail brushes do not split the BSP tree, so the copied furniture does not add vis clusters:
        the ship is at q3map2's vis-cluster ceiling (MAX_MAP_VISCLUSTERS, 16384) and a fourth full
        re-dress would push it over. The shell, the door and the tube stay structural and seal the
        room; everything inside it is detail. The aliases are written by --detail-shader, shipped in
        the pak, and loaded by the compiler from the shader mod dir (scripts/build-ship.sh)."""
        out = []
        for ln in lines:
            if ln.startswith("("):
                m = stitch.FACE_TEXTURE.match(ln)
                if m:
                    tex = m.group(2)
                    if not (tex.startswith("lwh/deck07") or tex.startswith("jefferies/")):
                        self.detail_textures.add(tex)
                        ln = m.group(1) + DETAIL_PREFIX + tex + ln[m.end():]
            out.append(ln)
        return out

    # -- new construction -------------------------------------------------------------------------
    def w(self, lines, detail=False):
        lines = aslines(lines)
        if detail:
            lines = self._as_detail(lines)
        self.world.append(lines)
        self.solids.add(lines)

    def named_brush(self, cls, name, tex, bounds, extra=None):
        keys = collections.OrderedDict([("classname", cls), ("targetname", name)])
        for k, v in (extra or {}).items():
            keys[k] = v
        lines = aslines(brush(bounds, tex))
        self.entities.append((keys, [lines]))
        self.solids.add(lines)

    def point(self, cls, name, origin, extra=None):
        keys = collections.OrderedDict([("classname", cls)])
        if name:
            keys["targetname"] = name
        for k, v in (extra or {}).items():
            keys[k] = v
        keys["origin"] = "%d %d %d" % origin
        self.entities.append((keys, []))

    def model(self, cls, name, model_path, origin, angle, mins, maxs, extra=None):
        keys = collections.OrderedDict([("classname", cls)])
        if name:
            keys["targetname"] = name
        for k, v in (extra or {}).items():
            keys[k] = v
        keys["model"] = model_path
        keys["angle"] = str(angle)
        keys["mins"] = "%d %d %d" % mins
        keys["maxs"] = "%d %d %d" % maxs
        keys["autobound"] = "1"
        keys["origin"] = "%d %d %d" % origin
        self.entities.append((keys, []))
        self.solids.add_box((origin[0] + mins[0], origin[1] + mins[1], origin[2] + mins[2]),
                            (origin[0] + maxs[0], origin[1] + maxs[1], origin[2] + maxs[2]))

    def _shell(self):
        self.w(brush(((RX0 - WALL, RY0 - WALL, FLOOR - WALL), (RX1 + WALL, RY0, TOP + WALL)), WALLTEX))
        self.w(brush(((RX0 - WALL, RY1, FLOOR - WALL), (RX1 + WALL, RY1 + WALL, TOP + WALL)), WALLTEX))
        self._wall("x", RX1, RY0, RY1, [(DOOR_Y - DOOR_HALF, DOOR_Y + DOOR_HALF, DOOR_H)], WALL0)
        self._wall("x", RX0 - WALL, RY0, RY1, [(TUBE_Y - TUBE_HALF, TUBE_Y + TUBE_HALF, TUBE_H)], WALL1)
        self.w(brush(((RX0 - WALL, RY0 - WALL, FLOOR - WALL), (RX1 + WALL, RY1 + WALL, FLOOR)), FLOORTEX))
        self.w(brush(((RX0 - WALL, RY0 - WALL, TOP), (RX1 + WALL, RY1 + WALL, TOP + WALL)), WALLTEX))

    def _wall(self, axis, fixed, a0, a1, openings, tex):
        segs = [(a0, a1)]
        for (olo, ohi, oh) in openings:
            nxt = []
            for (s0, s1) in segs:
                if ohi <= s0 or olo >= s1:
                    nxt.append((s0, s1))
                else:
                    if s0 < olo:
                        nxt.append((s0, olo))
                    if ohi < s1:
                        nxt.append((ohi, s1))
            segs = nxt
        for (s0, s1) in segs:
            if axis == "x":
                self.w(brush(((fixed, s0, FLOOR - WALL), (fixed + WALL, s1, TOP + WALL)), tex))
            else:
                self.w(brush(((s0, fixed, FLOOR - WALL), (s1, fixed + WALL, TOP + WALL)), tex))
        for (olo, ohi, oh) in openings:
            if axis == "x":
                self.w(brush(((fixed, olo, FLOOR + oh), (fixed + WALL, ohi, TOP + WALL)), tex))
            else:
                self.w(brush(((olo, fixed, FLOOR + oh), (ohi, fixed + WALL, TOP + WALL)), tex))

    def _door_and_lobby(self):
        y0, y1 = DOOR_Y - DOOR_HALF, DOOR_Y + DOOR_HALF
        self.named_brush("func_door", "lwh_d07_door", "cargo/cargodoor1",
                         ((RX1, y0, FLOOR), (RX1 + WALL, y1, FLOOR + DOOR_H)),
                         {"angle": "360", "speed": "50", "lip": "4", "wait": "3", "dmg": "-1",
                          "soundSet": "voyagerdoors", "team": "engdoor1"})
        lx0, lx1 = RX1, RX1 + 192
        for lo, hi, tex in [
            ((lx0, y0 - 64, FLOOR - WALL), (lx1 + WALL, y1 + 64, FLOOR), FLOORTEX),
            ((lx0, y0 - 64, FLOOR + DOOR_H), (lx1 + WALL, y1 + 64, FLOOR + DOOR_H + WALL), WALLTEX),
            ((lx0, y0 - 64, FLOOR), (lx1 + WALL, y0, FLOOR + DOOR_H + WALL), WALLTEX),
            ((lx0, y1, FLOOR), (lx1 + WALL, y1 + 64, FLOOR + DOOR_H + WALL), WALLTEX),
            ((lx1, y0 - 64, FLOOR), (lx1 + WALL, y1 + 64, FLOOR + DOOR_H + WALL), WALLTEX),
        ]:
            self.w(brush((lo, hi), tex))

    def _tube(self):
        tx = RX0 - WALL
        tz = FLOOR + TUBE_H
        self.w(brush(((tx - TUBE_LEN, TUBE_Y - TUBE_HALF, FLOOR - WALL), (tx, TUBE_Y + TUBE_HALF, FLOOR)), "jefferies/basesides"))
        self.w(brush(((tx - TUBE_LEN, TUBE_Y - TUBE_HALF, tz), (tx, TUBE_Y + TUBE_HALF, tz + WALL)), "jefferies/basesides"))
        self.w(brush(((tx - TUBE_LEN, TUBE_Y - TUBE_HALF - WALL, FLOOR), (tx, TUBE_Y - TUBE_HALF, tz + WALL)), "jefferies/sides"))
        self.w(brush(((tx - TUBE_LEN, TUBE_Y + TUBE_HALF, FLOOR), (tx, TUBE_Y + TUBE_HALF + WALL, tz + WALL)), "jefferies/sides"))
        ex0 = tx - TUBE_LEN - 224
        # the chamber's west face, closed around the tube opening (or the chamber is open to the void)
        self.w(brush(((tx - TUBE_LEN - WALL, TUBE_Y - 96, FLOOR), (tx - TUBE_LEN, TUBE_Y - TUBE_HALF, FLOOR + 96 + WALL)), "jefferies/sides"))
        self.w(brush(((tx - TUBE_LEN - WALL, TUBE_Y + TUBE_HALF, FLOOR), (tx - TUBE_LEN, TUBE_Y + 96, FLOOR + 96 + WALL)), "jefferies/sides"))
        self.w(brush(((tx - TUBE_LEN - WALL, TUBE_Y - TUBE_HALF, FLOOR + TUBE_H), (tx - TUBE_LEN, TUBE_Y + TUBE_HALF, FLOOR + 96 + WALL)), "jefferies/sides"))
        for lo, hi, tex in [
            ((ex0, TUBE_Y - 96, FLOOR - WALL), (tx - TUBE_LEN, TUBE_Y + 96, FLOOR), "jefferies/basesides"),
            ((ex0, TUBE_Y - 96, FLOOR + 96), (tx - TUBE_LEN, TUBE_Y + 96, FLOOR + 96 + WALL), "jefferies/basesides"),
            ((ex0, TUBE_Y - 96 - WALL, FLOOR), (tx - TUBE_LEN, TUBE_Y - 96, FLOOR + 96 + WALL), "jefferies/sides"),
            ((ex0, TUBE_Y + 96, FLOOR), (tx - TUBE_LEN, TUBE_Y + 96 + WALL, FLOOR + 96 + WALL), "jefferies/sides"),
            ((ex0 - WALL, TUBE_Y - 96, FLOOR), (ex0, TUBE_Y + 96, FLOOR + 96 + WALL), "jefferies/sides"),
        ]:
            self.w(brush((lo, hi), tex))
        self.point("target_level_change", "lwh_tube_d07", (ex0 + 48, TUBE_Y, FLOOR + 24),
                   {"mapname": TUBE_DEST})
        self.point("info_notnull", "lwh_tube_mouth", (RX0 - 48, TUBE_Y, FLOOR + 24))

    def _core_column(self):
        # The auxiliary core column: the deck's spine, at the centre, built from the room's own core
        # materials (the ones the dropped warp core used), floor to ceiling so it reads as the depth.
        cx, cy, h = CORE_X, CORE_Y, CORE_HALF
        base = self.solids.floor_at(cx, cy)
        self.w(brush(((cx - h - 40, cy - h - 40, base - 16), (cx + h + 40, cy + h + 40, base + 32)), "engineering/supportedge"), detail=True)
        self.w(brush(((cx - h, cy - h, base + 32), (cx + h, cy + h, TOP + WALL)), "engineering/corefloor"), detail=True)
        self.w(brush(((cx - h - 8, cy - h - 8, base + 32), (cx + h + 8, cy + h + 8, base + 96)), "engineering/chrome"), detail=True)
        self.w(brush(((cx - h - 8, cy - h - 8, base + 128), (cx + h + 8, cy + h + 8, base + 192)), "engineering/glass1"), detail=True)
        self.w(brush(((cx - h - 8, cy - h - 8, TOP - 96), (cx + h + 8, cy + h + 8, TOP - 32)), "engineering/beamsides"), detail=True)
        self.report["core_column"] = [cx, cy, base]

    def _cargo(self):
        # Two low cargo islands to the sides, the game's own crates stacked on them, each seated on the
        # island's top so nothing floats or is buried (deck 12's plant vessels are placed the same way).
        for i, (ix, iy) in enumerate(ISLANDS):
            base = self.solids.floor_at(ix, iy)
            top = base + 16
            self.w(brush(((ix - ISL, iy - ISL, base - 8), (ix + ISL, iy + ISL, top)), "engineering/supportedge"), detail=True)
            self.w(brush(((ix - ISL, iy - ISL, top), (ix + ISL, iy + ISL, top + 4)), "engineering/newgrey1"), detail=True)
            self.report["islands"].append([ix, iy, top])
            # a small stack, deliberately untidy (the brief's "one human trace" is reused from the copy)
            for dx, dy, dz in ((0, 0, 0), (80, -32, 0), (-8, 96, 0), (72, 64, 32)):
                px, py = ix + dx, iy + dy
                self.model("misc_model_breakable", None, CRATE_MODEL, (px, py, top + 16 + dz), 0,
                           CRATE_MINS, CRATE_MAXS)
                self.report["crates"].append([px, py, top + 16 + dz])

    def _escape_hatch(self):
        # The brief's "escape-pod access overhead": a short walkable platform with 16-unit steps, and a
        # hatch in the ceiling above it. No pod model ships, so the hatch is authored from the room's
        # own materials; the marker records the access.
        x, y, hh = PLAT_X, PLAT_Y, PLAT_HALF
        base = self.solids.floor_at(x, y)
        top = base + PLAT_H
        self.w(brush(((x - hh, y - hh, base - 8), (x + hh, y + hh, top)), "engineering/newgrey1"), detail=True)
        self.w(brush(((x - hh, y - hh, top), (x + hh, y + hh, top + 8)), "engineering/supportedge"), detail=True)
        # steps down the south side: 16-unit rises, the engine steps over 16, not 24
        for i in range(STEP_N):
            sy = y - hh - (i + 1) * STEP_W
            st = base + (PLAT_H // STEP_N) * (STEP_N - i)
            self.w(brush(((x - 96, sy, base), (x + 96, sy + STEP_W, st)), "engineering/newgrey1"), detail=True)
        # the ceiling hatch above the platform: a door panel flush under the ceiling with a lit rim
        self.w(brush(((x - 56, y - 56, TOP - 12), (x + 56, y + 56, TOP)), "cargo/cargodoor1"), detail=True)
        for lo, hi in [
            ((x - 72, y - 72, TOP - 16), (x + 72, y - 56, TOP)), ((x - 72, y + 56, TOP - 16), (x + 72, y + 72, TOP)),
            ((x - 72, y - 56, TOP - 16), (x - 56, y + 56, TOP)), ((x + 56, y - 56, TOP - 16), (x + 72, y + 56, TOP)),
        ]:
            self.w(brush((lo, hi), "engineering/elight1"), detail=True)
        self.point("info_notnull", "lwh_pod_hatch", (x, y, top + 24))
        self.report["hatch"] = [x, y, top]

    def _console(self):
        # The core panel at the base of the column, facing the door: the brief's control surface, with
        # the computer-core station marker (SYS_COMPUTER_CORE = 3) and the watch post in front of it.
        cx, cy = CORE_X + CORE_HALF + 64, CORE_Y   # the console, seated against the column's east face
        base = self.solids.floor_at(cx, cy)
        self.w(brush(((cx - 64, cy - 48, base), (cx + 64, cy + 48, base + 48)), "engineering/console1"), detail=True)
        self.w(brush(((cx - 56, cy - 40, base + 48), (cx + 56, cy + 40, base + 80)), "voyager/basic"), detail=True)
        self.w(brush(((cx - 48, cy - 40, base + 80), (cx + 48, cy - 32, base + 120)), "lwh/panel"), detail=True)
        self.point("info_notnull", "lwh_station_3", (cx + 48, cy + 80, self.solids.floor_at(cx + 48, cy + 80) + 24))
        # The watch post, a pace off the panel and measured clear, so the crew (and the check) stand
        # on the floor, not in a wall: the copied room is dense here.
        px, py = cx + 48, cy + 112
        self.point("info_notnull", "lwh_core_post", (px, py, self.solids.floor_at(px, py) + 24))
        self.point("waypoint_navgoal_1", "corewatch", (px, py, self.solids.floor_at(px, py) + 24))
        self.report["core_panel"] = [cx, cy, px, py]

    def _lighting(self):
        # Light by function: working neutral light over the cargo floor and a blue glow at the core
        # column, so the room reads and the spine is the brightest thing in it. Deck 11's own lights are
        # kept dimmed (see _copy_room) as the baseline over the copied structure.
        work = 0
        for x in range(RX0 + 256, RX1 - 96, 224):
            for y in range(RY0 + 192, RY1 - 160, 224):
                for z in (FLOOR + 160, FLOOR + 224):
                    if self.solids.contains((x, y, z), pad=24):
                        continue
                    self.point("light", None, (x, y, z), {"light": "260", "_color": "0.92 0.94 1.0"})
                    work += 1
        self.report["working_lights"] = work
        glow = 0
        for dx, dy in ((-CORE_HALF - 96, 0), (CORE_HALF + 96, 0), (0, -CORE_HALF - 96), (0, CORE_HALF + 96)):
            self.point("light", None, (CORE_X + dx, CORE_Y + dy, FLOOR + 128),
                       {"light": "300", "_color": "0.35 0.60 1.0"})
            glow += 1
        for dx, dy in ((-CORE_HALF - 96, 0), (CORE_HALF + 96, 0)):
            self.point("light", None, (CORE_X + dx, CORE_Y + dy, TOP - 96),
                       {"light": "220", "_color": "0.35 0.60 1.0"})
            glow += 1
        self.report["core_lights"] = glow

    def _fixtures(self):
        # Working light and the emergency red, as strips along the ceiling, interleaved, so the
        # state is visible from anywhere in the room. The red ones start off; the module swaps them.
        # The names carry a deck tag (07): decks 12, 13 and 14 define the same module prefix, and two
        # decks defining one name would make the stitcher rename both, leaving the module nothing to
        # toggle.
        span = RX1 - RX0 - 512
        seg = span // 8
        n = 0
        for i in range(8):
            x0 = RX0 + 256 + i * seg
            x1 = x0 + seg - 32
            if i % 2 == 0:
                self.named_brush("func_usable", "lwh_light_normal_07_%d" % n, "engineering/elight1",
                                 ((x0, CY - 24, TOP - 24), (x1, CY + 24, TOP - 2)))
            else:
                self.named_brush("func_usable", "lwh_light_emergency_07_%d" % n, "hall/hall_light_red",
                                 ((x0, CY - 24, TOP - 24), (x1, CY + 24, TOP - 2)), {"spawnflags": "1"})
            n += 1
        self.report["light_strips"] = n

    def _wiring(self):
        px = RX1 - 320
        self.w(brush(((px, RY0 + WALL, FLOOR + 40), (px + WALL, RY0 + WALL + 128, FLOOR + 168)), "lwh/panel"), detail=True)
        (ax, ay), aang = self._arrival()
        self.point("info_player_start", None, (ax, ay, self.solids.floor_at(ax, ay) + 24), {"angle": str(aang)})
        self.point("target_level_change", "lwh_lift_d07", (RX1 + 96, DOOR_Y, FLOOR + 24), {"mapname": "tour/deck04"})
        self.point("target_shaderremap", None, (px + 48, RY0 + WALL + 64, FLOOR + 24),
                   {"falsename": "textures/lwh/panel", "truename": "textures/lwh/borg"})

    def _blocked(self, x, y, z, hx=15.0, hy=15.0, zlo=-22.0, zhi=30.0):
        """Would the engine's waypoint box fit here? The engine checks a standing box (roughly
        +-15 in x/y, -24..+32 in z) before it accepts a waypoint; a bare point test lets one sit in
        a wall corner. Used to filter the waypoint grid."""
        lo = (x - hx, y - hy, z + zlo)
        hi = (x + hx, y + hy, z + zhi)
        for blo, bhi in self.solids.boxes:
            if (lo[0] <= bhi[0] and hi[0] >= blo[0] and lo[1] <= bhi[1] and hi[1] >= blo[1]
                    and lo[2] <= bhi[2] and hi[2] >= blo[2]):
                return True
        return False

    def _waypoints(self):
        copied = []
        for k, _ in self.entities:
            if k.get("classname") in ("waypoint", "waypoint_small") and "origin" in k:
                a = k["origin"].split()
                copied.append((float(a[0]), float(a[1])))
        self.entities = [(k, b) for k, b in self.entities
                         if k.get("classname") not in ("waypoint", "waypoint_small")]
        cand = copied + [(x, y) for x in range(RX0 + 128, RX1 - 64, 128)
                         for y in range(RY0 + 128, RY1 - 64, 128)]
        seen, final = set(), []
        for (x, y) in cand:
            if not (RX0 + 32 <= x <= RX1 - 32 and RY0 + 32 <= y <= RY1 - 32):
                continue
            key = (round(x / 64), round(y / 64))
            if key in seen:
                continue
            z = self.solids.floor_at(x, y) + 24
            if self._blocked(x, y, z):
                continue
            seen.add(key)
            final.append((int(x), int(y), int(z)))
        # The core post, the panel and the escape platform are off the 128-grid: make sure they have a
        # route even if the grid missed them.
        cpx, cpy = self.report["core_panel"][2], self.report["core_panel"][3]
        for x in (cpx - 64, cpx, cpx + 64):
            z = self.solids.floor_at(x, cpy) + 24
            if not self._blocked(x, cpy, z):
                final.append((int(x), int(cpy), int(z)))
        for x in range(PLAT_X - PLAT_HALF + 32, PLAT_X + PLAT_HALF, 128):
            z = FLOOR + PLAT_H + 24
            if self.solids.floor_at(x, PLAT_Y) == FLOOR + PLAT_H and not self._blocked(x, PLAT_Y, z):
                final.append((int(x), int(PLAT_Y), int(z)))
        for (x, y, z) in final:
            self.point("waypoint", None, (x, y, z))
        self.report["waypoints"] = len(final)

    def _find_open(self, cx, cy):
        for r in range(0, 1200, 64):
            for dx, dy in ((0, 0), (r, 0), (-r, 0), (0, r), (0, -r), (r, r), (-r, r), (r, -r), (-r, -r)):
                x, y = cx + dx, cy + dy
                if not (RX0 + 160 <= x <= RX1 - 160 and RY0 + 160 <= y <= RY1 - 160):
                    continue
                z = self.solids.floor_at(x, y) + 24
                if any(self.solids.contains((x + ox, y + oy, z), pad=12)
                       for ox, oy in ((0, 0), (64, 0), (-64, 0), (0, 64), (0, -64))):
                    continue
                return (x, y)
        return None

    def _clear_len(self, x, y, z, ang, maxlen):
        dx, dy = math.cos(math.radians(ang)), math.sin(math.radians(ang))
        d = 0
        while d < maxlen:
            d += 16
            if self.solids.contains((x + dx * d, y + dy * d, z), pad=6):
                return d
        return maxlen

    def _arrival(self):
        """A spot near the door to set the player down, facing its most open sightline."""
        best = (-1e9, None, 0)
        for x in range(RX1 - 96, RX0 + 224, -128):
            for y in range(RY0 + 192, RY1 - 160, 128):
                f = self.solids.floor_at(x, y)
                if f > FLOOR + HEIGHT - 72:      # no headroom for a standing player
                    continue
                z = f + 40
                if self.solids.contains((x, y, z), pad=16):
                    continue
                for ang in range(0, 360, 45):
                    d = self._clear_len(x, y, z, ang, 256)
                    score = d - 0.20 * (RX1 - x)   # prefer the door end
                    if score > best[0]:
                        best = (score, (x, y), ang)
        return best[1] or (RX1 - 224, DOOR_Y), best[2]


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--deck11", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--report")
    ap.add_argument("--census")
    ap.add_argument("--detail-shader", help="write the deck-7 detail shader aliases here (for the compiler and the pak)")
    a = ap.parse_args(argv)

    b = Build(a.deck11)
    os.makedirs(a.out, exist_ok=True)
    path = os.path.join(a.out, "deck07.map")
    world_keys = collections.OrderedDict([
        ("classname", "worldspawn"),
        ("message", "Deck 7 - Auxiliary Computer Core (re-dressed from tour/deck11)"),
        ("music", "music/elite13.mp3"),
        ("soundSet", "voyjeffries"),
    ])
    with open(path, "w", encoding="latin-1") as f:
        f.write("{\n" + "".join('"%s" "%s"\n' % kv for kv in world_keys.items()))
        for lines in b.world:
            f.write("{\n" + "\n".join(lines) + "\n}\n")
        f.write("}\n")
        for i, (keys, brushes) in enumerate(b.entities, 1):
            f.write("// entity %d\n{\n" % i)
            for k, v in keys.items():
                f.write('"%s" "%s"\n' % (k, v))
            for lines in brushes:
                f.write("{\n" + "\n".join(lines) + "\n}\n")
            f.write("}\n")
    if a.report:
        with open(a.report, "w") as f:
            json.dump(b.report, f, indent=1)
    if a.census:
        with open(a.census, "w") as f:
            json.dump(b.report["census"], f, indent=1)
    if a.detail_shader:
        os.makedirs(os.path.dirname(os.path.abspath(a.detail_shader)), exist_ok=True)
        with open(a.detail_shader, "w", encoding="latin-1") as f:
            f.write("// Generated by tools/shipmap/dressdeck07.py: deck-7 detail shaders (S8, deck 7).\n")
            f.write("// Same images as the room's own materials, with surfaceparm detail: the copied\n")
            f.write("// furniture must not split the BSP tree, or the merged ship passes q3map2's\n")
            f.write("// vis-cluster ceiling (MAX_MAP_VISCLUSTERS, 16384). The shell, door and tube stay\n")
            f.write("// structural and seal the room.\n")
            for tex in sorted(b.detail_textures):
                f.write("textures/%s%s\n{\n\tsurfaceparm detail\n\t{\n\t\tmap textures/%s\n\t}\n}\n"
                        % (DETAIL_PREFIX, tex, tex))
    r = b.report
    r["detail_textures"] = sorted(b.detail_textures)
    print("deck07 <- %s" % a.deck11)
    print("  room %d..%d x %d..%d, height %s -> %s (x%g)"
          % (RX0, RX1, RY0, RY1, r["room"]["height_before"], HEIGHT, VSCALE))
    print("  world brushes %d, entities %d, waypoints %d, core dropped %d, platforms dropped %d, "
          "patches dropped %d, lights kept %d (%d -> %d light), crates %d, detail materials %d"
          % (len(b.world), len(b.entities), r["waypoints"], r["core_brushes_dropped"],
             r["platform_brushes_dropped"], r["patches_dropped"], r["lights_kept"],
             r["light_value_before"], r["light_value_kept"], len(r["crates"]), len(r["detail_textures"])))
    print("  wrote %s" % path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
