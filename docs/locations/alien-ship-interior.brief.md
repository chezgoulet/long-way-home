# Location brief: the alien ship interior

**Deck and room:** not a deck of the player's ship. This is the **second** away-site archetype — the
place an away team beams to, outside the ship. Map `lwh_alien_ship`, built by
`tools/away/dress_alien_ship.py` from the shipped `scav3` source. Design:
`docs/outside-the-ship.md` (the archetype table; *Order of work* 2–3). Procedure:
`docs/authoring-a-location.md`. Evidence: `docs/evidence/alien-ship-interior.md`.

## 1. Function, in one sentence

An away team beams into an **intact alien engineering deck** — green ribbed machinery, gold lamp
coves, a working console, and two galleries looking out onto a starfield — where a person can walk,
work, and look out. What a site returns to the ship is the away-mission loop's, which is not built.

## 2. Canon anchors — and the decision this brief records

- **Not a canon place, and it does not pretend to be one.** No episode is reproduced. The site is one
  of the **archetypes** `docs/outside-the-ship.md` names as the kinds of place the Delta Quadrant
  throws at a ship; canon supplies the *kind*, not the room. The source's own species is the
  scavengers of `scav1`-`5`; **the site names no species, owner or episode**.
- **The archetype, and why it is second.** The design's table has six. The derelict interior (the
  first row) was built on `feat/derelict-interior`; this is the row after it, **the alien ship
  interior**, and the design's own source family is `scav1`-`5`. `scav2` is the derelict's source
  and is not reused.
- **Contested or uncertain, decided here:** the site is an **engineering deck of a working alien
  vessel**, not a wreck and not a station. The two open facings are **galleries cut through the
  hull**; the other two are solid because the ship continues there. The decision is recorded rather
  than assumed.
- **Neighbours:** none. An away site is reached by transporter or shuttle; the way home is the ship
  (the away-mission loop, `docs/outside-the-ship.md` §4).

## 3. Reuse target — the biggest cost saving, and it is checkable

- **Copy this existing map:** `scav3` — one of the design's named sources for the alien ship
  interior, and the densest compact single-storey candidate in
  `docs/evidence/the-archetype-survey.md` (3,547 brushes, 258 patches, one 128-unit deck).
- **Change exactly this:**
  1. **Crop to the upper engineering storey** — the window `x −224..512, y 560..1120`, floor z −128,
     roof z 16. Everything below is another deck and is not this site.
  2. **Drop crossing brushes, not clamp them.** A brush that crosses the window boundary is dropped
     and the shell replaces it; clamping a sloped brush's plane points turns a valid six-plane brush
     into a five-plane one, and a five-plane brush has no interior. Two hundred and forty-five
     source brushes and twenty-four patches are kept, and the rest are named in the report.
  3. **Shell it:** a floor, a ceiling, and solid walls west and north (the ship continues, and the
     machine banks stand against them). The south and east walls are **galleries**: a window band
     with mullions and a rail, closed by a sky brush (`holodeck_temple/ctfc_sky_light2`,
     `skyParms env/stars`) so the map is sealed and `-vis` still produces a portal file.
  4. **Light it:** the source's own light entities are kept where they fall inside the room; gold
     ceiling coves and green work lamps are added in the source's own materials
     (`scavenger/striplight`, `scavenger/m_ceiling`).
  5. **Dress it:** an alien console from the source's own plate, panel, trim and glass
     (`scavenger/m_ribs`, `scavenger/k_panel6_b`, `scavenger/h_trim`,
     `engineering/glass_nolightmap`), two working posts, and a gallery rail.
  6. **Wire it:** `info_player_start` at the arrival, a `waypoint` grid on the deck, two
     `waypoint_navgoal_*` posts (the console and the machine bank) for the crew layer, an
     `info_notnull` named `lwh_gallery` for the outside assertion, and an `info_notnull` named
     `lwh_alien_site` as the marker the away loop is designed to key on.

## 4. Parts list

