# Confidence and verification: what to trust in this repository, and how far

The rule this file exists to enforce: **a claim's confidence is set by how many independent witnesses it
has, not by how plausible it looks.** Plausible-but-wrong is the failure mode this project is most exposed
to, because the subject is Star Trek and every model knows Star Trek.

## The claim classes

| claim class | witnesses | confidence | how to re-check |
|---|---|---|---|
| shipped content inventory — deck maps, model families, entity counts, lift edges | map sources + paks + code | **90-95%** | re-run the censuses in `docs/research/game-alien-roster.md` and `docs/vv-gap-list-verified.md` |
| load-bearing canon numbers — warp 9.975, 141 crew, fifteen decks, 4000 teradynes/s, fourteen arrays | Memory Alpha, re-verified by hand | **90-95%** | the URL sits beside each number |
| room-by-room canon deck contents | one wiki deck list | **~85% per line** | spot-check the specific line before it carries weight |
| contested canon — the second holodeck's deck; rooms never seen on screen | none | **labelled, not guessed** | decide and record in the brief |
| factions the game never names -- `imperial`, `stasis`, `avatar` | none | **labelled unknown** | use as unknown species; never assert their identity |
| our own design decisions -- the brownout ladder, the organise timer, the reversal decay | owner-approved, marked **[our call]** | by decision | the docs mark them; do not re-derive them |

## What comes back from a dispatched build

| task | first-pass confidence | what moves it |
|---|---|---|
| a standard room re-dressed from an existing map | **~90%** | the structural gates catch its failure modes |
| a bespoke room from a filled brief, blockout gate enforced | **~70-75%** | blockout approved before any detail |
| the same brief, blockout gate skipped | **~50%** | plausible-but-wrong geometry survives to the end |
| a whole deck with no copy target | **~45%** | pick a copy target or expect drift |
| a returned **prose claim** about behaviour | **~70%** | require transcripts; a claim is not evidence |
| a returned **transcript** -- compile, BSP structure, headless load, `.nav` | **~90-95%** | machine-checkable and cheap to re-run |

## The dominant risk

**Model prior overriding the repository.** An LLM knows Star Trek and will confidently improve on canon.
Unconstrained, the probability that an authoring task introduces at least one unsourced canon detail:
**~60-70%**. With the rules in `docs/authoring-a-location.md` -- a source line per canon claim or an
explicit "invented", and contested placements decided and recorded -- **~20-25%**.

Second risk: **our own verification coverage.** Structural accuracy is cheap to check and roughly 95% of it
gets caught. Lore accuracy is expensive per room, so review alone catches about half.

## Therefore, the dispatch shape

Two or three neighbouring rooms dispatched and verified end to end **before batching** -- because per-room
verification, not agent throughput, is what breaks at scale. The same reason the save cost was measured at
five crew before anything was designed for thirty.
