# Proposal — Native Linux Client for Star Trek: Voyager — Elite Force (Singleplayer)

Date: 2026-10-04
Prepared for: Christopher Goulet
Status: proposal, awaiting go/no-go

---

## 1. Recommendation

**Build it, as a desktop target added to the existing open-source singleplayer port rather than as new
work from the source code.**

The reason for confidence is simple: the hard part has already been solved, shipped, and released four
times this year. `VoyagerSP-Android` (v1.2.0, released 2026-07-18) is a working singleplayer Elite
Force client built from Raven's released singleplayer source, and it is described by its own author as
"over 90% complete" and "not yet fully play tested". Its architecture is exactly the architecture a
Linux desktop client needs. What remains is platform work — build system, the platform seam, and one
pointer-width audit — not invention.

**Probability of a playable native Linux singleplayer client: 75–85%.** The failure modes are ordinary
build and integration problems, and they are bounded by an existing, readable implementation. The
residual uncertainty is concentrated in two places: the x86_64 pointer-width audit (a known, catalogued
class of error), and savegame/cinematic edge cases.

**Parallel, no-regret move:** while this is built, running the GOG build under Wine/Proton gets
Starbase 11 playable this week. The two efforts don't compete; the Wine path proves the content, the
native path makes it permanent.

---

## 2. Why this is a port and not a research project

Four prior works, each independently verifiable:

**Raven's singleplayer source code** — officially released, under Raven's measured licence (see §9).
This is the game logic: NPC AI, weapons, objectives, savegames, and the ICARUS scripting system.
It is public and it is what makes this possible at all.

**Elite-Reinforce** (`kugelrund/Elite-Reinforce`) — a cleaned-up fork of that source with bug fixes and
speedrunning improvements, structured as a modern C++ project (CMake/Visual Studio). This is the tree
the Android port actually builds from.

**VoyagerSP-Android** (`imjustadudegamer/VoyagerSP-Android`) — the reference implementation. Its
repository contains, in three clearly separated pieces:

- `efgame/` — the singleplayer game module, built as two shared libraries that export only
  `GetGameAPI`, `vmMain`, `dllEntry` and `GetUIAPI`, with *zero* unresolved symbols. Documented build
  status: all 152 translation units compile for 32-bit ARM; 64-bit ARM has 245 `pointer→int`
  truncation errors across 24 files, and those errors are catalogued in the repository as a work list.
- `EFAndroid-SP/app/jni/efcode/` — the engine: lilium-voyager (an ioquake3 lineage engine that already
  targets Windows, macOS **and GNU/Linux**) plus a Vulkan renderer ported from Quake3e. It includes
  `sys/sys_unix.c` — the Unix platform layer is already there.
- `EFAndroid-SP/app/jni/sp/` — the integration layer that binds the engine to the singleplayer module:
  `sp_bridge.cpp`, `sp_ui_bridge.cpp`, `sp_integration.c`, plus `bink_ff.c`/`bink_ff.h`, which decodes
  the game's Bink cutscenes through FFmpeg.

**lilium-voyager** — the engine upstream, which states GNU/Linux support as an explicit goal in its own
README and ships SDL2, an OpenGL1 and OpenGL2 renderer, and the Unix system layer.

### The decisive detail

The Android port's compatibility work is deliberately confined. Its game-module README says the
compatibility layer exists "so the source's `#include <windows.h>` etc. resolve to the compat layer
**on Linux**", and the shims it names — Windows typedefs, MSVC string intrinsics, calling-convention
no-ops, `random()` renaming, stdio-backed stand-ins for the ICARUS tokenizer's Windows file API — are
exactly the Linux problems, already solved and isolated into one directory.

And the platform seam in the integration layer is thin: the only Android coupling in `sp_bridge.cpp`
and `sp_integration.c` is `#include <android/log.h>` and calls to `__android_log_print`. That is the
whole seam. On desktop it becomes a two-line logging shim.

This is what a port looks like when someone has already done the hard thinking.

---

## 3. Scope

**In scope**
- Native x86_64 Linux client, singleplayer campaign, built from source, no Wine.
- Vulkan and/or OpenGL rendering on the desktop, at modern resolutions.
- Keyboard/mouse and gamepad input, audio, savegames, cutscenes.
- Loading the retail game data from an existing install or GOG copy.
- Running community singleplayer mods (the acceptance test is Starbase 11).

