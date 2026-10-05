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

Reproduce with `scripts/bootstrap-upstream.sh`, `scripts/fetch-gdk.sh` and `scripts/check.sh`.

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

## G3 — reactive crew 🔶 measured, awaiting the owner's judgement

**Implemented and measured 2026-10-05.** Six crew hold six posts on `tour/deck04` through a ten-minute
headless run: 100% coverage at all 121 samples, every post reached within 6.1 s, no stuck or
out-of-world events, every address acknowledged within 1.5 s, the deck's four scripted characters
undisturbed, a script run on a post-holder honoured and the member returned to duty, crew state
identical across save and reload, 25 bytes of save per crew member against a budget of 256.
Evidence: `docs/evidence/g3-reactive-crew-measured.md`; reproduce with `scripts/g3-measure.sh`.

What is left is the owner's:

- **Does the deck feel inhabited?** `scripts/run-scenario.sh`, in a logged-in session.
- **Is adding crew acceptable for this gate?** None of deck04's own eight NPCs can be given a post —
  four are permanently scripted, four are seated props spawned in solid — so the scenario declares
  six crew of existing character types and the layer spawns them. No new art, voice or animation;
  but the charter says "named crew already placed", and that is not what was measured.
- **Frame time on the target desktop.** Measured here as time inside the game frame (0.34 ms with the
  crew, 0.24 ms without); a frame rate needs a real renderer.

The layer is `module/crew/`, attached by `patches/0005`, authored through the manifest's `crew`
section. It is off by default (`g_crew 0`), and with it off the module behaves and saves exactly as
before. Arbitration precedence — scripted sequence > direct combat/reaction > director override >
duty/routine > idle/social — was written before the code, as required, and is unit-tested.

## G4 and G5 — replaced by the ship programme (2026-10-05)

The owner redirected the programme: the ship must work as a system before a crew can be given lives
aboard it. `docs/ship-programme.md` records the decisions and the gates S1–S10 that replace G4 and
G5. G3 stays open and work proceeds regardless, by his decision.

| gate | state | evidence |
|---|---|---|
| **S1** ship core | ✅ done 2026-10-05 | `docs/evidence/s1-ship-core.md` |
| **S2** Engineering console in game | 🔶 built and verified headless; awaiting the owner at the console | `docs/evidence/s2-engineering-console.md` |
| **S3** whole-ship map | 🔶 the ten decks stitch, compile, load and run as one map (game frame 3.1 ms average); lit and with visibility; every deck is drawn when stood on; not yet walked, or joined by working turbolifts | `docs/evidence/s3-merged-map-measured.md` |
| **S4–S10** | ⏳ pending | |

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

- ~~**The Game Development Kit was no longer on the playtest host**~~ **Closed 2026-10-05** — it had
  been extracted under `/tmp` and was lost. `scripts/fetch-gdk.sh` now restores it into `build/gdk`
  from the Internet Archive, checksummed, and G0 was re-run against it: validator negative tests all
  pass; dictionary 262 classes + 2 templates, 237 of 239 used classes covered across 106 map sources
  (unchanged); corpus 2,016 of 2,024 scripts compile and read back, the 8 rejections being the three
  named in `docs/evidence/g0-script-compiler.md` plus five files that are not scripts. This archive's
  script set is smaller than the 2,408 files counted originally, which included the GDK's own copies.
- **The entity dictionary's flags and keys were wrong until 2026-10-05** — a header pattern ran on
  into each description. Class names, and so G0's coverage figures, were unaffected. Regenerated
  from the restored GDK: 211 classes with documented keys, 232 with spawnflag sets.
- **Crew cannot open doors.** Two suggested starting positions on deck04 were one waypoint from their
  posts and unreachable. Scenario layouts must be measured, and G4's schedules will meet this.

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
