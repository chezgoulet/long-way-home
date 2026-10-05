# Contributing & workflow

## Branch flow

- `main` — releases only.
- `testing` — the integration branch. **All work targets `testing`.**
- `feature/<name>` — branched from `testing`, PR'd back to `testing`.
- `hotfix/<name>` — branched from `main`, PR'd to `main` and `testing`.

Note: GitHub only auto-closes an issue when a PR merges into the *default* branch. Since PRs
target `testing`, close issues by hand with a comment naming the merge commit.

## Gates

Work is organised by gate, not by calendar. See `docs/program-charter-v3.md`:

- **G0** — module builds natively + native script compiler (byte-compared against 2,175
  shipped scripts) + asset validator (with negative tests).
- **G1** — client playable: first campaign mission completed, save and reload.
- **G2** — Virtual Voyager acceptance, and no base-campaign regression.
- **G3** — reactive crew: 5–10 NPCs, one deck, no new animations, measurable criteria.
- **G4** — living ship. Does not start until G3 passes.
- **G5** — capstone scenario, judged by playtest.

Nothing downstream starts before its gate passes.

## Holding our delta from upstream: inherit, do not own

> **Revised 2026-10-05.** The ship programme (`docs/ship-programme.md`) needs engine capabilities no
> upstream will carry: a whole-ship map, live in-world panels, runtime asset replacement. The owner's
> decision: **we own the engine fork for modes 1 and 3, and keep it layered.** What follows still
> governs *how* — one concern per patch, game logic in `module/` and never in the engine, the delta
> re-measured as it grows — but a needed engine capability is no longer a reason to stop. Retail
> multiplayer (mode 2) remains cMod with zero delta.

The programme builds on stacks other people maintain — the pinned upstream port for the single-player
engine, and the community's multiplayer client for retail multiplayer. **Our position is to consume their
maintenance, and to keep our own delta thin enough to rebase onto it whenever they release.** The owner's
rule, 2026-10-04: *"I'd rather be in the best position to inherit community work than increase our support
of more stuff."*

Current delta from the pinned upstream: **53 files, +338/−133 lines, in eleven patches by concern.**

| patch | concern | why it is separate |
|---|---|---|
| `0001` | build under a modern, non-clang toolchain | flags and headers only; the sort of change upstream might take directly |
| `0002` | explicit 15-bit `rand` at every call site | mechanical, 32 files, and it exists only because the shim's macro poisoned libstdc++ headers |
| `0003` | 64-bit correctness in released source | upstream-relevant independently of us: a pointer-width write and an overlapping `strcpy` |
| `0004` | raise the content and entity ceilings | a policy choice, not a fix; ours to justify |
| `0005` | attach points for our own game logic (crew layer, ship simulation, our UI screens) | twenty-one one-line calls and a build option; inert unless `LWH_MODULE_DIR` is set, so it changes nothing on its own |
| `0006` | SP: console commands reach the game module | an upstream-relevant fix: the released game's own developer commands were unreachable in this port |
| `0007` | SP: menus that do not pause the game | our first engine capability under the revised policy: a ship console is operated while the ship runs |
| `0008` | entities to 4,096, and the bridge tables the earlier raise left at 1,024 | a limits policy like `0004`, plus a fix for what `0004` missed |
| `0009` | brush-model tables: their own limit, and the bounds check the released source lacks | the missing check is an upstream-relevant defect; the sizes are ours |
| `0010` | SP: snapshots choose what is visible and near | an engine capability: without it a map larger than one snapshot shows only its first thousand entities |
| `0011` | triggers may be boxes | a small capability in the released game source: a trigger with `mins`/`maxs` needs no brush model |

Rules that keep this shape:

- **One concern per patch, and a patch that could be offered upstream on its own.** `0001` and `0003`
  are arguably bug reports with fixes attached; `0004` is a decision. Rebasing three reviewable patches
  is a different job from rebasing one.
- **New game logic lives behind the module boundary**, in a game module the engine loads — never in the
  engine. Anything placed in the engine is a delta we own forever and a rebase cost on every release.
- **And it lives in `module/`, not in a patch.** Our own source files are compiled into the game module
  from this repository (`-DLWH_MODULE_DIR`); the patch carries only the calls into them. The direction
  layer is some 1,600 lines of ours and 51 added lines of delta. Keep that ratio: a new system gets a
  directory here and, at most, another line in `lwh_hooks.h`.
- **Where the engine must change, ask upstream first.** The multiplayer programme's engine-level needs in
  particular belong to the projects maintaining those engines, so the maintenance is shared rather than
  duplicated.
- **Re-measure the delta when it grows.** The four-patch shape is the asset; a patch series that doubles
  quietly is how a fork becomes unforkable.

## Tests

`scripts/test.sh` runs everything that needs no game data — the direction layer's unit tests, the tool
tests, lint, and the shape of the patch series — and is what CI runs on every push and pull request.
It must pass before a PR is opened.

- Logic that can be separated from the engine is, and is tested there: `module/crew/crew_core.*`
  includes no game header for exactly this reason (`tests/crew`).
- A tool gets tests in `tests/tools`, built on synthetic inputs. No test may depend on game data,
  the GDK or the upstream checkout: those checks are gate measurements (`scripts/check.sh`,
  `scripts/g3-measure.sh`) and their output is evidence, recorded under `docs/evidence/`.
- A bug fixed is a test added. The fault that motivates a check should be visible in the check.

## Evidence rules

Every claim of "done" ships with the artifact that proves it: a symbol report, a byte
comparison, a passing negative test, a log. A green build is not a green gate.

## Licensing of contributions

By contributing you agree your work is licensed GPL-2.0 for code, except inside `module/`
which follows Raven's STEF terms. Do not commit game assets — see `.gitignore`.
