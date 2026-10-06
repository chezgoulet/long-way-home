#!/usr/bin/env python3
"""Re-dress deck 12 -- environmental control -- from `tour/deck11`, main engineering.

    dressdeck12.py --deck11 build/gdk/maps/.../deck11.map --out build/ship/generated

The brief (docs/locations/deck12-environmental-control.brief.md) names the reuse target: **copy
`tour/deck11`**, the densest systems room in the deck set, and change what makes this room *this*
room. This tool is that copy. It reads the published deck11 source and writes `deck12.map`:

  * copies the *main engineering room* -- the dense cluster of detail brushes, with the room's own
    consoles, railings, lighting fixtures, wall panels, door hardware and props, "including any small
    untidiness in the original";
  * **reduces the vertical scale** (the room is compressed toward its floor);
  * **swaps the warp core for an atmosphere plant** (the core cluster is dropped; the plant stands on
    the raised deck, behind a rail);
  * adds a **Jefferies tube exit** on the side opposite the main door, low and awkward, with a
    traversal to deck 11 -- the route that works when the turbolifts are down;
  * moves the **watch console** to where the room can be entered, with sightlines to the door and the
    plant, and puts the life-support station marker at it;
  * keeps the deck's wiring (the station marker, section 42 sealed next door, the breach triggers and
    field, the status panel, the lift edge) and the navigation furniture;
  * adds the **emergency lighting state**: red strips (`hall/hall_light_red`, a shader the game
    already has) held off until the module switches them on, and the normal strips they replace.

Every texture and model this tool emits already exists in the shipped game: the parts list is a
census over the source it copies (--census). No new art.

Writes only under the output directory. The stitcher (tools/shipmap/stitch.py) then treats deck12.map
like any other deck source.
"""

import argparse
import collections
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "mapgen"))
sys.path.insert(0, HERE)
from mapgen import brush  # noqa: E402
import stitch  # noqa: E402

# ---- the room, and how it is changed -------------------------------------------------------------
# Main engineering's room, from the deck source itself: the detail brushes (max horizontal extent
# <= 300) have centres x -4012..-2730, y -3972..-2814. Snapped to a 16-grid and padded. Everything
# outside is the deck's corridors: not this room, not copied.
RX0, RX1 = -4096, -2704
RY0, RY1 = -4032, -2784
FLOOR = -3996            # the room's lower walkway; z compression keeps this fixed
HEIGHT = 192             # the room after re-dress: deck 11's ~244 compressed to this
VSCALE = 0.78            # the reduction in vertical scale
TOP = FLOOR + HEIGHT
ZHI = FLOOR + (HEIGHT + 64) / VSCALE
WALL = 16
WALLTEX, WALL0, WALL1 = "lwh/deck12wall2", "lwh/deck12wall0", "lwh/deck12wall1"
FLOORTEX = "lwh/deck12floor"

CY = (RY0 + RY1) // 2
DOOR_Y, DOOR_HALF, DOOR_H = CY, 56, 128
TUBE_Y, TUBE_HALF, TUBE_H, TUBE_LEN = RY0 + 320, 32, 64, 288
S42_X1, S42_Y0 = RX0 + 224, RY1 - 224

