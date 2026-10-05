# Long Way Home — make the ship real (Year-of-Hell mandate)

You are continuing **Long Way Home**, a project that turns *Star Trek: Voyager — Elite Force* into a
ship simulator: one ship, one crew, systems that can be run together, damage that stays, and no reset
button. A previous run implemented the earlier goals. Your job now is the one the owner cares about
most: **make the ship a thing that persists, that can be operated, that can be hurt, and that can be
lost to the Borg if the crew let them organise.**

You have full authority over implementation. Where this brief and the repo disagree, the repo's specs
win — and if you believe a spec is wrong, say so in your report and build the better thing.

## Read before writing code, in this order

1. `docs/ship-model.md` — the core design. The ship is **state, not maps**; decks are views. The save
   block, crew records, systems, compartments, threats, the clock, and the per-deck load/unload adapter.
2. `docs/ship-systems.md` — how each of Voyager's systems maps onto our model: the five-part contract,
   the canon constants, four tiers, and the places canon is silent (marked as our calls).
3. `docs/research/` — four sourced canon briefs (propulsion and power; defence and sensors; interior and
   life systems; damage and Borg) plus `game-systems-inventory.md`: what the shipped maps already expose.
4. `docs/g3-reactive-crew.md` — the crew milestone and its acceptance criteria, including the owner's
   rejection criterion: *unrealistic and unnatural actions for the characters in their environment*.
5. `docs/gates.md`, `docs/client-modes.md`, `CONTRIBUTING.md` — the gate ledger, the three client modes,
   and the integration protocol you must follow for every change.

## Where the project stands

Modes 1 and 2 are proven: the retail campaign and Virtual Voyager play on a native Linux client, and the
inherited multiplayer client serves. G3's specification exists but **verify what was actually built
before you extend it** — audit the previous run's work against the specs and fix drift rather than
assuming a clean slate. `docs/gates.md` is the authority on what is closed.

## First: prove the spine before extending it

Before new features, build a headless integration harness that proves the model's fundamental claims,
and record its transcripts as evidence:

- a save writes the ship block, reloads it, and every field survives a round trip;
- a **deck change carries the ship block** — cross from one deck to another and back, and the state
  (crew positions, system values, damage) is the same on return;
- crew records drive spawning: a crew member recorded on deck A is spawned there and **suppressed** on
  deck B, with no duplicates;
- one control changes one state, and that change is visible somewhere other than where it was made.

If any of these fail, that is the sprint. Everything else is downstream.

## The mandate

**The ship knows where everybody is, and they remember.** Every crew member is a record: identity, post,
deck, compartment, duty, shift, current order, and the few memories worth keeping. Crew cross decks by a
field changing, never by a simulated walk. A crew member hauled off a post to repair something leaves an
empty post, and something should notice.

**The ship is wired.** Every system satisfies the five-part contract in `docs/ship-systems.md` — state,
a control a person can operate, an effect visible *where it belongs*, a failure mode, and a location.
Power is the spine and the brownout ladder is the spine of the drama: *what do you turn off to keep the
shields up, and who has to decide?* Canon gives us two gifts here — life support and the holodecks have
independent power, and the holodeck matrix is canonically incompatible with the main grid, so
jump-starting from it destroys relays. Use both.

**The ship is damaged and stays damaged.** Per-deck hull, pressure, power and life support; ship-level
structural integrity, torpedoes, shuttle roster, materials; a crew roster of able, wounded and dead.
Breaches decompress. Damage spreads along the EPS graph. Uninhabitable decks displace their people. An
interruption is a pause, not a repair. Repairs cost materials *and* crew time — the same people who hold
posts — which is what makes the crew model and the damage model one system instead of two. Canon's Year
of Hell gives you a dated ledger of decay to copy (day 1, 32, 47, 65) and then erases it with a timeline
reset. **Copy the decay. Refuse the ending.**