**Out of scope**
- Multiplayer. Holomatch already has a mature native Linux client (cMod); we will not duplicate it.
- Multiplayer mod support (RPG-X stays on the Holomatch stack, where it belongs).
- Windows or macOS targets. They already work via the retail/community clients.
- Redistributing any game data, ever. This builds the engine and modules; the assets remain yours.

**Uncertain**
- The expansion pack (Virtual Voyager) — upstream has a closed issue about its turbolift menu, which
  suggests partial support. Treat as a milestone-4 investigation, not a promise.

---

## 4. Architecture

```
  ┌──────────────────────────────────────────────────────────┐
  │  efgame  (singleplayer game module)                      │  STEF licence
  │  game + cgame + icarus + ui, built as shared libraries   │  (Raven, non-commercial)
  │  exports: GetGameAPI / vmMain / dllEntry / GetUIAPI      │
  └───────────────────────────┬──────────────────────────────┘
                              │  dlopen, function-pointer tables
  ┌───────────────────────────┴──────────────────────────────┐
  │  engine  (lilium-voyager lineage + port layer)           │  GPLv2
  │  client / server / qcommon / sys_unix / sdl / renderer   │
  └──────────────────────────────────────────────────────────┘
                              │
  ┌───────────────────────────┴──────────────────────────────┐
  │  integration layer (sp_bridge, sp_ui_bridge, bink_ff)    │  port code
  │  the only Android-specific lines: android/log.h          │  → replace with a log shim
  └──────────────────────────────────────────────────────────┘
```

The two licences stay separated by construction: the game module is a shared library the GPL engine
loads at runtime, exactly as the retail game loaded `efgamex86.dll`. Nothing is statically linked
across the boundary.

---

## 5. Milestones, exit criteria, and how each one is proven

Every milestone has an artifact that either exists or doesn't — no milestone is "it seems to work".

**M0 — Game module builds for x86_64 Linux.**
Replace the Android toolchain with a native one; reuse the existing CMake structure (its source lists,
exclusions and export version scripts are already correct and documented).
*Proof:* `libefgame.so` and `libefui.so` exist for x86_64; `nm -D` shows only the four intended exports;
no unexpected unresolved symbols. **Effort: days.** This is the known work item — it is the same class
of error as the catalogued 64-bit ARM audit.

**M1 — Engine builds and runs on x86_64 Linux.**
Build the lilium-lineage engine with its existing Unix/SDL2 path; confirm it starts, reads config, and
opens a window.
*Proof:* engine binary launches and logs its version; a window opens (software rendering acceptable at
this stage — see §8). **Effort: days to one week.**

**M2 — The engine loads the singleplayer module and reaches the main menu.**
Replace the Android log seam with a desktop shim; wire the module load path; get past initialisation.
*Proof:* engine log shows the module loading; the LCARS main menu renders. This is the milestone with
the most unknowns — this is where the Android port's assumptions about its host get tested.
**Effort: one week, with a real chance of two.**

**M3 — Playable.**
Rendering at modern resolutions, keyboard/mouse and gamepad input, audio, save and load.
*Proof:* the first mission of the base campaign is completed end-to-end from a clean start, with a save
written and reloaded. **Effort: one to three weeks.**

**M4 — Content acceptance, including mods.**
The bar that matters for the actual goal.
*Proof:* the base campaign starts; the expansion content is loaded (or its absence is documented
precisely); Starbase 11 installs and its maps load without asset errors. **Effort: one to two weeks.**

**M5 — Packaged and pleasant.**
Cutscenes decoding (the FFmpeg Bink path already exists upstream), sensible defaults, a desktop entry,
an AppImage or equivalent so it installs like software rather than living in a build directory.
*Proof:* a single artifact that runs on a clean machine with only the game data added.
**Effort: about a week.**

**Total: roughly four to eight weeks of focused effort**, with M2 and M3 carrying most of the variance.
These are ranges, not commitments; I will report actual progress against them rather than defend them.

---

## 6. Risks

**x86_64 pointer-width errors (likely, low impact).** The 64-bit ARM build hit 245 `pointer→int`
truncations in 24 files, and x86_64 is LP64 — the same class of problem is expected. Mitigation: the
upstream repository already catalogues the affected files for ARM64, so the audit has a starting list
and a pattern rather than a blank page. This is mechanical work with a compiler that flags each one.