KEEP_CLASSES = {
    "light", "waypoint", "waypoint_small",
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
            "patches_dropped": 0, "core_brushes_dropped": 0, "waypoints": 0, "breach_placed": True,
            "census": census(stitch.parse(deck11_path)),
        }
        self.world = []
        self.entities = []
        self.solids = Solids()
        self._copy_room(stitch.parse(deck11_path))
        self._section_42()
        self._shell()
        self._door_and_lobby()
        self._tube()
        self._plant()
        self._console()
        self._fixtures()
        self._wiring()
        self._breach()
        self._waypoints()

    # -- the copy ---------------------------------------------------------------------------------
    def _copy_room(self, ents):
        tops = []
        for keys, brushes in ents:
            cls = keys.get("classname", "")
            if cls in ("worldspawn", "func_group"):
                self._copy_brushes(brushes, tops, world=True)
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

    def _section_42(self):
        self.w(brush(((S42_X1 - WALL, S42_Y0, FLOOR), (S42_X1, RY1, TOP + WALL)), WALLTEX))
        self.w(brush(((RX0, S42_Y0, FLOOR), (S42_X1, S42_Y0 + WALL, TOP + WALL)), WALLTEX))

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
        self.named_brush("func_door", "lwh_d12_door", "cargo/cargodoor1",
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
        self.point("target_level_change", "lwh_tube_d12", (ex0 + 48, TUBE_Y, FLOOR + 24),
                   {"mapname": "tour/deck11"})
        self.point("info_notnull", "lwh_tube_mouth", (RX0 - 48, TUBE_Y, FLOOR + 24))

    def _plant(self):
        # The plant stands on the room's own raised deck where there is one, otherwise on the
        # walkway. Each vessel is a vessel at the local floor, so nothing floats or is buried.
        self.report["plant"] = []
        for i, vx in enumerate((RX0 + 160, RX0 + 320, RX0 + 480)):
            vy = CY + (-160 if i % 2 == 0 else 160)
            base = self.solids.floor_at(vx, vy)
            self.w(brush(((vx - 48, vy - 48, base), (vx + 48, vy + 48, TOP - 16)), "engineering/chrome"))
            self.w(brush(((vx - 40, vy - 40, base + 48), (vx + 40, vy + 40, base + 80)), "engineering/glass1"))
            self.w(brush(((vx - 16, vy - 48, base), (vx + 16, vy + 48, base + 8)), "engineering/beamsides"))
            self.report["plant"].append([vx, vy, base])
        # The rail between the walkway and the plant: posts at the local floor, a top rail.
        rail_x = RX0 + 544
        for ry in range(RY0 + 128, RY1 - 96, 128):
            b = self.solids.floor_at(rail_x, ry)
            self.w(brush(((rail_x, ry, b), (rail_x + 8, ry + 8, b + 48)), "engineering/railsupports"))
        self.w(brush(((rail_x, RY0 + 128, FLOOR + 88), (rail_x + 8, RY1 - 96, FLOOR + 96)), "engineering/handrail"))

    def _console(self):
        import math
        _, (ox, oy), ba = self._open_spot()
        if ox is None:
            ox, oy, ba = RX1 - 320, CY, 180
        base = self.solids.floor_at(ox, oy)
        self.point("info_notnull", "lwh_station_0", (ox, oy, base + 24))
        cx = ox + int(math.cos(math.radians(ba + 90)) * 96)
        cy = oy + int(math.sin(math.radians(ba + 90)) * 96)
        cb = self.solids.floor_at(cx, cy)
        x0, x1 = cx - 64, cx + 64
        y0, y1 = cy - 48, cy + 48
        self.w(brush(((x0, y0, cb), (x1, y1, cb + 48)), "engineering/console1"))
        self.w(brush(((x0 + 16, y0 + 8, cb + 48), (x1 - 16, y1 - 8, cb + 80)), "voyager/basic"))
        self.w(brush(((x0 + 24, y0 + 12, cb + 80), (x1 - 24, y1 - 12, cb + 112)), "lwh/panel"))

    def _fixtures(self):
        # Working light and the emergency red, as strips along the ceiling, interleaved, so the
        # state is visible from anywhere in the room. The red ones start off; the module swaps them.
        span = RX1 - RX0 - 512
        seg = span // 8
        n = 0
        for i in range(8):
            x0 = RX0 + 256 + i * seg
            x1 = x0 + seg - 32
            if i % 2 == 0:
                self.named_brush("func_usable", "lwh_light_normal_%d" % n, "engineering/elight1",
                                 ((x0, CY - 24, TOP - 24), (x1, CY + 24, TOP - 2)))
            else:
                self.named_brush("func_usable", "lwh_light_emergency_%d" % n, "hall/hall_light_red",
                                 ((x0, CY - 24, TOP - 24), (x1, CY + 24, TOP - 2)), {"spawnflags": "1"})
            n += 1
        self.report["light_strips"] = n

    def _wiring(self):
        px = RX0 + 704
        self.w(brush(((px, CY - 64, FLOOR + 40), (px + WALL, CY + 64, FLOOR + 168)), "lwh/panel"))
        (ax, ay), aang = self._arrival()
        self.point("info_player_start", None, (ax, ay, self.solids.floor_at(ax, ay) + 24), {"angle": str(aang)})
        self.point("target_level_change", "lwh_lift_d12", (RX1 + 96, DOOR_Y, FLOOR + 24), {"mapname": "tour/deck04"})
        self.point("target_shaderremap", None, (px + 48, CY, FLOOR + 24),
                   {"falsename": "textures/lwh/panel", "truename": "textures/lwh/borg"})

    def _breach(self):
        spot = self._find_open(RX1 - 750, CY + 320)
        if not spot:
            self.report["breach_placed"] = False
            return
        bx, by = spot
        z = self.solids.floor_at(bx, by)
        self.point("info_notnull", "lwh_breach", (bx, by, z + 24))
        # Proportions are the blockout's: a short push box aimed at a hole a person's hop away, so the
        # shove is survivable and the body keeps its horizontal speed into the sample.
        self.named_brush("trigger_push", "lwh_breach_push", "common/trigger",
                         ((bx - 60, by - 60, z), (bx + 60, by + 60, z + 96)),
                         {"spawnflags": "128", "target": "lwh_breach_hole"})
        self.named_brush("trigger_hurt", "lwh_breach_hurt", "common/trigger",
                         ((bx - 260, by - 140, z), (bx + 40, by + 140, z + HEIGHT)),
                         {"spawnflags": "128", "dmg": "10", "wait": "0.5"})
        self.named_brush("func_usable", "lwh_breach_field", "voyager/field_activation3",
                         ((bx + 124, by - 120, z), (bx + 132, by + 120, z + HEIGHT)), {"spawnflags": "1"})
        self.point("target_position", "lwh_breach_hole", (bx + 132, by, z + 100))

    def _waypoints(self):
        deck11 = []
        for k, _ in self.entities:
            if k.get("classname") in ("waypoint", "waypoint_small") and "origin" in k:
                a = k["origin"].split()
                deck11.append((float(a[0]), float(a[1])))
        self.entities = [(k, b) for k, b in self.entities
                         if k.get("classname") not in ("waypoint", "waypoint_small")]
        cand = deck11 + [(x, y) for x in range(RX0 + 128, RX1 - 64, 128)
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
        for (x, y, z) in final:
            self.point("waypoint", None, (x, y, z))
        self.report["waypoints"] = len(final)

    def _find_open(self, cx, cy):
        for r in range(0, 1800, 64):
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
        import math
        dx, dy = math.cos(math.radians(ang)), math.sin(math.radians(ang))
        d = 0
        while d < maxlen:
            d += 16
            if self.solids.contains((x + dx * d, y + dy * d, z), pad=6):
                return d
        return maxlen

    def _open_spot(self):
        """The roomiest spot in the room, and the direction with the longest sightline from it."""
        best = (-1, (None, None), 0)
        for x in range(RX0 + 320, RX1 - 192, 128):
            for y in range(RY0 + 256, RY1 - 192, 128):
                f = self.solids.floor_at(x, y)
                if f > FLOOR + HEIGHT - 72:
                    continue
                z = f + 40
                if self.solids.contains((x, y, z), pad=16):
                    continue
                d, ba = 0, 0
                for ang in range(0, 360, 45):
                    dd = self._clear_len(x, y, z, ang, 384)
                    if dd > d:
                        d, ba = dd, ang
                if d > best[0]:
                    best = (d, (x, y), ba)
        return best

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
    path = os.path.join(a.out, "deck12.map")
    world_keys = collections.OrderedDict([
        ("classname", "worldspawn"),
        ("message", "Deck 12 - Environmental Control (re-dressed from tour/deck11)"),
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
    print("deck12 <- %s" % a.deck11)
    print("  room %d..%d x %d..%d, height %s -> %s (x%g)"
          % (RX0, RX1, RY0, RY1, r["room"]["height_before"], HEIGHT, VSCALE))
    print("  world brushes %d, entities %d, waypoints %d, core dropped %d, patches dropped %d, breach %s"
          % (len(b.world), len(b.entities), r["waypoints"], r["core_brushes_dropped"],
             r["patches_dropped"], r["breach_placed"]))
    print("  wrote %s" % path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
