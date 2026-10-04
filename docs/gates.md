# Gate ledger

Status of the programme's gates. The charter defines *what* each gate requires; this records *whether*
it passed, on what evidence, and what is still open.

---

## G0 — toolchain ✅ closed 2026-10-04

| item | result | evidence |
|---|---|---|
| single-player game modules build natively | 157/157 units, both modules, four intended exports only | `docs/evidence/g0-native-module-build.md` |
| native ICARUS script compiler | 2,394 of 2,408 corpus files compile clean; 11 are not scripts; 3 rejected with named causes | `docs/evidence/g0-script-compiler.md` |
| round-trip verification | 2,408 of 2,408 compiled scripts read back by the game's own reader | `docs/evidence/g0-roundtrip-and-dictionary.md` |
| scenario validator | six fault classes each caught *and named*; run against all 106 published map sources | `docs/evidence/g0-validator.md` |
| entity dictionary | 262 classes + 2 family templates; 237 of 239 used classes covered | `docs/evidence/g0-roundtrip-and-dictionary.md` |

Reproduce with `scripts/bootstrap-upstream.sh` and `scripts/check.sh`.

## G1 — client playable ✅ closed 2026-10-04, signed off by the owner

| milestone | result | evidence |
|---|---|---|
| M1 engine builds and runs | native Linux build; retail data loaded (21,675 files) | `docs/evidence/g1-engine-native-build.md`, `g1-engine-loads-game-data.md` |
| M2 engine loads our modules | `Lilium Voyager SP`, SP UI loaded, SP commands registered, `SP_LoadGame: apiversion=6` | `docs/evidence/g1-sp-engine-starts.md`, `g1-sp-mission-plays.md` |
| M3 playable | campaign map `borg1` spawned through the SP path; player connected; save written mid-mission and reloaded; clean exit | `docs/evidence/g1-sp-mission-plays.md` |

Signed off by Christopher, 2026-10-04, on the mechanics being demonstrated plus his own play session.

## G2 — Virtual Voyager ⏳ next

Content confirmed present rather than assumed: the expansion pak carries **32 maps including
`maps/tour/deck01`–`deck04` and beyond** — the VV decks — plus 274 turbolift/virtual-voyager assets.

Acceptance bar (from the charter, plus the owner's addition):

- the decks load and the turbolift menu moves between them
- the station's menus open: Library, Astrometrics, Personal Log, Medical Log, Recipes, Social
  Calendar, Disease Library, Shooting Range, Weapon Library, Cargo, Engineering Library
- holodeck and shooting range enter and exit cleanly
- a save taken inside Virtual Voyager reloads correctly
- **no regression in the base campaign**

Testing needs a logged-in desktop session: under a virtual display the renderer reports
`failed to find graphics queue family` and the load never reaches the SP path.

## G3 — reactive crew ⏳ pending

5–10 NPCs, one deck, no new animations, reused barks, posts and acknowledgement, with the measurable
criteria in the charter (post coverage over a sampled run, bounded time to post, zero navigation
failures, save/load restores posts and schedules, an explicit per-NPC save-size budget, no ICARUS
script regressions).

**Arbitration precedence must be written before any Track C code exists** — scripted sequence >
direct combat/reaction > director override > duty/routine > idle/social.

## G4 — living ship ⏳ pending, does not start until G3 passes

## Adopted from RPG-X prior art (2026-10-04)

Four decisions recorded in `docs/prior-art-rpg-x.md`, each landing somewhere concrete:

- **rank and permission model → Track D** as the starting design for ship authority
- **embedded SQLite persistence → Track D and G3** as the storage pattern
- **emote / interaction vocabulary → capstone and G4**
- **content headroom → Track B, before capstone content work**: `MAX_CONFIGSTRINGS` 1024 against their
  4096, `MAX_GENTITIES` 1024 against 2048, `MAX_MODELS` 256 against 512 — with the two constraints the
  source states (models and sounds ride the network as 8 bits; configstrings and
  `MAX_GAMESTATE_CHARS` must move together). First task: measure configstring usage on a loaded
  campaign map, so we know how much of the budget retail already spends.

## G5 — capstone scenario ⏳ pending

---

## Residuals (named, queued, not forgotten)

- **~30 `Cmd_AddCommand: … already defined`** on each level transition. Harmless, but it means the SP
  path's console registrations are not idempotent.
- **`MAX_PACKET_USERCMDS`** printed twice at shutdown. Cosmetic.
- **Dependencies no longer live in `/tmp`** — the tree is in `build/deps`, on the binary's RPATH, and
  `ldd` with no environment set reports zero missing libraries. Closed.
- ~~**No cutscene playback.**~~ **Closed** — FFmpeg is available from the plain archive with the version
  pinned (the ESM build was merely the preferred candidate), and the engine now links Bink playback.
  See `docs/evidence/deps-durable-and-cutscenes.md`. Whether a cutscene *plays* still wants a session.
- **126 script references the GDK corpus lacks** — the validator found them; most likely the
  expansion's scripts. Worth resolving before G2 content work.
- **Retail PC saves are incompatible** with the port (upstream documents this). Start new campaigns.
