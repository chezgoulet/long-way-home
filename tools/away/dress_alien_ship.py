#!/usr/bin/env python3
"""Re-dress the away site -- an **alien ship interior** -- from `scav3`.

    dress_alien_ship.py --source build/gdk/maps/eliteforce_scav_maps/scav3.map \
        --out build/away/alien --report build/away/alien.json --census build/away/alien-census.json

The design (`docs/outside-the-ship.md`, the archetype table, its **second** row) names the **alien
ship interior** as a re-dress of `scav1`-`5`: *unfamiliar layout, the species' own design language*.
The derelict interior (the first row) took `scav2`; this tool takes **`scav3`**, whose engineering
deck is a single 128-unit storey of green ribbed machinery, gold lamp coves and arched bulkheads --
a design language that is plainly not the player's own ship, and a room whose floor, ceiling and
machinery are complete enough to shell. The rows and the command are in
`docs/evidence/the-archetype-survey.md`.

It writes `lwh_alien_ship.map`:

  * copies the source's own brushes on one deck (floor -128), clamped to the window, dropping the
    decks below, the arena's other levels and the shipped hazards;
  * **opens two facings**: the south and east shell walls are galleries -- a continuous window band
    with mullions, a sky brush (`holodeck_temple/ctfc_sky_light2`, `env/stars`) closing each
    aperture, and a rail inside. The ship's own hull starfield is what the site looks onto. The west
    and north walls stay solid, because the ship continues there: the machinery banks stand against
    them and a compartment is not open on every side. Named in the report;
  * lights it **maintained**: gold ceiling coves and green work lamps in the source's own materials,
    so the site reads as a working alien deck rather than the derelict's failing light;
  * adds an **alien console** and a **gallery post** from the source's own materials, two working
    posts, waypoints and the arrival;
  * `--drab` keeps the light and replaces every material with a neutral grey -- the before-capture
    the archetype's palette clause must reject; `--unlit` drops the light *and* seals the galleries
    -- the before-capture the lit clause must reject; `--sealed` keeps the light and seals the
    galleries, so a check can prove its "open facing" assertion has teeth.

Every texture this tool emits already exists in the shipped game: the parts list is a census over
the source it copies (--census). No new art.
"""

import argparse
import collections
import json
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "mapgen"))
sys.path.insert(0, os.path.join(HERE, "..", "shipmap"))
from mapgen import brush  # noqa: E402
import stitch  # noqa: E402

# ---- the deck, and how it is changed -------------------------------------------------------------
# The source's own upper engineering storey: floor z -128, ceiling z 0. The window is the room
# around the machine banks (x -200..420, y 600..1000) with margin for the shell and the galleries.
X0, X1 = -224, 512
Y0, Y1 = 560, 1120
FLOOR = -128            # the deck this interior is cropped to (the source is multi-level)
ROOF = 16               # just above the source's own ceiling at 0
WALL = 16
COPY_ZMIN = FLOOR - 24  # copy only what reaches this deck; the decks below are not this site
COPY_ZMAX = ROOF + 24

# The source's own materials (`scav3`: a scavengers' ship; green ribs, gold coves, worn plate).
# No new art; the shell and the gallery are built from the source's own palette.
WALLTEX = "scavenger/m_basic"
FLOORTEX = "scavenger/m_floor3"
RIBTEX = "scavenger/m_ribs"
TRIMTEX = "scavenger/h_trim"
PANELTEX = "scavenger/k_panel6_b"
CEILTEX = "scavenger/k_ceiling_b"
LAMPTEX = "scavenger/striplight"
GREENTEX = "scavenger/striplight"
GLASSTEX = "engineering/glass_nolightmap"
# A starfield, not the outpost's debris: the alien ship is under way, and the design's own list of
# shipped skies names the star cubemaps (docs/outside-the-ship.md, section 3).
SKYTEX = "holodeck_temple/ctfc_sky_light2"
DRABTEX = "common/greysquare2"   # the neutral grey the palette clause must reject

DROP_TEX = ("common/nodrop", "common/sky_light", "common/clip", "common/trigger", "common/caulk",
            "common/blocknpc", "common/shotclip", "common/nodraw_solid", "common/areaportal",
            "common/lava", "common/kill", "common/water", "common/slime")

# The gallery: a window band at eye height, and the rail a person stands behind.
SILL = FLOOR + 16
HEADER = FLOOR + 112
RAIL_H = 40
RAIL_INSET = 40

