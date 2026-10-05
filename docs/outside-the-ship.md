# Outside the ship: the chart and the sites

The scale mismatch is the whole problem. The journey is measured in tens of thousands of light years; the game
is measured in metres and is, at heart, an interior shooter. The answer is not to build a flight sim. It is
**two representations with a contract between them**.

## 1. The chart — the galaxy as data, not as terrain

The map "outside the ship" in the strategic sense is a table plus a display.

A chart row is one object: a **system**, an anomaly, a wreck, a station, a planet, a phenomenon. Each carries:

- `type` — which kind of place it is, and therefore which site archetype loads when you go there;
- `position` and `distance` — light years from the current position and from home;
- `confidence` — how much the crew actually know (see `docs/exploration-and-science.md`; a low-confidence row
  is how a survey becomes a trap);
- `hazards`, `resources`, `first_contact` — flags that drive scenario breeding conditions;
- `site` — the map to load, or nothing if it is only ever observed from orbit.

Displayed on the **astrometrics wall** (deck 8), on the bridge, and by query from any console. And here is the
unification worth noticing: **the chart is also the map of progress** — the same artifact that shows where the
systems are shows how far is left. The navigation counter in `docs/navigation-counter.md` is a readout *of* the
chart, not a separate thing. One wall, two jobs: where we can go, and how far it is from home.

Cost: a table, a save entry, and a screen. No new geometry at all.

## 2. The site — where exploration actually happens

Arriving somewhere loads an ordinary EF map, re-dressed per the location brief. The archetypes we can build from
art that already ships:

| archetype | reuse | texture of the place |
|---|---|---|
| **derelict interior** | re-dress `dn*`, `borg*`, `scav*` | wrecked corridors, failing light, no gravity in places |
| **alien ship interior** | re-dress `scav1`-`5` | unfamiliar layout, the species' own design language |
| **industrial surface facility** | re-dress `forge1`-`5` | the closest thing the game has to a *place on a planet* |
| **space station / outpost** | re-dress `ctf_space`, `scav5a` | docking bays, terminals, a market |
| **cave / rock / mine** | re-dress `forge*` with rock textures | enclosed, dark, mineral |
| **colony or settlement** | re-dress `voy*` interiors plus new dressing | lived-in, domestic, someone's home |

That set covers a large fraction of what canon's Delta Quadrant throws at a ship: derelicts, other people's
ships, mining operations, outposts, and the occasional place where people live.

## 3. What the engine gives us for "outside" — measured, not assumed

- **Skyboxes work, and art ships**: only two shipped maps declare a `sky` at all, but the paks carry cubemap
  sets — `junk_*` (a debris field, essentially a ready-made exterior), `bsky_*`, `camelot_*`, plus cloud
  textures. A map can therefore *look* like space or open sky without new geometry.
- **Everything else is interior.** `func_group`, `func_door` and `func_usable` dominate every shipped map. There
  is no terrain system, no vehicle or flight system, and no zero-gravity EVA movement.
- So: **exterior views are cheap, exteriors as playable space are not.** A room with a window onto a starfield is
  an afternoon. Flying a shuttle is engine work with a real rebill.

**Scope recommendation:** take the cheap exteriors and skip the expensive ones. A starfield out the window, a
debris field, a planet on the viewscreen, a hull you can see from a docking bay — these carry the feeling. A
flight sim does not, and it would cost more than every system in `docs/ship-systems.md` combined.

## 4. The away mission is the interface

The crew do not explore by flying. They explore by **going**, and the contract is already specified: crew leave
posts (the job economy), carry a loadout (stores), need a way home (transporter range 40,000 km standard,
10 km emergency; blocked by shields, ion storms and certain minerals -- or a shuttle), and the window can close.

And the rule that keeps away sites from being scenery: **a site exists to write back to the ship.** Resources to
stores, samples to the lab, data to the chart, casualties to crew records, occasionally a recovered component
that becomes a system or a modification. A site that returns nothing is a holiday.

## Order of work

1. **The chart** — table, save entry, astrometrics display. Cheap, and it makes everything else legible.
2. **Two or three site archetypes** re-dressed from existing maps, one of each kind we expect to reuse most: a
   derelict, a surface facility, a station.
3. **The exterior dressing pass** — starfield and debris skies on a handful of maps, so "outside" reads.
4. **The away-mission loop** on top (crew, loadout, window, return, write-back).
5. **Only then** consider anything requiring new engine capability, and treat it as its own gate.
