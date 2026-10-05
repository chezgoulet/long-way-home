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

## What S6 still needs

- **It is not in the game beyond the numbers.** The Engineering console shows "UNDER REPAIR" in the
  status report, but nobody is seen walking to a damaged system, and nothing in the world breaks.
- **Hull repair has no crew or cost yet** — `RepairDeck` is a bare call.
- **Injury has one cause** (no air). Weapons fire, fire, radiation, intruders: later gates.
- **No triage, no medical supplies, no rations, no trade or salvage** — the wider economy.
- **Fatigue still has no effect.**
- Every rate here is invented and is in `docs/lore-ledger.md`.
