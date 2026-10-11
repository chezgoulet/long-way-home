# Location brief: the away site — an outpost in a debris field

**Deck and room:** not a deck. This is the first **away site** — the place an away team beams to,
outside the ship. Map `lwh_away_depot`, built by `tools/away/dress_away_site.py` from the shipped
`ctf_space` source. Design: `docs/outside-the-ship.md` (the archetype table; *Order of work* 2–3).
Procedure: `docs/authoring-a-location.md`. Evidence: `docs/evidence/the-away-site.md`.

## 1. Function, in one sentence

An away team beams onto the apron of an abandoned outpost standing in a debris field, where a person
can walk the deck, work the console, find the stores left behind, and scan the site — and where the
*outside* is the point: whatever else it is, it must not read as another corridor.

## 2. Canon anchors — and the decision this brief records

- **Not a canon place, and it is not pretending to be one.** No episode is being reproduced. The site
  is one of the **archetypes** `docs/outside-the-ship.md` names as the kinds of place the Delta
  Quadrant throws at a ship; canon supplies the *kind*, not the room.
- **The archetype, and why it is first.** The design's table has six (`derelict interior`, `alien ship
  interior`, `industrial surface facility`, `space station / outpost`, `cave / mine`, `colony`). This
  pass builds **one properly rather than five badly**, and it is the **space station / outpost**, whose
  source the design itself names: `ctf_space`. **Why this one first:** it is the archetype whose source
  already carries the one thing the brief says a site exists to do — `ctf_space`'s own deck already
  stands in an open debris field under the game's own lit sky (`textures/common/sky_light`, `skyParms
  env/junk`, `q3map_sun`). The derelict, the surface facility and the rest need the exterior *built*;
  this one inherits it, so it is the cheapest place to prove the exterior reads at all. The derelict
  interior is the design's own first row and remains the next one to build.
- **Contested or uncertain, decided here:** the *kind* of outpost. Canon gives no name, owner or
  flag for this site; it is **an abandoned industrial outpost**, and nothing on it says otherwise. The
  decision is recorded rather than assumed, per `docs/authoring-a-location.md` step 1.
- **Neighbours:** none. An away site is not adjacent to anything; it is reached by transporter or
  shuttle, and the way home is the ship (the away-mission loop, `docs/outside-the-ship.md` §4).

## 3. Reuse target — the biggest cost saving, and it is checkable

- **Copy this existing map:** `ctf_space` — the design's own named source for the station/outpost
  archetype (`docs/outside-the-ship.md`, archetype table), and the smallest of the six archetypes'
  sources, so it is the one that fits a *small* site.
- **Change exactly this:**
  1. **Crop to the outpost's top deck** — the window `x 400..1280, y 672..1144`, the height band from
     the deck floor (z 168) up. Everything below that deck is the arena's lower levels and is not this
     site.
  2. **Drop the machine blocks that stand on the deck** (a body over 64 units square, sitting on the
     deck rather than forming it). The source's deck is an arena packed with them; a crop that keeps
     them is a maze, not a place. Thin ribs, rails, the floor plates and the walls stay.
  3. **Shell it:** solid walls west/south/north; a solid roof over the sheltered half (`x ≤ 880`); the
     open half left under the source's own sky, with a solid parapet at the far edge and a sky wall
     beyond it — the edge a person stands at to look out.
  4. **Light it:** the source's own light entities are kept where they fall on this deck; a working
     outpost's ceiling light, mid-height wall lamps and edge lamps are added (the source's own light
     materials, `forge/lillight` and `forge/trimlight2`).
  5. **Dress it:** a console (source materials), a cargo rack left standing (`models/mapobjects/cargo/
     rack.md3`, shipped, used in `scav1`), and a crate with it — the one human trace *and* the stores
     the loop will read.
  6. **Wire it:** `info_player_start` at the arrival, a dense `waypoint` grid on the deck for
     navigation, `waypoint_navgoal_*` posts for the crew layer, and an `info_notnull` named
     `lwh_away_site` as the marker the away loop will key on.
- **Keep** everything else copied, including its mistakes: the source's own `common/caulk` step
  (invisible, 16 units high, `x 704..896 y 864..1056`) is kept because it was copied.

## 4. Parts list

Every material is the source's own — `tools/shipmap/parts_census.py` proves no new art (8 distinct
materials, all present in the game or in the reuse source's census):

| part | existing asset | where it is used already |
|---|---|---|
| deck and wall plates | `forge/marmetal2_b`, `forge/flooring_b` | `ctf_space` floor plates (copied) |
| shell walls and console | `forge/fmetal1` | `ctf_space` wall/block material (copied; also the shell) |
| rails and ribs | `forge/ribs2` | `ctf_space` railings (copied) |
| the red trim strips | `forge/trimlight2` | `ctf_space` trim (copied) and the console's face |
| ceiling / wall / edge lamps | `forge/lillight` | `ctf_space` lamp material (copied; added as fixtures) |
| the sky | `common/sky_light` (`skyParms env/junk`, `q3map_sun`) | `ctf_space`'s own sky (copied) |
| the cargo rack | `models/mapobjects/cargo/rack.md3` | `scav1` (the one part not from `ctf_space`; named) |

## 5. Spatial program

- **Footprint:** the deck crop is `880 × 472` units, inside a shell `912 × 504`, floor at z 168,
  roof at z 560. A small site by design — the design's *"the site is small"*, and q3map2's vis-cluster
  budget wants it small.
- **Entries and exits:** one, and it is the point of the site — the **arrival** at `(960 908 200)` on
  the open apron, where the transporter sets a party down, facing the shelter. There is no second way
  in and no way off the edge: a parapet runs the open side.
- **Circulation:** in from the apron, west past the machinery into the sheltered half where the
  console is, and back out along the deck to the stores and the edge; the deck's own ribs and rails
  make the far side of the site a place you come round into rather than see from the door.
- **Verticality:** the shell's roof covers the west half only, so the site has a sheltered half and an
  open half under the sky — the asymmetry section 5 of the template asks for, and the thing that makes
  "outside" read.
- **Chokepoint:** the threshold between the sheltered half and the open apron, between the machinery
  and the console.
- **Where a person stands:** the console post at `(720 1036)` — the place the harness photographs the
  room from — and the stores at `(924 780)`. Both are `waypoint_navgoal` posts the crew layer reads;
  the console itself stands at `(816 908)`, and the site's marker (`lwh_station_site`) at `(720 908)`.

## 6. Lighting brief

Light by function:

- **the sheltered half: warm ceiling light** (`light 2000`, `_color 1.00 0.94 0.82`) on a grid, each
  with a visible ceiling fixture, and mid-height **wall lamps** (`light 500`) along the long walls;
- **the open apron: cool floods** (`light 1400`, `_color 0.80 0.86 1.00`) high up, so the deck reads
  against the black sky;
- **the edge: marker lamps** (`light 250`, warm) at the parapet;
- **the sky: the source's own** `common/sky_light` (`q3map_surfacelight 125`, `q3map_sun 1 0.75 0.75
  75 65 65`) lights the open half from above.

There is **no emergency lighting state**, and that is deliberate: the emergency-lighting mechanism
(`lwh_light_emergency_*`, `SyncEmergencyLight`) is the ship's, and a site outside the ship is not on
its grid. The site has no failure mode of its own in this pass — an away site's air/endurance is the
away-mission loop's, which is not built.

## 7. The hook — what this room does for the simulation

- **Navigation:** a dense `waypoint` grid on the deck; the engine bakes `maps/lwh_away_depot.nav`
  from it on first load, without which no crew could work the site.
- **Crew posts:** two `waypoint_navgoal_*` posts (the console, the stores), which the crew layer
  (`module/crew/g_crew.cpp`) reads as the places a person can work. Measured: see §8.
- **The arrival:** `info_player_start` at the apron — the place the party is set down.
- **The site marker:** `info_notnull` named `lwh_away_site`, carrying the site's name. **This is a
  marker with no reader yet**, and that is named rather than hidden: the away-mission loop that would
  key on it — beam down, scan, carry back, leave before the window closes — is *scenario content* and
  the next item of `docs/outside-the-ship.md`'s order of work, deliberately not built here
  (`docs/omissions.md` O11, step 4). The map side of the loop is this brief; the loop is not.

## 8. Acceptance checks

Machine-checkable, in `scripts/away-check.sh`:

- the map compiles and `tools/mapgen/check-bsp.py` reads the artifact (structure, not exit codes);
- it loads in the engine (`CM_LoadMap`, `Munro connected`) and the player stands at the site's own post;
- `maps/lwh_away_depot.nav` is baked by the engine on first load (deleted before the run);
- every material already exists in the game (`tools/shipmap/parts_census.py`);
- the site's own two posts each read at their own four facings, measured (`tools/frames/frame.py
  floor`, mid band); the rig's paces are measured too and the one dark pace is named;
- the frame that looks out into the debris field **differs** from the frame that looks at the site's
  own wall at the same post (`frame.py diff`), so "outside" is a picture of a world;
- the crew layer embodies the ship's roster on the site and **every body reaches its place**
  (`g_crew`, the `scripts/s5-check.sh` shape), with none in solid.

Then the one that decides, and it is not here: **does it read as somewhere, rather than as assembled
parts?** That is the owner's walkthrough. **The seam is the risk, not the rooms** — this is a slice of
one source butted against new shell and new light, and a slice of a whole place can read as a diorama.
See "What could not be verified" in `docs/evidence/the-away-site.md`.

## 9. Non-goals

- **No game assets committed.** The site is built from what the installation already has, re-dressed by
  script; the extracted sources are regenerated, never committed. No new art.
- **No new systems, no ship-side changes.** Nothing in `module/` is touched by this pass.
- **No scenario content.** The away-mission loop (loadout, window, return, write-back) is not built;
  the site is the map it will run on.
- **Not five archetypes badly.** One, properly; the derelict is next.
- **Nothing invented to fill a gap** — nothing about the site claims an owner, a species or an episode.
