# Evidence: S6 — damage, repair and casualties, first slice

Date: 2026-10-06. Gate S6 of `docs/ship-programme.md`. This slice is in the ship core
(`module/ship/ship_core.*`) and is evidenced by `tests/ship`, which `scripts/test.sh` runs.

## What the ship now does

- **Nothing repairs itself.** Engineers on duty who stand no station are the damage-control party.
  They go to whatever is damaged, the most critical system first, up to three to a system. What
  they restore is paid for in spare parts, and when the parts are gone the work stops.
- **A deck without air injures within a minute and kills within five.** The injured stand no watch.
- **Sickbay treats six at a time**, at a pace set by its own output — so a sickbay without power, or
  without its staff, mends more slowly or not at all.
- **Sealing the hull** (`RepairDeck`) lets life support refill a deck.
- All of it is in the save (format version 2).

## What the tests establish

| property | test |
|---|---|
| A damaged system draws a party of three and no more; an undamaged one draws nobody | damage control |
| Three engineers restore a quarter of a system in half an hour, and it costs a quarter of a system's parts | damage control |
| When the work is done the party stands down and no more parts are spent | damage control |
| Of two damaged systems the critical one is manned first and both are worked | damage control |
| With a tenth of the parts, a destroyed system is rebuilt a tenth and no further | damage control |
| With no engineers to spare, damage stays and no parts are spent | damage control |
| Main Engineering open to space: everyone there is injured, none yet dead, and the warp drive's station is unmanned | casualties |
| Sickbay has as many under treatment as it has beds | casualties |
| While the deck stays open, mending people only sends them back to be hurt again | casualties |
| Hull sealed: the deck refills and the injured count falls | casualties |
| Someone hurt, with no bed and quarters on an airless deck, dies | casualties |

The third casualty row was not designed; the simulation produced it. Recovered crew returned to
their stations on a deck that was still open and were injured again, until the hull was sealed.
That is the attrition loop the design asks for, arising from the rules rather than scripted.

## Hull repair, fire and rations (2026-10-07)

The items the first slice left are built and tested (`TestHullSealFireRations`, `scripts/gaps-check.sh`):

- **Hull repair costs crew and parts.** A breached deck is sealed by the damage-control party — up to
  `REPAIR_TEAM_MAX` engineers, over `SEAL_HOURS_PER_DECK`, consuming `PARTS_PER_DECK_SEAL` — and with
  no parts the breach stays open. `RepairDeck` remains the console/damage-control seam for a
  scripted seal, but the ship's own hull is now mended by people at a price. The party's deck is set
  to the breached deck, so the crew layer has somebody to embody walking to the work.
- **Fire is a second way to be hurt.** `IgniteDeck` (every third penetrating hit in a fight starts
  one) burns a deck: it injures and can kill the crew on it (`FIRE_INJURES`/`FIRE_KILLS`), is rough
  on the hull, spreads to the decks beside it, and is put down by security and spare engineers
  (`FIRE_FIGHT_MINUTES`). It survives a save.
- **Rations.** The galley's `Stores.rations` are eaten at meals; with none and the replicators down,
  the crew's mood is measurably worse (see the morale gap's drivers).
- **Fatigue has an effect** (the morale gap, above): a spent post is weighted down in `Effectiveness`,
  so the gap's original row is closed.
- Save format **version 15**.

## A multi-day soak (2026-10-07)

S6's exit evidence includes *a multi-day soak with no stuck state*. `TestSoak`
(`tests/ship/test_ship_core.cpp`) runs a deterministic fortnight — a breach, a fire, boarders, a
damaged system, and every few days Borg and a fight, standing down afterwards — and checks the
invariants at the end of every day: no NaN clock/stores/outputs, every system's health, output and
control in `[0,1]`, every deck's atmosphere, hull, fire and assimilation in `[0,1]`, every crew
member's morale, fatigue, severity and wounds in `[0,1]` and on a real deck, and stores not negative.
The whole ship then saves and restores identically. It runs in about four seconds and passes.

## Damage is visible in the world (2026-10-07)

The mandate asked for damage *seen* in the world, not only the numbers. `SyncDamage`
(`module/crew/g_crew.cpp`) spawns the game's own `fx_spark` at a damaged system's station marker —
the same marker the crew stand at — on the deck the player is on, and puts it out when the system is
repaired or the player leaves. The ship's health decides; the world shows it; no new art, no new
class. `scripts/damage-check.sh` (`g_shipTest 34`) stands on deck 12 and damages Life Support:

```
    SHIP: damage test: standing on deck 12 at (-4512 -3584 -37744)
    SHIP: damage test: life support at 60% health
    CREW: life support is damaged; sparks at (-4352 -3840 -37768) on deck 12
PASS  a damaged system sparks where it is worked, in the world
```

A **burning deck is seen to burn**: `SyncFire` puts the game's own `fx_smoke` and (one in three) its
`fx_electricfire` across the deck the player is on, as many as `Deck.fire` is strong (to eight), and
puts them out when the fire is or the player leaves. `scripts/fire-check.sh` (`g_shipTest 35`):

```
    SHIP: fire test: standing on deck 12 at (-4512 -3584 -37744)
    SHIP: fire test: deck 12 alight at 60%
    CREW: fire on deck 12 (60%); 5 effects in the world
PASS  a burning deck is embodied as smoke and flame across the deck
```

Per-section rather than per-deck placement remains.

## What S6 still needs

- **More injury causes.** Air, fire, fighting wounds, **radiation** (a failing warp core below half
  health irradiates the engineering watch on deck 11, injuring and, at length, killing them;
  `TestRadiation`), an **exploding console** (a hit on a deck hurts whoever was manning a station
  there), and a **poisoned site** (beaming an away team to a phenomenon or a belt hazards it). The
  last two: `TestHazardInjuries`. Weapons fire *outside* combat, and other environmental hazards,
  remain for later gates.
- **Trade and salvage** are the outside loop's (S9); the derelict salvage exists (S9).
- Every rate here is invented and is in `docs/lore-ledger.md`.
