# Evidence: the archetype source survey (derelict interior), landed

**Why this exists.** The Item-1 pass stopped before Item 2 (the derelict interior) and measured four
candidate source maps in the conversation, but **landed none of it in the repository**, and named the
instrument only as "`tools/stitch`" — **not a path on this tree**. A successor could not re-run it, and a
number whose instrument is unnamed is not reproducible. This note lands the survey: the sources, the exact
command, and the rows. It is the instrument's own file — `tools/away/survey_sources.py` — so the next pass
re-runs it rather than re-deriving it.

**The instrument, named.** `tools/shipmap/stitch.py` is the map parser the ship and away-site evidence
already cite; `tools/away/survey_sources.py` wraps it and prints one row per source. **It counts brushes,
not faces**: a six-faced brush is one brush, and that is the unit that says how dense a source is. (The
earlier conversation's figures were cross-checked against the source maps: entities and lights reproduced
exactly; the brush figure is the brush count, and a face count would read ~6× larger.)

```sh
tools/away/survey_sources.py build/gdk/maps/*/scav2.map build/gdk/maps/*/scav4.map \
    build/gdk/maps/*/dn3.map build/gdk/maps/*/borg2.map
```

| map | brushes | patches | entities | lights | bounds x | bounds y | bounds z | footprint | height |
|---|---|---|---|---|---|---|---|---|---|
| `build/gdk/maps/eliteforce_scav_maps/scav2.map` | 7028 | 61 | 1201 | 475 | −1176..1612 | −2376..2068 | −168..476 | 2.7×4.3 | **0.6** |
| `build/gdk/maps/eliteforce_scav_maps/scav4.map` | 3263 | 331 | 878 | 197 | −996..1728 | 272..4310 | −648..808 | 2.7×3.9 | 1.4 |
| `build/gdk/maps/eliteforce_dreadnought_maps/dn3.map` | 2394 | 172 | 645 | 296 | −778..2096 | −1318..3524 | −1560..956 | 2.8×4.7 | 2.5 |
| `build/gdk/maps/eliteforce_borg_maps/borg2.map` | 4098 | 240 | 825 | 332 | −1348..3016 | −2720..1464 | −776..1648 | 4.3×4.1 | 2.4 |

**The reading.** The derelict interior (`docs/outside-the-ship.md:34`: *re-dress `dn*`, `borg*`, `scav*` —
wrecked corridors, failing light, no gravity in places*) wants a **compact single-deck interior** — a
corridor network to crop and shell, not a multi-storey hull. `scav2` is that: the densest of the four, in
a **z span of 168..476 alone (0.6 k)** — one deck — where `dn3` (2.5 k) and `borg2` (2.4 k) are multi-deck
and `scav4` (1.4 k) sits between. So `scav2` is the source the derelict pass starts from, as the stopped
pass found; this note makes that finding reproducible.

## The alien ship interior — the source chosen for the next row

The design's second row is the **alien ship interior**, re-dress `scav1`-`5`. `scav2` is the
derelict's source and is not reused. The same instrument, on the remaining scavenger maps:

```sh
tools/away/survey_sources.py build/gdk/maps/eliteforce_scav_maps/scav1.map \
    build/gdk/maps/eliteforce_scav_maps/scav3.map build/gdk/maps/eliteforce_scav_maps/scav4.map \
    build/gdk/maps/eliteforce_scav_maps/scav5.map
```

| map | brushes | patches | entities | lights | bounds x | bounds y | bounds z | footprint | height |
|---|---|---|---|---|---|---|---|---|---|
| `scav1` | 897 | 12 | 366 | 244 | −4408..6848 | −7784..4512 | −3160..2600 | 11.0×12.0 | 5.6 |
| `scav3` | 3547 | 258 | 589 | 226 | −1544..776 | −576..1920 | −424..496 | 2.3×2.4 | **0.9** |
| `scav4` | 3263 | 331 | 878 | 197 | −996..1728 | 272..4310 | −648..808 | 2.7×3.9 | 1.4 |
| `scav5` | 3651 | 316 | 966 | 235 | −1200..1676 | −2728..1684 | −1288..416 | 2.8×4.3 | 1.7 |

**The reading.** The alien ship wants an **intact** compartment with a strong species' design language,
not the derelict's wreck: `scav3`'s upper engineering storey is one 128-unit deck (z −128..0) of green
ribbed machinery, gold lamp coves and arched bulkheads, and it is the densest compact single-storey
candidate (`scav1` is an 11×12 arena-sized map; `scav4` and `scav5` are multi-deck and larger). The
re-dress copy is the room around the machine banks (`x −224..512, y 560..1120`), named in
`docs/evidence/alien-ship-interior.md`. The choice is the pass's, recorded so it can be overruled.

## What this pass changed here

- Landed `tools/away/survey_sources.py` (the instrument) and this note (the rows).
- `docs/outside-the-ship.md`'s **derelict interior** row now names `scav2` and points at this survey, so
  the next pass starts from the row and not from a re-derivation.
- The **alien ship interior** row now names `scav3`, with the rows above and the re-dress in
  `docs/evidence/alien-ship-interior.md`.