VANTAGE_PACE = 96       # the rig's pace for this cramped interior (g_shipVantagePace)


def aslines(b):
    if isinstance(b, str):
        ls = b.split("\n")
        if ls and ls[0].strip() == "{":
            ls = ls[1:]
        if ls and ls[-1].strip() == "}":
            ls = ls[:-1]
        return ls
    return b


def brush_tex(lines):
    for ln in lines:
        if ln.startswith("("):
            m = stitch.FACE_TEXTURE.match(ln)
            if m:
                return m.group(2).lower()
    return ""


def copy_brush(b):
    """The source brush, unchanged, if it lies wholly inside the window; None otherwise.

    The derelict's tool *clamped* each face's plane points into its window. That works on a source
    of straight, axis-aligned walls; on `scav3`'s sloped and arched brushes it silently turns a
    valid six-plane brush into a five-plane one, and a five-plane brush has no interior -- every
    point in the room then tests as solid, which is how this tool first placed no waypoints at all.
    Here a crossing brush is dropped and the shell replaces it: the window is the room, the shell is
    its wall, and a brush that only *reaches* the boundary is not part of the room's own detail.
    """
    if b.is_patch:
        if not (X0 <= b.lo[0] and b.hi[0] <= X1 and Y0 <= b.lo[1] and b.hi[1] <= Y1):
            return None
        if b.hi[2] < COPY_ZMIN or b.lo[2] > COPY_ZMAX:
            return None
        return list(b.lines)
    if (b.hi[0] - b.lo[0]) > 1500 or (b.hi[1] - b.lo[1]) > 1500:
        return None                                   # the source's own sky box and ground plane
    if brush_tex(b.lines) in DROP_TEX:
        return None
    if b.hi[2] < COPY_ZMIN:
        return None                                   # below this deck: not this site
    if b.lo[2] > COPY_ZMAX:
        return None                                   # above this deck: not this site
    if not (X0 <= b.lo[0] and b.hi[0] <= X1 and Y0 <= b.lo[1] and b.hi[1] <= Y1):
        return None                                   # crosses the shell: the shell replaces it
    return list(b.lines)


