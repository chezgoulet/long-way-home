#!/usr/bin/env python3
"""Re-dress deck 14 -- stasis chambers and Holodeck 1 -- from `tour/deck11`, main engineering.

    dressdeck14.py --deck11 build/gdk/maps/.../deck11.map --out build/ship/generated

The brief (docs/locations/deck14-stasis.brief.md) names the reuse target: **copy `tour/deck11`**
"for the industrial read", and change what makes this room *this* room. It is deck 12's and deck
13's pattern -- read the published `deck11.map` and write a deck map -- not a third mechanism:
decks 12 and 13 are left untouched (their checks are the regression test), and this is deck 13's
copy with deck 14's program.

  * copies the *main engineering room* -- the dense cluster of detail brushes, with the room's own
    consoles, railings, wall panels, door hardware and props, "including any small untidiness in
    the original";
  * **reduces the vertical scale** (the room is compressed toward its floor; a stasis ward is lower
    than the plant hall deck 13 built -- 208 against its 224);
  * **clears the raised core deck** (the brief's "flat" chamber): deck 11's large raised machinery
    platform is dropped, leaving the base floor, the walls and the smaller furniture;
  * **swaps the warp core for banks of stasis pods**: the core cluster is dropped and a row of the
    game's own stasis pods (`models/mapobjects/stasis/pod.md3`, the brief's section 4) is set along
    the north wall, the machinery wall the brief asks for, each on a pedestal at its local floor;
  * adds a **holodeck doorframe** at the far (west) end of the pod row, on the north wall, warm-lit,
    with the holodeck's control panel -- Holodeck 1, the placement this deck decides; the numbered
    SYS_HOLODECKS station marker moved to deck 6's re-dress when deck 6 was built (ship_core's
    SystemSpec puts the system on deck 6), leaving this deck the frame and a maintenance post;
  * adds a **Jefferies tube mouth** at the far end from the door (west), low and awkward, with a
    traversal to deck 13 -- the route down when the turbolifts are down;
  * keeps the deck's wiring (the lift edge, the status panel, the navigation furniture) and adds the
    **holodeck station marker** and the deck's **stasis/holodeck maintenance post**;
  * **re-lights the room by function**: cold and dim over the pods, warm at the holodeck. Deck 11's
    own ceiling lights are kept but dimmed (they are engineering-white and light the copied
    structure; deck 13's evidence records that the copy alone reads dark), and authored cold,
    neutral and warm lights are added on top to set the two zones;
  * adds the **emergency lighting state**: red strips (`hall/hall_light_red`, a shader the game
    already has) held off until the module switches them on, and the normal strips they replace.

Every texture and model this tool emits already exists in the shipped game: the parts list is a
census over the source it copies (--census). No new art.

Writes only under the output directory. The stitcher (tools/shipmap/stitch.py) then treats deck14.map
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
# Main engineering's room, from the deck source itself: the same crop deck 12 and deck 13 took. The
# detail brushes (max horizontal extent <= 300) have centres x -4012..-2730, y -3972..-2814; snapped
# to a 16-grid and padded. Everything outside is the deck's corridors: not this room, not copied.
RX0, RX1 = -4096, -2704
RY0, RY1 = -4032, -2784
FLOOR = -3996            # the room's lower walkway; z compression keeps this fixed
HEIGHT = 208             # the room after re-dress: between deck 12's control room (192) and deck
                         # 13's plant hall (224). A stasis ward is a low, clinical room; the pod
                         # banks are 96-unit units and want headroom over them.
VSCALE = 0.72            # the reduction in vertical scale
TOP = FLOOR + HEIGHT
ZHI = FLOOR + (HEIGHT + 64) / VSCALE
WALL = 16
WALLTEX, WALL0, WALL1 = "lwh/deck14wall2", "lwh/deck14wall0", "lwh/deck14wall1"
FLOORTEX = "lwh/deck14floor"

CY = (RY0 + RY1) // 2
DOOR_Y, DOOR_HALF, DOOR_H = CY, 56, 128
TUBE_Y, TUBE_HALF, TUBE_H, TUBE_LEN = RY0 + 320, 32, 64, 288
TUBE_DEST = "tour/deck13"          # the life-support plant above; the tube is the route down

# The stasis pod row: the game's own pod, along the north wall -- the brief's "machinery wall".
POD_MODEL = "models/mapobjects/stasis/pod.md3"
POD_MINS = (-32, -19, -48)         # from the shipped stasis maps' own placements
POD_MAXS = (32, 19, 48)
POD_Y = RY1 - 40
POD_Z = FLOOR + 48                  # the model's origin is its middle, so this sets it on the floor
POD_N = 8
POD_X0, POD_X1 = RX0 + 448, RX1 - 448

# The holodeck doorframe: at the far (west) end of the pod wall, so the walk runs past the pods to
# the holodeck. On the north wall, where the copied room's interior is clear.
HOLO_X = RX0 + 256
HOLO_W, HOLO_H = 224, 192

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
            "lights_kept": 0, "waypoints": 0, "pods": [], "holodeck": None,
            "census": census(stitch.parse(deck11_path)),
        }
        self.world = []
        self.entities = []
        self.solids = Solids()
        self._copy_room(stitch.parse(deck11_path))
        self._shell()
        self._door_and_lobby()
        self._tube()
        self._pods()
        self._holodeck()
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
                self._copy_brushes(brushes, tops, world=True)
                continue
            if cls == "light":
                # Deck 11's own ceiling lights light its copied structure; keep the ones inside the
                # room, but dim them so the authored cold and warm zones read on top. A light left
                # outside the sealed room is an entity in the void and a leak, so the crop still applies.
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
                    k["light"] = str(max(40, int(float(k.get("light", "300")) * 0.7)))
                except ValueError:
                    pass
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

    def _copy_brushes(self, brushes, tops, world):
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
            # the chamber, ringed by consoles. The brief wants one flat chamber, so the *large* raised
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
            self.report["brushes_kept"] += 1
            (self.world if world else kept).append(lines)
            self.solids.add(lines)
        return kept

    # -- new construction -------------------------------------------------------------------------
    def w(self, lines):
        lines = aslines(lines)
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
        self.named_brush("func_door", "lwh_d14_door", "cargo/cargodoor1",
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
        # the chamber's east face, closed around the tube opening (or the chamber is open to the void)
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
        self.point("target_level_change", "lwh_tube_d14", (ex0 + 48, TUBE_Y, FLOOR + 24),
                   {"mapname": TUBE_DEST})
        self.point("info_notnull", "lwh_tube_mouth", (RX0 - 48, TUBE_Y, FLOOR + 24))

    def _pods(self):
        # The brief's "banks of stasis pods": a row of the game's own pod model along the north wall,
        # from the west end to the east -- the machinery wall the brief asks for. Each pod stands on a
        # small pedestal at its own local floor (the copied room's surface rises and falls), so no pod
        # floats or is buried -- deck 12's plant vessels are placed the same way.
        n = POD_N
        span = POD_X1 - POD_X0
        for i in range(n):
            px = POD_X0 + (i * span) // max(1, n - 1)
            py = POD_Y
            base = self.solids.floor_at(px, py)
            self.w(brush(((px - 40, py - 32, base), (px + 40, py + 32, base + 8)), "engineering/supportedge"))
            self.model("misc_model_breakable", None, POD_MODEL, (px, py, base + 8 + 48), 180,
                       POD_MINS, POD_MAXS)
            self.report["pods"].append([px, py, base + 8 + 48])
        # A trace at the foot of the row: a toolkit left down (the game's own prop, from the copy).
        self.model("misc_model_breakable", None, "models/mapobjects/cargo/toolkit.md3",
                   ((POD_X0 + POD_X1) // 2, RY1 - 176, FLOOR), 0, (-8, -9, -1), (11, 11, 15))

    def _holodeck(self):
        # Holodeck 1: a closed doorframe on the north wall at the far end of the pod row, warm-lit.
        # The holodeck programme itself is a non-goal, so it is a frame on an intact wall, with the
        # control panel in front and the maintenance post facing it.
        wy = RY1
        x0, x1 = HOLO_X - HOLO_W // 2, HOLO_X + HOLO_W // 2
        post = 32
        # the uprights and the lintel, standing proud of the wall so the frame reads
        for lo, hi, tex in [
            ((x0, wy - post, FLOOR), (x0 + post, wy, FLOOR + HOLO_H), "voyager/voydoor3b"),
            ((x1 - post, wy - post, FLOOR), (x1, wy, FLOOR + HOLO_H), "voyager/voydoor3b"),
            ((x0, wy - post, FLOOR + HOLO_H - post), (x1, wy, FLOOR + HOLO_H), "voyager/voydoor3b"),
        ]:
            self.w(brush((lo, hi), tex))
        # the closed door itself, recessed between the uprights: a glowing panel, no room behind it
        self.w(brush(((x0 + post + 2, wy - 6, FLOOR), (x1 - post - 2, wy - 2, FLOOR + HOLO_H - post)), "engineering/elight1"))
        # the glow strips down the uprights (the brief's "the holodeck arch glows")
        for gx in (x0 + 4, x1 - 12):
            self.w(brush(((gx, wy - post, FLOOR + 8), (gx + 8, wy - post + 4, FLOOR + HOLO_H - post - 8)), "engineering/holodecklights"))
        # the control panel in front of the arch, and the live surface on it
        px, py = HOLO_X + 200, wy - 288
        base = FLOOR
        self.w(brush(((px - 64, py - 48, base), (px + 64, py + 48, base + 48)), "engineering/console1"))
        self.w(brush(((px - 56, py - 40, base + 48), (px + 56, py + 40, base + 80)), "voyager/basic"))
        self.w(brush(((px - 48, py - 40, base + 80), (px + 48, py - 32, base + 120)), "lwh/panel"))
        # The maintenance post in front of the arch, facing it. The numbered SYS_HOLODECKS station
        # marker (`lwh_station_17`) is NOT here: ship_core's SystemSpec puts the holodecks system on
        # deck 6, so the station marker moved to deck 6's re-dress (dressdeck06.py) when it was built
        # -- two decks defining one name would make the stitcher rename both and StationFor would find
        # neither. Deck 14 keeps the holodeck frame and this maintenance post.
        self.point("info_notnull", "lwh_stasis_post", (HOLO_X, wy - 288, base + 24))
        self.point("waypoint_navgoal_1", "stasiswatch", (HOLO_X, wy - 288, base + 24))
        self.report["holodeck"] = [px, py, HOLO_X]

    def _lighting(self):
        # Light by function: cold and dim over the pod wall, warm at the holodeck end, a neutral wash
        # between so the room reads and no surface is left black. Deck 11's own lights are kept dimmed
        # (see _copy_room) as the baseline over its copied structure.
        cold = 0
        for x in range(POD_X0, POD_X1 + 1, 160):
            self.point("light", None, (x, POD_Y - 80, FLOOR + HEIGHT - 40),
                       {"light": "170", "_color": "0.42 0.55 0.98"})
            cold += 1
        self.report["cold_lights"] = cold
        warm = 0
        for dx in (HOLO_X - 128, HOLO_X + 128):
            for dy in (RY1 - 128, RY1 - 288):
                self.point("light", None, (dx, dy, FLOOR + HEIGHT - 48),
                           {"light": "340", "_color": "1.0 0.78 0.48"})
                warm += 1
        self.report["warm_lights"] = warm
        neutral = 0
        for x in range(RX0 + 256, RX1 - 96, 224):
            for y in range(RY0 + 224, RY1 - 160, 224):
                for z in (FLOOR + 96, FLOOR + 168):
                    if self.solids.contains((x, y, z), pad=24):
                        continue
                    self.point("light", None, (x, y, z),
                               {"light": "150", "_color": "0.90 0.93 1.0"})
                    neutral += 1
        self.report["neutral_lights"] = neutral

    def _fixtures(self):
        # Working light and the emergency red, as strips along the ceiling, interleaved, so the
        # state is visible from anywhere in the room. The red ones start off; the module swaps them.
        # The names carry a deck tag (14): decks 12 and 13 define the same module prefix, and two
        # decks defining one name would make the stitcher rename both, leaving the module nothing to
        # toggle.
        span = RX1 - RX0 - 512
        seg = span // 8
        n = 0
        for i in range(8):
            x0 = RX0 + 256 + i * seg
            x1 = x0 + seg - 32
            if i % 2 == 0:
                self.named_brush("func_usable", "lwh_light_normal_14_%d" % n, "engineering/elight1",
                                 ((x0, CY - 24, TOP - 24), (x1, CY + 24, TOP - 2)))
            else:
                self.named_brush("func_usable", "lwh_light_emergency_14_%d" % n, "hall/hall_light_red",
                                 ((x0, CY - 24, TOP - 24), (x1, CY + 24, TOP - 2)), {"spawnflags": "1"})
            n += 1
        self.report["light_strips"] = n

    def _wiring(self):
        px = RX1 - 320
        self.w(brush(((px, RY0 + WALL, FLOOR + 40), (px + WALL, RY0 + WALL + 128, FLOOR + 168)), "lwh/panel"))
        (ax, ay), aang = self._arrival()
        self.point("info_player_start", None, (ax, ay, self.solids.floor_at(ax, ay) + 24), {"angle": str(aang)})
        self.point("target_level_change", "lwh_lift_d14", (RX1 + 96, DOOR_Y, FLOOR + 24), {"mapname": "tour/deck04"})
        self.point("target_shaderremap", None, (px + 48, RY0 + WALL + 64, FLOOR + 24),
                   {"falsename": "textures/lwh/panel", "truename": "textures/lwh/borg"})

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
            if self.solids.contains((x, y, z), pad=8):
                continue
            seen.add(key)
            final.append((int(x), int(y), int(z)))
        # The holodeck end and its post are off the 128-grid: make sure the watch position has a
        # route even if the grid missed the clear strip in front of the arch.
        for x in (HOLO_X - 64, HOLO_X, HOLO_X + 64):
            for y in (RY1 - 320, RY1 - 384):
                z = self.solids.floor_at(x, y) + 24
                if not self.solids.contains((x, y, z), pad=8):
                    final.append((int(x), int(y), int(z)))
        for (x, y, z) in final:
            self.point("waypoint", None, (x, y, z))
        self.report["waypoints"] = len(final)

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
    a = ap.parse_args(argv)

    b = Build(a.deck11)
    os.makedirs(a.out, exist_ok=True)
    path = os.path.join(a.out, "deck14.map")
    world_keys = collections.OrderedDict([
        ("classname", "worldspawn"),
        ("message", "Deck 14 - Stasis and Holodeck 1 (re-dressed from tour/deck11)"),
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
    r = b.report
    print("deck14 <- %s" % a.deck11)
    print("  room %d..%d x %d..%d, height %s -> %s (x%g)"
          % (RX0, RX1, RY0, RY1, r["room"]["height_before"], HEIGHT, VSCALE))
    print("  world brushes %d, entities %d, waypoints %d, core dropped %d, platforms dropped %d, "
          "patches dropped %d, lights kept %d, pods %d"
          % (len(b.world), len(b.entities), r["waypoints"], r["core_brushes_dropped"],
             r["platform_brushes_dropped"], r["patches_dropped"], r["lights_kept"], len(r["pods"])))
    print("  wrote %s" % path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