**Integration assumptions in the bridge (moderate, medium impact).** The `sp_bridge` code was written
against Android's lifecycle and surface handling. Desktop exposes different failure modes (window
creation order, context loss, audio device negotiation). Mitigation: isolate and instrument early;
the bridge is a small, readable file, not a mystery.

**Savegame compatibility (moderate, low impact).** Upstream documents that saves may occasionally fail
and that retail PC saves are not yet compatible, with a transcoder already written for width
differences. Mitigation: accept non-portable saves in v1; treat retail-save import as a stretch goal.

**Renderer choice (low, medium impact).** The Android port ships Vulkan only, because Android demanded
it. On desktop, lilium's OpenGL renderers are more battle-tested. Mitigation: build OpenGL first for
the smoke test, add Vulkan afterwards. Having both is a strength, not a fork.

**Expansion pack support (uncertain).** Mitigation: verify early in M4 and report plainly rather than
discovering it late.

**Build host capacity (certain, low impact).** The build machine runs an existing build runner but its disk
has been chronically near-full; this project needs a few tens of gigabytes of headroom for toolchains,
build trees, and the game data (pak0 alone is ~541 MB). Mitigation: clear space first, or build on the
other Linux box (4 cores, 22 GB free) and accept slower builds.

---

## 7. Where it runs

Development and CI on the Linux build host, using the existing runner for repeatable builds and the
headless smoke test in §8. The finished client runs on the desktop Linux machine, which is the only
place that can verify the experience — I can prove the build, the module loading, and the logs
headlessly; I cannot prove it plays well.

## 8. Testing approach

- **Compile and link gate:** exported symbols exactly as intended, zero unexpected unresolved symbols.
- **Headless smoke test:** the client can be started under a virtual display with software rendering and
  driven far enough to produce a menu frame and a log. This is real verification, not a substitute for
  playing, and it is automatable in CI on the build host — which has no GPU.
- **Content assertions:** scripted level loads that fail loudly on missing assets, run across the base
  campaign and each Starbase 11 map.
- **Human acceptance:** the only test that counts for feel — frame pacing, mouse feel, audio mix — done
  on the desktop, by you.

## 9. Licence posture

Two licences, two artifacts, cleanly separated:

- **Engine: GPLv2** (id Software → ioquake3 → lilium-voyager → Quake3e). Modifications to GPL code
  carry source obligations *on distribution*. For personal use, nothing is triggered.
- **Game module: Raven's source licence** (November 2002). It grants the right to create modifications
  and to distribute them free of charge for non-commercial purposes, and forbids sale, rental,
  commercial exploitation, and reverse engineering of the shipped game. Building and running a port at
  home sits squarely inside it. Upstream flags the same nuance: fine to build and port locally; shipping
  the module inside a GPL binary is a separate distribution question, which is why the module stays a
  separate dynamically loaded file.
- **Game data is never redistributed.** The client requires a copy you own, as upstream does.

## 10. Upstream strategy

**Recommended: develop as a branch against upstream, and offer the desktop target back.**

Rationale: the engine half is GPL, so a distributed fork would carry source obligations anyway; the
maintainer is active (four releases in mid-2026) and already thinking about other titles from the same
engine family; and a merged desktop target survives us. Practically this means keeping our changes as a
reviewable patch series — a Linux CMake target, the log shim, the pointer-width fixes — rather than a
package of local edits. If the maintainer declines, we keep the branch and lose nothing.

This also matches how the durable projects in this game's community have worked: they were contributed
to, not held privately, and they outlived their authors' attention spans.

## 11. Decisions required

1. **Go/no-go** on the native build — and whether to run the Wine/Proton path in parallel in the
   meantime (recommended: yes, it costs nothing and de-risks the wait).
2. **Upstream-first or private branch** (recommended: upstream-first, local branch as the working area).
3. **Build host** — clear space on the build machine, or build on the other Linux box.
4. **Scope of "done"** — base campaign only, or the expansion and Starbase 11 in the acceptance bar
   (recommended: the latter; it is the actual point of the exercise).

## 12. Immediate first step, if approved

Clone the upstream repository, run the existing 64-bit ARM audit tooling against x86_64, and build the
game module natively. That is M0: a days-long task producing two shared libraries and a symbol report —
the first hard evidence that the rest of this proposal is sound. Nothing about it depends on decisions
2 through 4.