class Solids:
    """Point-in-brush, not point-in-AABB.

    The derelict's tool tested a point against each brush's axis-aligned box. On a source as dense
    and as *sloped* as `scav3` -- arches, ribbed bulkheads, machinery that leans -- the boxes of
    neighbouring brushes overlap the whole room, so every candidate waypoint read as "in solid" and
    the tool placed none. This tests the brush's own planes instead: the interior is the positive
    side of every face plane (the .map's winding), so a point is solid when it is inside all of
    them. The same test gives `floor_at` a downward scan rather than a guess at the highest box.
    """

    def __init__(self):
        self.brushes = []

    def add(self, lines):
        planes = []
        points = []
        for ln in lines:
            if not ln.startswith("("):
                continue
            pts = [(float(m.group(1)), float(m.group(2)), float(m.group(3)))
                   for m in stitch.POINT.finditer(ln)]
            if len(pts) < 3:
                continue
            points.extend(pts[:3])
            p0, p1, p2 = pts[:3]
            u = (p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2])
            v = (p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2])
            n = (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])
            length = math.sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2])
            if length < 1e-9:
                continue
            n = (n[0] / length, n[1] / length, n[2] / length)
            planes.append((n, n[0] * p0[0] + n[1] * p0[1] + n[2] * p0[2]))
        if len(planes) < 4 or not points:
            return
        # The .map's winding is not guaranteed to put the solid on one side of every face; a brush
        # written by a different editor can have its planes flipped. Decide the orientation once,
        # from the brush's own point cloud: if the centroid is on the negative side of most planes,
        # flip them all, so `solid_at` is "inside every face" for every source.
        c = (sum(p[0] for p in points) / len(points),
             sum(p[1] for p in points) / len(points),
             sum(p[2] for p in points) / len(points))
        score = sum(n[0] * c[0] + n[1] * c[1] + n[2] * c[2] - d for n, d in planes)
        if score < 0:
            planes = [((-n[0], -n[1], -n[2]), -d) for n, d in planes]
        self.brushes.append(planes)

    def solid_at(self, x, y, z, eps=0.5):
        for planes in self.brushes:
            inside = True
            for n, d in planes:
                if n[0] * x + n[1] * y + n[2] * z - d < -eps:
                    inside = False
                    break
            if inside:
                return True
        return False

    def contains(self, p, pad=0.0):
        x, y, z = p
        if pad <= 0:
            return self.solid_at(x, y, z)
        for ox in (-pad, 0.0, pad):
            for oy in (-pad, 0.0, pad):
                for oz in (-pad, 0.0, pad):
                    if self.solid_at(x + ox, y + oy, z + oz):
                        return True
        return False

    def box_clear(self, x, y, z, hx=20.0, zlo=20.0, zhi=36.0):
        """A point entity's own bounding box (the engine's waypoint is -12,-12,-24 .. 12,12,32) does
        not overlap a solid. A point that is merely *near* a brush is in solid to the engine, and the
        engine aborts the load on `Waypoint ... in solid!` -- so the box is what is tested."""
        for ox in (-hx, 0.0, hx):
            for oy in (-hx, 0.0, hx):
                for oz in (-zlo, 0.0, zhi):
                    if self.solid_at(x + ox, y + oy, z + oz):
                        return False
        return True

    def floor_at(self, x, y, top=None):
        """The first solid surface below the room's head height, scanning down.

        A ceiling, a duct or a machine overhead is not the floor: the scan starts at the window
        header (below the ceiling) and returns the highest solid it meets, which is the surface a
        body stands on at that column.
        """
        z = top if top is not None else FLOOR + 96
        while z > FLOOR - 160:
            if self.solid_at(x, y, z):
                return z
            z -= 2
        return FLOOR


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
    def __init__(self, source, lit=True, galleries=True, drab=False):
        self.lit = lit
        self.galleries = galleries
        self.drab = drab
        self.report = {"copied_from": os.path.basename(source),
                       "window": {"x": [X0, X1], "y": [Y0, Y1], "floor": FLOOR, "roof": ROOF,
                                  "sill": SILL, "header": HEADER},
                       "brushes_kept": 0, "brushes_dropped": 0, "patches_dropped": 0,
                       "lights_kept": 0, "entities_dropped": 0, "waypoints": 0, "lit": lit,
                       "galleries": galleries, "drab": drab,
                       "census": census(stitch.parse(source)),
                       "parts": collections.Counter()}
        self.world = []
        self.entities = []
        self.solids = Solids()
        self._copy_deck(stitch.parse(source))
        self._shell()
        if lit:
            self._lighting()
        self._console()
        self._waypoints()
        if drab:
            self._drab()

    # -- the copy ---------------------------------------------------------------------------------
    def _copy_deck(self, ents):
        for keys, brushes in ents:
            cls = keys.get("classname", "")
            if cls in ("worldspawn", "func_group"):
                for b in brushes:
                    lines = copy_brush(b)
                    if lines is None:
                        self.report["patches_dropped" if b.is_patch else "brushes_dropped"] += 1
                        continue
                    self.report["brushes_kept"] += 1
                    self.report["parts"][brush_tex(lines) if not b.is_patch else "patch"] += 1
                    self.world.append(lines)
                    self.solids.add(lines)
                continue
            if cls == "light":
                o = keys.get("origin", "")
                try:
                    x, y, z = (float(t) for t in o.split())
                except ValueError:
                    self.report["entities_dropped"] += 1
                    continue
                if (self.lit and X0 + WALL <= x <= X1 - WALL and Y0 + WALL <= y <= Y1 - WALL
                        and FLOOR - 8 <= z <= ROOF - 8):
                    self.entities.append((collections.OrderedDict(keys), []))
                    self.report["lights_kept"] += 1
                else:
                    self.report["entities_dropped"] += 1
                continue
            self.report["entities_dropped"] += 1     # the source's spawns, items and machinery logic

    # -- new construction -------------------------------------------------------------------------
    def w(self, lines, tex=None):
        lines = aslines(lines)
        self.world.append(lines)
        self.solids.add(lines)
        if tex:
            self.report["parts"][tex] += 1

    def point(self, cls, name, origin, extra=None):
        keys = collections.OrderedDict([("classname", cls)])
        if name:
            keys["targetname"] = name
        for k, v in (extra or {}).items():
            keys[k] = v
        keys["origin"] = "%d %d %d" % origin
        self.entities.append((keys, []))

    def _shell(self):
        # the floor and the two solid facings: the ship continues west and north, and the machine
        # banks stand against those walls. The south and east facings are the galleries.
        self.w(brush(((X0 - WALL, Y0 - WALL, FLOOR - WALL), (X1 + WALL, Y1 + WALL, FLOOR)), FLOORTEX), FLOORTEX)
        self.w(brush(((X0 - WALL, Y0 - WALL, FLOOR), (X0, Y1 + WALL, ROOF + WALL)), WALLTEX), WALLTEX)
        self.w(brush(((X0 - WALL, Y0 - WALL, FLOOR), (X1 + WALL, Y0, ROOF + WALL)), WALLTEX), WALLTEX)
        self._gallery_wall("south")
        self._gallery_wall("east")
        self.w(brush(((X0 - WALL, Y0 - WALL, ROOF), (X1 + WALL, Y1 + WALL, ROOF + WALL)), CEILTEX), CEILTEX)
        self._ribs()
        self._bulkheads()

    def _bulkheads(self):
        # The two solid facings are the ship continuing: give them the source's own ribs, pipe
        # channels and trim, so they read as bulkheads rather than as the inside of a plain box.
        # These are also what makes the west- and north-facing frames carry structure: a bare wall
        # is a flat field, and the archetype's measure is right to call that out.
        for y in range(Y0 + 96, Y1 - 64, 160):
            self.w(brush(((X0, y, FLOOR), (X0 + 16, y + 16, ROOF)), RIBTEX), RIBTEX)
            self.w(brush(((X0 + 4, y + 24, FLOOR + 48), (X0 + 12, y + 128, FLOOR + 64)), TRIMTEX), TRIMTEX)
        for x in range(X0 + 96, X1 - 64, 160):
            self.w(brush(((x, Y0, FLOOR), (x + 16, Y0 + 16, ROOF)), RIBTEX), RIBTEX)
            self.w(brush(((x + 24, Y0 + 4, FLOOR + 48), (x + 128, Y0 + 12, FLOOR + 64)), TRIMTEX), TRIMTEX)
        self.w(brush(((X0, Y0 + 64, FLOOR + 96), (X0 + 12, Y1 - 64, FLOOR + 112)), PANELTEX), PANELTEX)
        self.w(brush(((X0 + 64, Y0, FLOOR + 96), (X1 - 64, Y0 + 12, FLOOR + 112)), PANELTEX), PANELTEX)

    def _gallery_wall(self, side):
        """A wall with a window band, or a solid wall when the galleries are sealed (--unlit/--sealed).

        The aperture is **closed** by a sky brush, so the map is sealed and -vis must still produce a
        portal file; the sky brush is what a person sees through the opening. This is the way an away
        site reads as *outside* without a leak.
        """
        if side == "south":
            y0, y1 = Y1, Y1 + WALL
            self.w(brush(((X0, y0, FLOOR), (X1, y1, SILL)), WALLTEX), WALLTEX)
            self.w(brush(((X0, y0, HEADER), (X1, y1, ROOF)), WALLTEX), WALLTEX)
            self.w(brush(((X0, y0, SILL), (X0 + WALL, y1, HEADER)), RIBTEX), RIBTEX)
            self.w(brush(((X1 - WALL, y0, SILL), (X1, y1, HEADER)), RIBTEX), RIBTEX)
            if self.galleries:
                self.w(brush(((X0 + WALL, y0, SILL), (X1 - WALL, y1, HEADER)), SKYTEX), SKYTEX)
            else:
                self.w(brush(((X0 + WALL, y0, SILL), (X1 - WALL, y1, HEADER)), WALLTEX), WALLTEX)
            # the rail a person stands behind, inside the window
            self.w(brush(((X0 + WALL, Y1 - RAIL_INSET, FLOOR), (X1 - WALL, Y1 - RAIL_INSET + 12,
                                                                FLOOR + RAIL_H)), RIBTEX), RIBTEX)
            self.report["gallery_south"] = [X0 + WALL, Y1, SILL, X1 - WALL, Y1 + WALL, HEADER]
        else:
            x0, x1 = X1, X1 + WALL
            self.w(brush(((x0, Y0 + WALL, FLOOR), (x1, Y1 - WALL, SILL)), WALLTEX), WALLTEX)
            self.w(brush(((x0, Y0 + WALL, HEADER), (x1, Y1 - WALL, ROOF)), WALLTEX), WALLTEX)
            self.w(brush(((x0, Y0, FLOOR), (x1, Y0 + WALL, ROOF)), RIBTEX), RIBTEX)
            self.w(brush(((x0, Y1 - WALL, FLOOR), (x1, Y1, ROOF)), RIBTEX), RIBTEX)
            if self.galleries:
                self.w(brush(((x0, Y0 + WALL, SILL), (x1, Y1 - WALL, HEADER)), SKYTEX), SKYTEX)
            else:
                self.w(brush(((x0, Y0 + WALL, SILL), (x1, Y1 - WALL, HEADER)), WALLTEX), WALLTEX)
            self.w(brush(((X1 - RAIL_INSET, Y0 + WALL, FLOOR), (X1 - RAIL_INSET + 12,
                                                                Y1 - WALL, FLOOR + RAIL_H)), RIBTEX), RIBTEX)
            self.report["gallery_east"] = [X1, Y0 + WALL, SILL, X1 + WALL, Y1 - WALL, HEADER]

    def _ribs(self):
        # the gallery's mullions and the room's own ribs: the source's `m_ribs` and `h_trim`, laid
        # out on the source's grid, so the window reads as a ship's window and not a missing wall.
        for x in range(X0 + 128, X1 - 64, 160):
            self.w(brush(((x - 8, Y1 - 4, SILL), (x + 8, Y1 + WALL - 4, HEADER)), RIBTEX), RIBTEX)
        for y in range(Y0 + 128, Y1 - 64, 160):
            self.w(brush(((X1 - 4, y - 8, SILL), (X1 + WALL - 4, y + 8, HEADER)), RIBTEX), RIBTEX)
        # a ceiling cove on the room's centre line, from the source's own light material
        self.w(brush(((X0 + 96, Y0 + 160, ROOF - 20), (X1 - 96, Y0 + 176, ROOF - 8)), LAMPTEX), LAMPTEX)

    def _lighting(self):
        # MAINTAINED LIGHT (docs/outside-the-ship.md): the alien deck is working, not failing. Gold
        # ceiling coves and green work lamps, in the source's own palette; the gallery's starfield
        # also lights the sill. Nothing here is the derelict's one-in-two failing bank.
        n = 0
        for x in range(X0 + 160, X1 - 96, 224):
            for y in range(Y0 + 160, Y1 - 96, 224):
                self.point("light", None, (x, y, ROOF - 32),
                           {"light": "620", "_color": "0.98 0.84 0.55"})
                self.w(brush(((x - 24, y - 24, ROOF - 12), (x + 24, y + 24, ROOF - 4)), LAMPTEX), LAMPTEX)
                n += 1
        for y in range(Y0 + 176, Y1 - 64, 192):
            self.w(brush(((X0 + 96, y, FLOOR + 8), (X0 + 108, y + 80, FLOOR + 16)), GREENTEX), GREENTEX)
            self.point("light", None, (X0 + 112, y + 40, FLOOR + 48),
                       {"light": "300", "_color": "0.62 0.96 0.72"})
            n += 1
        # a brighter pool at the console end, so the working post is legible at every facing
        self.point("light", None, (X0 + 300, Y0 + 320, ROOF - 40),
                   {"light": "700", "_color": "1.00 0.88 0.62"})
        n += 1
        # and the east strip, where the second gallery looks back into the room: without it the
        # inward frame at that gallery is a dim wall, and "a lit room" is not what it shows.
        self.point("light", None, (X1 - 128, Y0 + 272, ROOF - 40),
                   {"light": "760", "_color": "0.98 0.86 0.60"})
        self.w(brush(((X1 - 152, Y0 + 248, ROOF - 12), (X1 - 104, Y0 + 296, ROOF - 4)), LAMPTEX), LAMPTEX)
        n += 1
        self.report["lights_added"] = n

    def brush_ent(self, keys, lines):
        self.entities.append((collections.OrderedDict(keys), [aslines(lines)]))

    def _console(self):
        # an alien console: the source's own plate, panel and trim, standing against the machine
        # bank. It is the place a person works, not a Federation screen.
        ox, oy = X0 + 324, Y0 + 288
        base = self.solids.floor_at(ox, oy)
        self.point("info_notnull", "lwh_station_site", (ox, oy, base + 24))
        x0, x1 = ox - 72, ox + 72
        y0, y1 = oy - 48, oy + 48
        self.w(brush(((x0, y0, base), (x1, y1, base + 44)), RIBTEX), RIBTEX)
        self.w(brush(((x0 + 16, y0 + 8, base + 44), (x1 - 16, y1 - 8, base + 76)), WALLTEX), WALLTEX)
        self.w(brush(((x0 + 24, y0 + 12, base + 76), (x1 - 24, y1 - 12, base + 104)), PANELTEX), PANELTEX)
        self.w(brush(((x0 + 32, y0 + 16, base + 104), (x1 - 32, y1 - 16, base + 112)), GLASSTEX), GLASSTEX)
        self.report["console"] = [ox, oy, base]
        self.report["anchor"] = [ox, oy, base + 24]

    def _drab(self):
        # the palette clause's before-capture: the same lit room, every material neutral grey.
        def retex(lines):
            out = []
            for ln in lines:
                if ln.startswith("("):
                    m = stitch.FACE_TEXTURE.match(ln)
                    if m:
                        ln = ln[:m.start(2)] + DRABTEX + ln[m.end(2):]
                out.append(ln)
            return out
        self.world = [retex(lines) for lines in self.world]
        for keys, brushes in self.entities:
            for b in brushes:
                b.lines = retex(b.lines)
            if keys.get("classname") == "light":
                # white light on grey surfaces: the palette is the only thing removed. Coloured
                # lights on grey walls would still carry a palette, and the clause would not be
                # the thing the capture falsifies.
                keys["_color"] = "1 1 1"
        self.report["drabtex"] = DRABTEX

    # -- placement helpers ------------------------------------------------------------------------
    def _post_spot(self, cx, cy, maxr=224):
        """A post the vantage rig can stand at: clear, on the deck, and with both of the rig's
        paces still inside the shell on the same floor. A post near a wall photographs the wall's
        far side (or a pit beyond it) and the frame is no longer the site's."""
        for r in range(0, maxr, 32):
            for dx, dy in ((0, 0), (r, 0), (-r, 0), (0, r), (0, -r), (r, r), (-r, r), (r, -r), (-r, -r)):
                x, y = cx + dx, cy + dy
                if not (X0 + WALL + VANTAGE_PACE + 16 <= x <= X1 - WALL - VANTAGE_PACE - 16):
                    continue
                if not (Y0 + WALL + VANTAGE_PACE + 16 <= y <= Y1 - WALL - VANTAGE_PACE - 16):
                    continue
                z = self.solids.floor_at(x, y) + 24
                if self.solids.floor_at(x, y) > FLOOR + 48 or not self.solids.box_clear(x, y, z):
                    continue
                if not all(self.solids.box_clear(x + ox, y, z) and
                           abs(self.solids.floor_at(x + ox, y) - self.solids.floor_at(x, y)) <= 8
                           for ox in (-VANTAGE_PACE, VANTAGE_PACE)):
                    continue
                if not all(self.solids.box_clear(x, y + oy, z) and
                           abs(self.solids.floor_at(x, y + oy) - self.solids.floor_at(x, y)) <= 8
                           for oy in (-VANTAGE_PACE, VANTAGE_PACE)):
                    continue
                return (x, y)
        return None

    def _find_open(self, cx, cy, maxr=224):
        for r in range(0, maxr, 64):
            for dx, dy in ((0, 0), (r, 0), (-r, 0), (0, r), (0, -r), (r, r), (-r, r), (r, -r), (-r, -r)):
                x, y = cx + dx, cy + dy
                if not (X0 + 96 <= x <= X1 - 96 and Y0 + 96 <= y <= Y1 - 96):
                    continue
                z = self.solids.floor_at(x, y) + 24
                if self.solids.floor_at(x, y) > FLOOR + 48:
                    continue
                if any(not self.solids.box_clear(x + ox, y + oy, z)
                       for ox, oy in ((0, 0), (64, 0), (-64, 0), (0, 64), (0, -64))):
                    continue
                return (x, y)
        return None

    def _waypoints(self):
        cand = [(x, y) for x in range(X0 + 64, X1 - 32, 96) for y in range(Y0 + 64, Y1 - 32, 96)]
        seen, final = set(), []
        for (x, y) in cand:
            key = (round(x / 64), round(y / 64))
            if key in seen:
                continue
            z = self.solids.floor_at(x, y) + 24
            if self.solids.floor_at(x, y) > FLOOR + 48:
                continue                                  # a ledge a body cannot climb: not a place
            if not self.solids.box_clear(x, y, z):
                continue
            seen.add(key)
            final.append((int(x), int(y), int(z)))
        for (x, y, z) in final:
            self.point("waypoint", None, (x, y, z))
        self.report["waypoints"] = len(final)
        # posts: the crew layer reads `waypoint_navgoal_*` as the places a person can work. The
        # console and the machine bank are the two; the gallery is a place to look, not a post.
        posts = []
        for label, near in (("console", (X0 + 128, Y0 + 384)),
                            ("bank", (X0 + 288, Y0 + 144))):
            spot = self._post_spot(int(near[0]), int(near[1]))
            if not spot:
                continue
            x, y = spot
            z = self.solids.floor_at(x, y) + 24
            posts.append((x, y))
            self.point("waypoint_navgoal", "lwh_post_%s" % label, (x, y, z),
                       {"message": "%s" % label})
        self.report["posts"] = posts
        # the gallery: the place the outside assertion is shot from, inside the rail and facing out
        gx, gy = self._find_open((X0 + X1) // 2, Y1 - 96) or ((X0 + X1) // 2, Y1 - 96)
        gz = self.solids.floor_at(gx, gy) + 24
        self.point("info_notnull", "lwh_gallery", (gx, gy, gz))
        self.report["gallery"] = [gx, gy, gz]
        # the second gallery, on the east wall: a place to look out, not a post. It stands in the
        # east open strip, and the rig's shorter pace (64) keeps both of its pace frames inside.
        ex, ey = X1 - WALL - 96, Y0 + 272
        if not self.solids.box_clear(ex, ey, self.solids.floor_at(ex, ey) + 24):
            ex, ey = self._find_open(ex, ey) or (ex, ey)
        ez = self.solids.floor_at(ex, ey) + 24
        self.point("info_notnull", "lwh_gallery_east", (ex, ey, ez))
        self.report["gallery_east_anchor"] = [ex, ey, ez]
        # the arrival: at the room's west end, facing the galleries
        ax, ay = X0 + 160, (Y0 + Y1) // 2
        spot = self._find_open(ax, ay)
        if spot:
            ax, ay = spot
        az = self.solids.floor_at(ax, ay) + 24
        self.point("info_player_start", None, (ax, ay, az), {"angle": "0"})
        self.point("info_notnull", "lwh_alien_site", (ax, ay, az),
                   {"message": "away site: an alien ship interior"})
        self.report["arrival"] = [ax, ay, az]


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--source", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--name", default="lwh_alien_ship")
    ap.add_argument("--unlit", action="store_true",
                    help="add NO lighting, drop the source's own lights and seal the galleries: the "
                         "before-capture the lit clause must reject")
    ap.add_argument("--drab", action="store_true",
                    help="keep the lighting, replace every material with neutral grey: the "
                         "before-capture the palette clause must reject")
    ap.add_argument("--sealed", action="store_true",
                    help="keep the lighting, seal the galleries: the before-capture an 'open facing' "
                         "assertion must reject")
    ap.add_argument("--report")
    ap.add_argument("--census")
    a = ap.parse_args(argv)

    b = Build(a.source, lit=not a.unlit, galleries=not (a.unlit or a.sealed), drab=a.drab)
    os.makedirs(a.out, exist_ok=True)
    path = os.path.join(a.out, "%s.map" % a.name)
    world_keys = collections.OrderedDict([
        ("classname", "worldspawn"),
        ("message", "Away site - an alien ship interior (re-dressed from scav3)"),
        ("soundSet", "forgehalls"),
        ("music", "music/elite17.mp3"),
        ("gravity", "800"),
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
    b.report["parts"] = dict(b.report["parts"])
    if a.report:
        with open(a.report, "w") as f:
            json.dump(b.report, f, indent=1)
    if a.census:
        with open(a.census, "w") as f:
            json.dump(b.report["census"], f, indent=1)
    r = b.report
    print("%s <- %s" % (a.name, a.source))
    print("  window x %d..%d y %d..%d, floor %d roof %d (sill %d header %d)"
          % (X0, X1, Y0, Y1, FLOOR, ROOF, SILL, HEADER))
    print("  world brushes %d, brushes kept %d/%d, lights kept %d, waypoints %d, posts %s"
          % (len(b.world), r["brushes_kept"], r["brushes_kept"] + r["brushes_dropped"],
             r["lights_kept"], r["waypoints"], r["posts"]))
    print("  galleries: south %s east %s; drab %s, lit %s"
          % ("open" if b.galleries else "sealed", "open" if b.galleries else "sealed",
             b.drab, b.lit))
    print("  wrote %s" % path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
