#!/usr/bin/env python3
"""Re-dress deck 6 -- Holodeck 2, the armory and the crew quarters -- as a *composition*.

    dressdeck06.py --quarters build/gdk/maps/.../deck09.map --armory build/gdk/maps/brig-map/_brig.map \
                   --holodeck build/gdk/maps/.../Tour/_holodeck_firingrange.map \
                   --out build/ship/generated

The brief (docs/locations/deck06-holodecks.brief.md, section 3) is not a single copy target: deck 6
carries **three functions on one deck**, so its parts come from **three maps**, and the commit message
names which zone came from which. This is the fifth and last re-dress, and the only one that
*composes* rather than crops:

  * **the massing and the corridor spine -- `tour/deck09`** (the crew-quarters deck): a slice of the
    hall is set along the north side of the deck as the quarters frontage -- the hall-and-alcoves read
    the spatial program asks for, so the corridor stays the *route* and not the room;
  * **the armory and the security side -- `_brig`**: the only security interior in the game, and the
    map that carries the brig; its room is set into an alcove on the south-west, so the armory borrows
    its lockers, grating and lighting;
  * **the holodeck doorframes -- a `_holodeck_*` programme map** (`Tour/_holodeck_firingrange`): the
    arch read and the warm light, twice, on the south-east wall -- a frame, not a working programme,
    which is the brief's own non-goal.

**Copy nothing exactly: three sources, composited.** The failure to beat, in deck 14's own words, is
that a copied interior can dominate the room it was copied into; this deck has three inside one
corridor. The corridor is the route; the lighting brief is the instrument -- warm where people live
(the quarters and the holodeck), working light at the armory, the holodeck arch glowing.

**The copied interiors are detail geometry** (deck 7's mechanism): the three zones are emitted through
generated per-material shader aliases (`lwh/detail06/...`, same images plus `surfaceparm detail`), so
they do not split the BSP tree and the fifth re-dress does not pass q3map2's vis-cluster ceiling. The
shell, the door and the tube stay structural and seal the room.

It keeps the deck wiring (the lift edge, the status panel, the navigation furniture) and adds the
**holodeck station marker** (`lwh_station_17`, SYS_HOLODECKS), the deck's **holodeck post** and its
**armory post** with their navgoals.

Every texture and model this tool emits already exists in the shipped game: the parts list is a census
over the three sources it copies (--census). No new art.

Writes only under the output directory. The stitcher (tools/shipmap/stitch.py) then treats deck06.map
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

# ---- the envelope, and how it is composed --------------------------------------------------------
# Deck 6 shares the footprint the re-dress decks sit in (x -4096..-2704, y -4032..-2784) widened
# along x to give the corridor its length. It is one hall, 1,536 x 1,024, compressed to 224 units
# tall: taller than deck 12's control room (192) and the same as deck 13's plant hall (224), because
# this deck has to carry a holodeck arch (a 192-unit portal) and still have headroom over it.
RX0, RX1 = -4608, -3072
RY0, RY1 = -4096, -3072
FLOOR = -3996
HEIGHT = 224
TOP = FLOOR + HEIGHT
WALL = 16
WALLTEX, WALL0, WALL1 = "lwh/deck06wall2", "lwh/deck06wall0", "lwh/deck06wall1"
FLOORTEX = "lwh/deck06floor"
# Deck-6 detail shaders: aliases of the sources' own materials with surfaceparm detail. The copied
# interiors must be detail or the fifth re-dress pushes the ship past q3map2's vis-cluster ceiling;
# the aliases are emitted by --detail-shader (tools/shipmap/data is not touched).
DETAIL_PREFIX = "lwh/detail06/"

CY = (RY0 + RY1) // 2
DOOR_Y, DOOR_HALF, DOOR_H = CY, 56, 128
TUBE_Y, TUBE_HALF, TUBE_H, TUBE_LEN = CY, 32, 64, 288
TUBE_DEST = "tour/deck05"          # sickbay, the deck below; the tube is the fallback route

FLOOR_MIN_AREA = 96 * 96

# ---- the three zones, from the three sources -----------------------------------------------------
# Each zone is a rectangular slice of its source, carried into a destination box at true scale (a
# pure translate, so nothing is stretched except z). `src_floor` is the source's walkable floor
# (measured from the source's own player start), and the slice is compressed to `height`.
#
# Q -- the quarters frontage, from tour/deck09 (the hall alongside the crew quarters), along the
#      north side. s: x -3788..-2700, y -3300..-2964 (a 1,088 x 336 slice of the hall), floor -2912.
# A -- the armory, from _brig (the security interior), in a south-west alcove. s: the room around the
#      brig's own player start, x -320..192, y -460..-124 (512 x 336), floor 0.
# H -- the holodeck doorframe, from Tour/_holodeck_firingrange (the programme's own arch), twice, on
#      the south-east wall. s: the arch cluster x 32..152, y -96..112 (120 x 208), floor 0.
Z_QUARTERS = {
    "name": "quarters", "src": "deck09",
    "sx0": -3788, "sx1": -2700, "sy0": -3300, "sy1": -2964,
    "src_floor": -2912.0, "src_top": -2650.0, "height": 200,
    "dx0": -4592, "dy0": -3424, "rot": 0,
}
Z_ARMORY = {
    "name": "armory", "src": "brig",
    "sx0": -320, "sx1": 192, "sy0": -460, "sy1": -124,
    "src_floor": 0.0, "src_top": 200.0, "height": 192,
    "dx0": -4592, "dy0": -4080, "rot": 0,
}
Z_HOLODECK = {
    "name": "holodeck", "src": "holodeck",
    "sx0": 32, "sx1": 152, "sy0": -96, "sy1": 112,
    "src_floor": 0.0, "src_top": 384.0, "height": 208,
    "dx0": -3900, "dy0": -4060, "rot": 90,     # rotated: the doorway faces the corridor (north)
}
HOLO_COPY_DX = 400                              # the second arch, a pace down the corridor

# Classes the copy keeps from a zone: its geometry-carrying furniture and its own lights. Its
# scripts, triggers, NPCs, player starts, turbolift network and pickup are its business, not this
# deck's, and are dropped. Names are tagged (lwh_d06_) so a zone name cannot collide with the ship.
KEEP_CLASSES = {
    "light", "func_static", "func_usable", "misc_model_breakable",
}
# A holodeck programme's door leaves come as a lit panel ("panelon") and an unlit one ("paneloff")
# in the same place; with no programme running both would render and z-fight, so the unlit leaf is
# dropped and the arch keeps the lit one.
ZONE_DROP_NAMES = {"paneloff"}


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


class Zone:
    """One source slice and where it lands. A pure translate (plus an optional 90-degree turn),
    with z compressed toward the destination floor so the slice fits the deck's height."""

    def __init__(self, spec, src_floor):
        self.__dict__.update(spec)
        self.src_floor = src_floor
        self.cx_src = (self.sx0 + self.sx1) / 2.0
        self.cy_src = (self.sy0 + self.sy1) / 2.0
        self.w = self.sx1 - self.sx0
        self.d = self.sy1 - self.sy0
        # after a quarter-turn the footprint's x and y extents swap
        if self.rot in (90, 270):
            self.dw, self.dd = self.d, self.w
        else:
            self.dw, self.dd = self.w, self.d
        self.cx_dst = self.dx0 + self.dw / 2.0
        self.cy_dst = self.dy0 + self.dd / 2.0
        self.zscale = self.height / (self.src_top - self.src_floor)
        # Keep a brush whose centre falls in the slice and whose z band overlaps the slice's: the
        # slice's own boundary walls run past it in x or y and are clamped in, exactly as the re-dress
        # crop clamps a room's walls into its box.
        self.zlo, self.zhi = self.src_floor - 128, self.src_top + 128

    def in_slice(self, b):
        cx, cy = (b.lo[0] + b.hi[0]) / 2.0, (b.lo[1] + b.hi[1]) / 2.0
        return (self.sx0 - 8 <= cx <= self.sx1 + 8 and self.sy0 - 8 <= cy <= self.sy1 + 8
                and b.zmax >= self.zlo and b.zmin <= self.zhi)

    def map_point(self, x, y, z):
        u, v = x - self.cx_src, y - self.cy_src
        if self.rot == 90:
            a, b = -v, u
        elif self.rot == 180:
            a, b = -u, -v
        elif self.rot == 270:
            a, b = v, -u
        else:
            a, b = u, v
        X = self.cx_dst + a
        Y = self.cy_dst + b
        X = min(max(X, self.dx0), self.dx0 + self.dw)
        Y = min(max(Y, self.dy0), self.dy0 + self.dd)
        Z = FLOOR + (z - self.src_floor) * self.zscale
        Z = min(max(Z, FLOOR), TOP)
        return X, Y, Z

    def line(self, ln):
        if not ln.startswith("("):
            return ln
        n = [0]

        def rep(m):
            n[0] += 1
            if n[0] > 3:
                return m.group(0)
            X, Y, Z = self.map_point(float(m.group(1)), float(m.group(2)), float(m.group(3)))
            return "( %s %s %s )" % (fmt(X), fmt(Y), fmt(Z))
        return stitch.POINT.sub(rep, ln)

    def origin(self, v):
        try:
            x, y, z = (float(t) for t in v.split())
        except ValueError:
            return v
        X, Y, Z = self.map_point(x, y, z)
        Z = min(max(Z, FLOOR + 8), TOP - 8)
        return "%s %s %s" % (fmt(X), fmt(Y), fmt(Z))