**The Borg take the ship if you let them organise.** Boarding is a consequence of defensive failure, not
a scripted ambush. Adaptation is a per-weapon counter with a clock. Assimilation is a five-stage
transition on a crew record with a reversal window measured in minutes; past it, that record is a
hostile holding the post it used to hold. **Time-to-organise is a real timer** — start around twenty
minutes of ship time, tunable — and its progression must be *visible*: drones, systems taken, decks
lost, so the crew can see the clock they are racing. Counter-play is canon: level-10 force fields sever
drones from the Collective, explosive decompression works, remodulation works, and killing the vinculum
ends local coordination but only as a reprieve, because severed drones keep executing secondary
objectives. Ship-level loss is the Borg reaching the core.

## Questions to hold yourself to

- A player crosses from one deck to another and back. What did they leave behind, and what has changed
  while they were gone?
- A crew member is pulled off a post to seal a breach. Who is standing at that post when the player
  returns, and does anyone say anything about it?
- Power is failing. What goes dark first, who decides, and can the player overrule them?
- The Borg are aboard and nobody comes. What does the player see at minute one, minute five, minute
  twenty — and what can they still do at each of those moments?

Each question needs a measurable test, and a transcript that shows it.

## Out of scope

Do not touch the engine beyond the existing four-patch delta; new logic belongs in the game module.
Do not modify the inherited multiplayer client. Do not build new art, audio, animation or voice — reuse
what ships. Do not invent systems canon does not have: there is no point-defence grid, no Borg-versus-
hologram precedent, and no canonical brownout order. Do not build multiplayer or MMO features.

## Verification and evidence

- `scripts/check.sh` must stay green: script round-trip and the entity/validator suites.
- Build the SP module and the engine; a headless run must load a deck and spawn the player.
- Every milestone leaves a note in `docs/evidence/` with the log lines that prove it, plus the
  acceptance numbers from the spec. No claim of done without a transcript.
- Report honestly. A blocker reported is worth more than a feature invented. If something is
  impossible, say why and propose the nearest thing that works.
- Integration protocol: branch, PR to `testing`, review, merge. Never commit to `main`.

## Environment gotchas, already paid for

- **An engine rebuild wipes `build-engine/` and the module symlinks beside the binary.** Regenerate
  them, or the client silently falls back to the retail `vm/ui.qvm` and boots Holomatch instead of
  single player.
- Game data on the playtest host is **read-only**; all writes go to `build/home`. Never write into the
  installed game.
- `q3map2` needs `-fs_basepath` pointing at a directory whose child is spelled exactly `baseEF`; the
  install spells it `BaseEF` and the filesystem is case-sensitive.
- Brush winding and texture names in generated maps must match the shipped maps' convention, or the
  compiler exits 0 and produces an empty BSP. Verify the BSP's lumps with
  `tools/mapgen/check-bsp.py`, never the compiler's exit code.
- `map <name>` takes the single-player route; `spmap <name>` takes the Holomatch route and wants `.aas`
  bot data.
- The save format must emit canonical 32-bit blobs (see the ILP32 handling in `g_savegame.cpp`) or
  saves stop being portable across architectures.
- **Measure the per-head save cost at five crew before designing for thirty.** Budget target is 256
  bytes per crew member; the save block is the thing that grows.
- Persist with `gi.AppendToSaveGame('<TAG>', ptr, size)`, the mechanism the campaign already uses for
  objectives and tactical state. The four-character tag is yours to claim.

## Definition of done

The ship model exists and is proven by transcripts; crew across at least two decks are recorded,
spawned and suppressed correctly; power, doors and life support on one deck are operable and visibly
consequential elsewhere; damage from a real engagement persists across a deck change and through a save;
and a Borg boarding runs as a visible progression the crew can fight, delay, or lose to — with the save
carrying all of it. The specs in `docs/` are updated to match what you actually built, and the gate
ledger records what passed with its evidence.
