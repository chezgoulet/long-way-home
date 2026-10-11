# Engine extension policy

Decision record, 2026-10-05, by the owner: **we will extend the engine where our goals require it.**
The reasoning offered was that Raven will not be shipping updates. That reasoning is exactly right for
one half of our stack and does not hold for the other, so this record keeps the distinction and the rules
that make extension recoverable.

## The distinction

**The game module rests on dead source, and extension there is free.** The single-player game code is
Raven's released source from 2001. It is frozen; there is no upstream left to rebase onto. Game logic has
always lived there and always will — **27,883** lines of ours in `module/` (`.cpp`/`.h`), compiled in through
an attach-point patch that carries **116** added lines (`patches/0005`). Nothing about that changes.
*(**Corrected 2026-10-11 (C-19):** this read "some 3,000 lines of ours in `module/`, compiled in through a
51-line attach-point patch". Both counts were stale: measured `find module -name '*.cpp' -o -name '*.h' | xargs
wc -l` gives `27,883`; `git apply --numstat patches/0005-direction-layer-hooks.patch` gives 116 added, 1 removed.
`CONTRIBUTING.md` carried the same two figures and is corrected with this one.)*

**The engine is not Raven's, and it is alive.** We ride the lilium-voyager / ioEF lineage, and both our
pinned single-player port (`imjustadudegamer/VoyagerSP-Android`) and the multiplayer client we inherit
(`Chomenor/ioef-cmod`) were pushed within the last two months. Engine edits therefore *do* carry a rebase
cost against a maintained upstream. We are accepting that cost deliberately — not pretending it is zero.

## Why it is worth accepting

Three capabilities the programme wants were previously deferred with the note that upstream might carry
them: a whole-ship map, live in-world panels, and runtime asset replacement. With extension on the table
we can build them ourselves instead of waiting, and the crew and ship work already demonstrated that
engine changes can be small — plumbing (`cmd.c`, `sp_integration.c`) plus one attach-point header.

## The rules that keep it recoverable

1. **Game logic lives in `module/`, never in the engine.** Permanent, and the reason the module boundary
   exists. A patch carries calls into our files; it does not contain our logic.
2. **Prefer a seam to a rewrite.** An engine patch adds a hook or attach point; it does not change
   released behaviour in place. One concern per patch.
3. **Every behaviour-changing engine extension is cvar-gated and off by default.** With no cvars set, the
   client behaves and **saves exactly as retail**. This single rule is what keeps modes 1 and 2's
   compatibility promise intact *while* extending — the existing `g_crew 0` / `g_ship 0` default is the
   pattern to copy. *(**Corrected 2026-10-11 (C-18):** this read "**Every** engine extension is cvar-gated and
   off by default", an absolute the tree does not keep. A **limit raise** is not a behaviour hook and is not
   gated: `patches/0004`, `0008`, `0009`, `0010`, `0011` and `0018` raise ceilings at compile time with no cvar.
   The register named this absolute — and its restatement in `AGENTS.md` — as the wrong side.)*
4. **A save must round-trip across a behaviour-changing extension.** Changing what the simulation does must not
   make a retail save unloadable; where a behaviour change would, it is gated. *(**Corrected 2026-10-11
   (C-18):** this read "**Saves must round-trip across the extension.** Raising a ceiling or changing entity
   layout must not make a retail save unloadable. Where a change would, it is gated and that is stated in the
   patch." The two ceiling raises that break a save are **not** gated: `patches/0008` and `patches/0018` raise
   `GENTITYNUM_BITS`, and each says in its own header, **"Saves written before this patch do not load after it:
   entity numbers are persisted and their width moved."** The honest rule is the one those patches follow — a
   **limit raise is not gated, and its save cost is stated in the patch.**)*
5. **The series stays rebasable.** Patches remain a series against a pinned commit, so a future upstream
   release can be taken by rebasing — or the patches dropped — rather than by archaeology.
6. **No game content or rules in the engine.** No ship state, no crew logic, no scenario data. This keeps
   mode 2 (cMod, zero delta) uncontaminated, and keeps the boundary meaningful rather than nominal.
7. **Every engine patch records its rebase cost** in `CONTRIBUTING.md`'s delta table, together with why
   the module could not do the job alone.

## What we are giving up

Free inheritance of future upstream work: renderer and Vulkan fixes, connectivity fixes, filesystem
improvements. Probability that it costs us something material: moderate. The mitigation is rules 2, 3 and
5 — additive, gated, seam-style patches keep a future rebase mechanical rather than expensive.

## What this revises in existing documents

`CONTRIBUTING.md` currently says "where the engine must change, ask upstream first". That becomes: **offer
the generalizable fixes upstream, because they are good citizens and sometimes the fix is theirs — but
never block our work on their acceptance.** The delta table keeps counting what we carry.
