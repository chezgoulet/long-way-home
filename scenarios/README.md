# scenarios/

A scenario is a directory with a `scenario.json` at its root — see `docs/scenario-manifest.md`.
It is a versioned artifact: what it contains is declared, the validator holds it to that, and the
build steps read the manifest rather than a folder of loose files.

| scenario | what it is |
|---|---|
| `deck04-watch` | Gate G3's measured scenario: six crew holding six posts on Virtual Voyager's deck 4 (`tour/deck04`). Brings no map and no assets of its own — the deck, the characters and their voices are the installation's. |

```
python3 tools/validator/validate.py scenarios/deck04-watch --data build   # check it
scripts/run-scenario.sh scenarios/deck04-watch                            # play it
scripts/g3-measure.sh --scenario scenarios/deck04-watch                   # measure it
```

To start a crewed scenario on another deck, ask the map where its navigation is and edit from
there:

```
python3 tools/crewgen/crewgen.py suggest --data build --map tour/deck05 --crew 6
```

The suggestion places crew and posts on the map's own waypoints, which the navigator can reach —
but it cannot see doors. `deck04-watch` began as a suggestion; two of its six starting positions
turned out to be behind doors that do not open for crew, which the measured run reported as
"failed to reach post" and which were then moved by hand. Measure before trusting a layout.
