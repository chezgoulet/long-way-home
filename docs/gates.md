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

## G2 — Virtual Voyager ✅ reported working (2026-10-05)

**Reported working by the owner from his own play session** -- "single player game works great.
virtual voyager works great" -- with the decks, turbolift and station menus exercised by hand.
Awaiting his explicit sign-off, as with G1.

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

## The client's contract — three modes (agreed 2026-10-04)

Recorded in full in `docs/client-modes.md`. The client offers:

1. **the original game and all downloadable content** — retail campaign, Expansion Pack, 1.2 voice pack
2. **original LAN multiplayer** — retail Holomatch over a LAN or a VPN (Tailscale, Nebula, ZeroTier)
3. **Long Way Home** — single player (Tracks A/B/C) and multiplayer (Track D)

Two decisions that go with it:

- **Mode 2 is cMod as shipped, not our engine.** Revised 2026-10-05 under the "inherit, don't own"
  principle: our delta from upstream is the rebase cost, so mode 2 runs cMod's own client with **zero**
  delta from us, inheriting twenty years of connect-path fixes rather than re-earning them. The earlier
  "one engine, two game modules" idea is kept as an option for later, not the plan. Modes 1 and 3 are our
  engine; mode 2 is a separate application, and the two share the same game data.
- **The compatibility promise:** "full original" means compatibility of *gameplay and content* — retail
  maps, saves, configuration and demos behaving as they did — not a byte-identical binary. cMod's
  rendering and limit changes are wanted, not violations.

## G6 — retail single player, including the Expansion Pack ✅ reported working (2026-10-05)

Proven in the owner's session: the retail campaign runs, and Virtual Voyager works (G2's bar met). One
item of the bar remains unconfirmed: a save surviving a switch between modes inside the client.

The original game as a *mode*, not a stepping stone. Bar: the campaign runs; the Virtual Voyager decks,
turbolift and station menus work (G2's bar); retail configuration and saves behave; and a save survives
switching modes inside the client. Needs sessions; almost no new code.

## G7 — retail multiplayer, LAN and over a VPN 🔶 server half proven

**Server half proven 2026-10-05**, headless: cMod v1.30's dedicated server locates the retail data
(19,451 files across 5 pk3s, 2,268 shaders), loads `hm_borg1` with AAS bot navigation, and opens 32
client slots. Staged in `build/cmod`, launched by `scripts/run-cmod.sh`; evidence in
`docs/evidence/g7-cmod-staged.md`.

Still needs a human: the client's menus, the server browser, connecting to a local server, and input.
The bar is a match between two machines on a LAN and the same over a VPN address, with bots filling the
population.

**The control fault from the first playtest is no longer ours to close.** It appeared in our engine's
Holomatch path, and mode 2 now runs cMod's client, whose changelog already carries the connect-path
fixes ours lacked. The open question is narrower: whether our engine keeps a Holomatch path at all, or
whether modes 1 and 3 simply do not need one.

Agreed ordering: **G6 and G7 before G4 and G5.** They are cheap, they make the client useful now, and
they are the recruitment path for the multiplayer programme — Track D's hardest constraint is population.
G3 proceeds in parallel, because its code can be written without a session even though its criteria need one.

## G3 — reactive crew ⏳ pending

**Foundation added 2026-10-05:** the ship is a **state model**, not the maps --
see `docs/ship-model.md`. Its first artifact (the ship blob, the crew record, the system
table, the clock, and the per-deck load/unload adapter) lands inside G3, because G3's crew
records must be written into that shape from the first line of code rather than retrofitted.
Crossing decks therefore becomes a field on a crew record, and Borg assimilation becomes a state
transition on one. G3's own bar is unchanged.


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