Every material is the source's own or a shipped game material — `tools/shipmap/parts_census.py`
proves no new art (26 distinct materials, all present in the game or in the reuse source's census):

| part | existing asset | where it is used already |
|---|---|---|
| walls, machinery, floor plates | `scavenger/m_basic`, `scavenger/m_floor3`, `scavenger/m_floor5` | `scav3` (copied) |
| ribs, trim, panels | `scavenger/m_ribs`, `scavenger/h_trim`, `scavenger/k_panel6_b` | `scav3` (copied; also the gallery mullions and the console) |
| ceiling and lamp coves | `scavenger/k_ceiling_b`, `scavenger/striplight` | `scav3` (copied; also the added coves) |
| the console's glass | `engineering/glass_nolightmap` | `scav3` (copied) |
| the starfield | `holodeck_temple/ctfc_sky_light2` (`skyParms env/stars`) | shipped shader (the gallery sky) |
| the drab capture's material | `common/greysquare2` | shipped game material, used only by `--drab` |

## 5. Spatial program

- **Footprint:** the room is `736 × 560` units inside a shell `768 × 592`, floor at z −128, roof at
  z 16 (the source's own ceiling at z 0 stays beneath the shell). A small site by design.
- **Entries and exits:** one arrival, at `(−64 776 −104)` — the place the transporter sets a party
  down. The gallery rail is the only edge; there is no walk-off.
- **Circulation:** in from the arrival, past the console at `(100 848)`, round the machine bank to
  the north, and out along the south gallery to the window; the east gallery is the second way to
  look out.
- **Verticality:** one deck; the galleries are the only openings, and they are the site's "outside".
- **Chokepoint:** the gap between the console and the machine bank, where the room narrows.
- **Where a person stands:** the two `waypoint_navgoal` posts — the console at `(−64 944)` and the
  machine bank at `(0 704)` — and the gallery anchor at `(144 1024)`, which is a place to look, not a
  post a person works.

## 6. Lighting brief

Light by function:

- **the working deck: gold ceiling coves** (`light 620`, `_color 0.98 0.84 0.55`) on a grid, each
  with a visible `striplight` fixture;
- **the walls: green work lamps** (`light 300`, `_color 0.62 0.96 0.72`) low along the west wall,
  with a `striplight` bar;
- **the console end: a brighter pool** (`light 700`, warm), so the working post is legible at every
  facing;
- **the galleries: the sky's own light** (`ctfc_sky_light2`, `q3map_sun 0.9 0.9 1 35 65 75`), which
  lights the sill and is the only light in the outward frames.

There is **no emergency-lighting state** and no failure mode of the site's own: the emergency
lighting and the air clocks are the ship's, and an away site is not on its grid. An away site's own
endurance is the away-mission loop's, which is not built.

## 7. The hook — what this room does for the simulation

- **Navigation:** a `waypoint` grid on the deck; the engine bakes `maps/lwh_alien_ship.nav` on first
  load, without which no crew could work the site.
- **Crew posts:** two `waypoint_navgoal_*` posts that the crew layer (`module/crew/g_crew.cpp`)
  reads as places a person can work. Measured: see §8.
- **The arrival:** `info_player_start` at the deck — the place the party is set down.
- **The site marker:** `info_notnull` named `lwh_alien_site`. **It has no reader yet**, and that is
  named rather than hidden: the away-mission loop that would key on it — beam down, scan, carry
  back, leave before the window closes — is *scenario content* and the next item of
  `docs/outside-the-ship.md`'s order of work, deliberately not built here.

## 8. Acceptance checks

Machine-checkable, in `scripts/alien-ship-check.sh`:

- the map compiles and `tools/mapgen/check-bsp.py` reads the artifact (structure, not exit codes);
- `-vis` produces a portal file, so the shell and its sky brushes are sealed;
- it loads in the engine (`CM_LoadMap`, `Munro connected`) and the harness stands at the site's own
  two posts;
- `maps/lwh_alien_ship.nav` is baked by the engine on first load (deleted before the run);
- every material already exists in the game (`tools/shipmap/parts_census.py`);
- the site's two posts each read at their own six vantages, measured by **`frame.py alien`** — lit,
  structured and coloured — with **no per-frame dark exemption**;
- the gallery's outward frame is **black space** (median ≤ 5) and its inward frame is a **lit room**
  (median ≥ 40), and the two differ by ≥ 30% — the opening is a picture of a world;
- the **drab** capture (same room, grey materials, white lights) fails the palette clause; the
  **unlit** capture (no lights, galleries sealed) fails the maintained clause; the **sealed** capture
  moves the outward median above 5, so the outside assertion rejects a sealed site;
- the crew layer embodies the ship's roster on the site and **every body reaches its place**
  (`g_crew`, the `scripts/s5-check.sh` shape), with none in solid.

Then the one that decides, and it is not here: **does it read as an alien place, rather than as
assembled parts?** That is the owner's walkthrough. The measure can assert a lit, built, coloured
interior; it cannot assert that the palette is the *right* species' or that the room is good.

## 9. Non-goals

- **No game assets committed.** The site is built from what the installation already has, re-dressed
  by script; the extracted sources are regenerated, never committed. No new art.
- **No new systems, no ship-side changes.** The only module change is the vantage rig's pace input
  (`g_shipVantagePace`, default 192, so no existing caller changes); no ship simulation and no save
  state is touched.
- **No scenario content.** The away-mission loop (loadout, window, return, write-back) is not built;
  the site is the map it is intended to run on.
- **Not five archetypes badly.** One, properly; the other four remain.
- **Nothing invented to fill a gap** — nothing about the site claims an owner, a species or an
  episode.