class Solids:
    def __init__(self):
        self.boxes = []

    def add(self, lines):
        lo, hi = [1e9] * 3, [-1e9] * 3
        for ln in lines:
            if not ln.startswith("("):
                continue
            for m in stitch.POINT.finditer(ln):
                for i in range(3):
                    lo[i] = min(lo[i], float(m.group(i + 1)))
                    hi[i] = max(hi[i], float(m.group(i + 1)))
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
        best = FLOOR
        for lo, hi in self.boxes:
            if not (lo[0] - 8 <= x <= hi[0] + 8 and lo[1] - 8 <= y <= hi[1] + 8):
                continue
            if hi[2] <= FLOOR + HEIGHT - 64 and (hi[0] - lo[0]) * (hi[1] - lo[1]) >= FLOOR_MIN_AREA \
                    and hi[2] > best:
                best = hi[2]
        return best


def brush_tex(lines):
    for ln in lines:
        if ln.startswith("("):
            m = stitch.FACE_TEXTURE.match(ln)
            if m:
                return m.group(2).lower()
    return ""


def zone_census(ents):
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
    return models, texs, classes


def merge_census(paths):
    models, texs, classes = collections.Counter(), collections.Counter(), collections.Counter()
    for p in paths.values():
        m, t, c = zone_census(stitch.parse(p))
        models.update(m)
        texs.update(t)
        classes.update(c)
    return {"models": dict(models.most_common()), "textures": dict(texs.most_common()),
            "classes": dict(classes.most_common())}


class Build:
    def __init__(self, paths):
        self.paths = paths
        self.report = {
            "copied_from": {k: os.path.basename(v) for k, v in paths.items()},
            "envelope": {"x": [RX0, RX1], "y": [RY0, RY1], "floor": FLOOR, "height": HEIGHT},
            "zones": {}, "entities_kept": 0, "entities_dropped": 0, "brushes_kept": 0,
            "brushes_dropped": 0, "lights_kept": 0, "waypoints": 0,
            "census": merge_census(paths),
        }
        self.world = []
        self.entities = []
        self.detail_textures = set()
        self.solids = Solids()
        self._zone("quarters", Z_QUARTERS, "deck09")
        self._zone("armory", Z_ARMORY, "brig")
        self._arch(Z_HOLODECK, 0, "holodeck")
        self._arch(Z_HOLODECK, HOLO_COPY_DX, "holodeck2")
        self._shell()
        self._door_and_lobby()
        self._tube()
        self._holodeck_console()
        self._armory_post()
        self._lighting()
        self._fixtures()
        self._wiring()
        self._waypoints()

    # -- the composition ---------------------------------------------------------------------------
    def _zone(self, name, spec, key):
        z = Zone(spec, spec["src_floor"])
        kept = dropped = 0
        for keys, brushes in stitch.parse(self.paths[key]):
            cls = keys.get("classname", "")
            if cls in ("worldspawn", "func_group"):
                for b in brushes:
                    if not z.in_slice(b):
                        dropped += 1
                        continue
                    self._emit(z, b, world=True)
                    kept += 1
                continue
            if cls == "light":
                try:
                    ox, oy, oz = (float(t) for t in keys["origin"].split())
                except (KeyError, ValueError):
                    self.report["entities_dropped"] += 1
                    continue
                if not (z.sx0 - 64 <= ox <= z.sx1 + 64 and z.sy0 - 64 <= oy <= z.sy1 + 64
                        and z.zlo - 64 <= oz <= z.zhi + 64):
                    self.report["entities_dropped"] += 1
                    continue
                k = collections.OrderedDict(keys)
                k["origin"] = z.origin(keys["origin"])
                self.entities.append((k, []))
                self.report["lights_kept"] += 1
                continue
            if cls not in KEEP_CLASSES or keys.get("targetname", "") in ZONE_DROP_NAMES:
                self.report["entities_dropped"] += 1
                continue
            body = []
            for b in brushes:
                if not z.in_slice(b):
                    continue
                lines = self._emit(z, b, world=False)
                if lines:
                    body.append(lines)
            if brushes and not body:
                self.report["entities_dropped"] += 1
                continue
            if not brushes:
                # A point entity (a prop, a light): only if it stands inside the slice. Without this
                # a prop with no brushes is carried from anywhere in the source map.
                try:
                    ox, oy, oz = (float(t) for t in keys["origin"].split())
                except (KeyError, ValueError):
                    self.report["entities_dropped"] += 1
                    continue
                if not (z.sx0 - 64 <= ox <= z.sx1 + 64 and z.sy0 - 64 <= oy <= z.sy1 + 64
                        and z.zlo - 64 <= oz <= z.zhi + 64):
                    self.report["entities_dropped"] += 1
                    continue
            k = collections.OrderedDict(keys)
            if k.get("targetname"):
                k["targetname"] = "lwh_d06_" + k["targetname"]
            if "origin" in k:
                k["origin"] = z.origin(k["origin"])
            self.entities.append((k, body))
            self.report["entities_kept"] += 1
        self.report["zones"][name] = {
            "source": os.path.basename(self.paths[key]), "slice": [spec["sx0"], spec["sx1"], spec["sy0"], spec["sy1"]],
            "dest": [spec["dx0"], spec["dy0"]], "rot": spec["rot"], "height": spec["height"],
            "brushes": kept, "brushes_dropped": dropped,
        }

    def _emit(self, z, b, world):
        if b.is_patch:
            # A patch (a light or a trim curve) does not survive the slice's clamp cleanly, and the
            # zones are furniture, not ceilings: drop them.
            return None
        lines = self._as_detail([z.line(ln) for ln in b.lines])
        if world:
            self.world.append(lines)
        self.report["brushes_kept"] += 1
        self.solids.add(lines)
        return lines

    def _arch(self, spec, extra_dx, name):
        spec = dict(spec)
        spec["dx0"] = spec["dx0"] + extra_dx
        self._zone(name, spec, "holodeck")

    # -- detail geometry --------------------------------------------------------------------------
    def _as_detail(self, lines):
        out = []
        for ln in lines:
            if ln.startswith("("):
                m = stitch.FACE_TEXTURE.match(ln)
                if m:
                    tex = m.group(2)
                    if not (tex.startswith("lwh/") or tex.startswith("jefferies/")
                            or tex.startswith("common/") or tex == "cargo/cargodoor1"
                            or tex.startswith("hall/hall_light_red")):
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
        keys["origin"] = "%d %d %d" % tuple(int(round(v)) for v in origin)
        self.entities.append((keys, []))

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
        self.named_brush("func_door", "lwh_d06_door", "cargo/cargodoor1",
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
        self.point("target_level_change", "lwh_tube_d06", (ex0 + 48, TUBE_Y, FLOOR + 24),
                   {"mapname": TUBE_DEST})
        self.point("info_notnull", "lwh_tube_mouth", (RX0 - 48, TUBE_Y, FLOOR + 24))

    def _holodeck_console(self):
        # The holodeck control surface and its station, at the first arch: the arch is the brief's
        # control surface (recreation/training/therapy); the numbered SYS_HOLODECKS station marker
        # stands at the panel, and a maintenance post faces the arch.
        ax = Z_HOLODECK["dx0"] + (Z_HOLODECK["sy1"] - Z_HOLODECK["sy0"]) / 2.0
        ay = -3872
        px, py = ax + 240, ay
        self.w(brush(((px - 64, py - 48, FLOOR), (px + 64, py + 48, FLOOR + 48)), "engineering/console1"), detail=True)
        self.w(brush(((px - 56, py - 40, FLOOR + 48), (px + 56, py + 40, FLOOR + 80)), "voyager/basic"), detail=True)
        self.w(brush(((px - 48, py - 40, FLOOR + 80), (px + 48, py - 32, FLOOR + 120)), "lwh/panel"), detail=True)
        self.point("info_notnull", "lwh_station_17", (px, py + 96, FLOOR + 24))
        hx, hy = ax, -3968
        self.point("info_notnull", "lwh_holo_post", (hx, hy, FLOOR + 24))
        self.point("waypoint_navgoal_1", "holowatch", (hx, hy, FLOOR + 24))
        self.report["holodeck"] = [ax, ay, px, py, hx, hy]

    def _armory_post(self):
        # The security post by the armory: where the security party draws its weapons, on the armory
        # floor, measured clear of the copied lockers.
        ax, ay = -4336, -3904
        self.point("info_notnull", "lwh_armory_post", (ax, ay, FLOOR + 24))
        self.point("waypoint_navgoal_1", "armorywatch", (ax, ay, FLOOR + 24))
        self.report["armory_post"] = [ax, ay]

    def _lighting(self):
        # Light by function: warm where people live (the quarters frontage and the holodeck arch),
        # working neutral light at the armory and down the corridor -- the instrument the brief names
        # to keep three interiors from reading as one corridor with three props in it.
        warm = 0
        for x in range(RX0 + 256, RX1 - 128, 320):
            self.point("light", None, (x, RY1 - 160, TOP - 48),
                       {"light": "300", "_color": "1.0 0.82 0.55"})
            warm += 1
        for dx in (-160, 0, 160):
            self.point("light", None, (Z_HOLODECK["dx0"] + 104 + dx, -3904, TOP - 56),
                       {"light": "360", "_color": "1.0 0.78 0.48"})
            warm += 1
        self.report["warm_lights"] = warm
        work = 0
        for x in range(RX0 + 224, RX1 - 96, 256):
            self.point("light", None, (x, CY, TOP - 56), {"light": "260", "_color": "0.95 0.97 1.0"})
            work += 1
        self.report["working_lights"] = work
        cool = 0
        for x in range(-4496, -4096, 200):
            for y in (-4016, -3840):
                self.point("light", None, (x, y, TOP - 72), {"light": "300", "_color": "0.86 0.90 1.0"})
                cool += 1
        self.report["armory_lights"] = cool

    def _fixtures(self):
        # Working light and the emergency red, as strips along the ceiling, interleaved, so the
        # state is visible from anywhere on the deck. The red ones start off; the module swaps them.
        # The names carry a deck tag (06): the other re-dress decks define the same module prefix, and
        # two decks defining one name would make the stitcher rename both, leaving it nothing to toggle.
        span = RX1 - RX0 - 512
        seg = span // 8
        n = 0
        for i in range(8):
            x0 = RX0 + 256 + i * seg
            x1 = x0 + seg - 32
            if i % 2 == 0:
                self.named_brush("func_usable", "lwh_light_normal_06_%d" % n, "engineering/elight1",
                                 ((x0, CY - 24, TOP - 24), (x1, CY + 24, TOP - 2)))
            else:
                self.named_brush("func_usable", "lwh_light_emergency_06_%d" % n, "hall/hall_light_red",
                                 ((x0, CY - 24, TOP - 24), (x1, CY + 24, TOP - 2)), {"spawnflags": "1"})
            n += 1
        self.report["light_strips"] = n

    def _wiring(self):
        px = RX1 - 320
        self.w(brush(((px, RY0 + WALL, FLOOR + 40), (px + WALL, RY0 + WALL + 128, FLOOR + 168)), "lwh/panel"), detail=True)
        (ax, ay), aang = self._arrival()
        self.point("info_player_start", None, (ax, ay, self.solids.floor_at(ax, ay) + 24), {"angle": str(aang)})
        self.point("target_level_change", "lwh_lift_d06", (RX1 + 96, DOOR_Y, FLOOR + 24), {"mapname": "tour/deck04"})
        self.point("target_shaderremap", None, (px + 48, RY0 + WALL + 64, FLOOR + 24),
                   {"falsename": "textures/lwh/panel", "truename": "textures/lwh/borg"})

    def _blocked(self, x, y, z, hx=15.0, hy=15.0, zlo=-22.0, zhi=30.0):
        lo = (x - hx, y - hy, z + zlo)
        hi = (x + hx, y + hy, z + zhi)
        for blo, bhi in self.solids.boxes:
            if (lo[0] <= bhi[0] and hi[0] >= blo[0] and lo[1] <= bhi[1] and hi[1] >= blo[1]
                    and lo[2] <= bhi[2] and hi[2] >= blo[2]):
                return True
        return False

    def _waypoints(self):
        cand = [(x, y) for x in range(RX0 + 128, RX1 - 64, 128)
                for y in range(RY0 + 128, RY1 - 64, 128)]
        seen, final = set(), []
        for (x, y) in cand:
            key = (round(x / 64), round(y / 64))
            if key in seen:
                continue
            z = self.solids.floor_at(x, y) + 24
            if self._blocked(x, y, z):
                continue
            seen.add(key)
            final.append((int(x), int(y), int(z)))
        # The posts are off the 128-grid: make sure each has a route to the grid even if the grid
        # missed the clear strip in front of it.
        holo = self.report.get("holodeck", [0, 0, 0, 0, 0, 0])
        arm = self.report.get("armory_post", [0, 0])
        for (px, py) in ((holo[4], holo[5]), (arm[0], arm[1])):
            for ox, oy in ((0, 0), (64, 0), (-64, 0), (0, 64), (0, -64)):
                x, y = int(px + ox), int(py + oy)
                if not (RX0 + 32 <= x <= RX1 - 32 and RY0 + 32 <= y <= RY1 - 32):
                    continue
                z = self.solids.floor_at(x, y) + 24
                if not self._blocked(x, y, z):
                    final.append((x, y, z))
        for (x, y, z) in final:
            self.point("waypoint", None, (x, y, z))
        self.report["waypoints"] = len(final)
        # How many entities the composition carries into the merged map (the brief asks this, against
        # the 81 the generated placeholder had).
        self.report["entity_count"] = 1 + len(self.entities)  # worldspawn + every entity

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
                if f > FLOOR + HEIGHT - 72:
                    continue
                z = f + 40
                if self.solids.contains((x, y, z), pad=16):
                    continue
                for ang in range(0, 360, 45):
                    d = self._clear_len(x, y, z, ang, 256)
                    score = d - 0.20 * (RX1 - x)
                    if score > best[0]:
                        best = (score, (x, y), ang)
        return best[1] or (RX1 - 224, DOOR_Y), best[2]


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--quarters", required=True, help="tour/deck09 (the crew-quarters deck)")
    ap.add_argument("--armory", required=True, help="_brig (the security interior)")
    ap.add_argument("--holodeck", required=True, help="a _holodeck_* programme map (the arch)")
    ap.add_argument("--out", required=True)
    ap.add_argument("--report")
    ap.add_argument("--census")
    ap.add_argument("--detail-shader", help="write the deck-6 detail shader aliases here (for the compiler and the pak)")
    a = ap.parse_args(argv)

    paths = {"deck09": a.quarters, "brig": a.armory, "holodeck": a.holodeck}
    b = Build(paths)
    os.makedirs(a.out, exist_ok=True)
    path = os.path.join(a.out, "deck06.map")
    world_keys = collections.OrderedDict([
        ("classname", "worldspawn"),
        ("message", "Deck 6 - Holodeck 2, Armory and Quarters (composed from tour/deck09, _brig, _holodeck_*)"),
        ("music", "music/elite15.mp3"),
        ("soundSet", "voyhalls"),
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
            f.write("// Generated by tools/shipmap/dressdeck06.py: deck-6 detail shaders (the composition).\n")
            f.write("// Same images as the three sources' own materials, with surfaceparm detail: the\n")
            f.write("// copied interiors must not split the BSP tree, or the fifth re-dress passes\n")
            f.write("// q3map2's vis-cluster ceiling (MAX_MAP_VISCLUSTERS, 16384). The shell, door and\n")
            f.write("// tube stay structural and seal the deck.\n")
            for tex in sorted(b.detail_textures):
                f.write("textures/%s%s\n{\n\tsurfaceparm detail\n\t{\n\t\tmap textures/%s\n\t}\n}\n"
                        % (DETAIL_PREFIX, tex, tex))
    r = b.report
    r["detail_textures"] = sorted(b.detail_textures)
    print("deck06 <- %s + %s + %s" % (a.quarters, a.armory, a.holodeck))
    print("  envelope %d..%d x %d..%d, height %d" % (RX0, RX1, RY0, RY1, HEIGHT))
    for name, z in r["zones"].items():
        print("  zone %-9s <- %-24s slice %s -> (%d %d) rot %d, %d brushes (%d dropped)"
              % (name, z["source"], z["slice"], z["dest"][0], z["dest"][1], z["rot"], z["brushes"], z["brushes_dropped"]))
    print("  world brushes %d, entities %d, waypoints %d, lights kept %d, detail materials %d"
          % (len(b.world), len(b.entities), r["waypoints"], r["lights_kept"], len(b.detail_textures)))
    print("  wrote %s" % path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
