# Evidence: S8 — the Borg, first slice

Date: 2026-10-06. Gate S8 of `docs/ship-programme.md`. This slice is the rules, in the ship core,
evidenced by `tests/ship`. In the game it is reachable from the console (`ship borg <deck> <n>`).

## The rules

- **Drones convert the deck they hold.** `assimilated` rises with the number of drones not pinned
  by defenders — one drone alone takes an hour over a deck. Past half, the deck's systems are theirs
  outright: control is forced to nothing, no console reaches them and no counter-hack either.
- **They take the crew they find**, one each ten minutes, and each crew member taken is another
  drone. Those taken do not come back.
- **Driving them off does not undo it.** The deck stays Borg, and its systems stay lost, until
  engineers strip it: eight engineer-hours and twenty spare parts for a wholly assimilated deck, up
  to four engineers at once, and only after the drones are gone. Without parts it stays as it is.
- **Security pins them** as it pins any boarders; pinned drones convert nothing.

## What the tests establish

| property | test |
|---|---|
| Two drones on an undefended deck: assimilation under way within a quarter of an hour, crew taken, and the number of drones grown by exactly those taken | the Borg |
| Within the hour the deck is assimilated and its system is at zero control; a counter-hack does nothing | the Borg |
| Vented, the drones die and the deck is still Borg, its system still lost | the Borg |
| Sealed and stripped: engineers go to it (four at most), assimilation falls and parts are spent; a day later it is clean, the system answers again, and the parts spent equal the deck's share of twenty | the Borg |
| The assimilated are on no deck | the Borg |
| With no parts, a deck stays 80% Borg after a day | the Borg |
| With security aboard, two drones are cleared having converted almost nothing | the Borg |
| All of it survives a save | the Borg |

## What S8 still needs

- **To be seen.** The owner asked for the ship's assets to *turn Borg* and be stripped back. That
  is replacing a section's textures and models at run time by `assimilated`, which is an engine
  capability that does not exist yet. The number that would drive it does.
- **Drones as bodies**, and assimilated crew appearing as drones with their own faces.
- **Per-section rather than per-deck.** A deck is the unit; the request was "parts of the ship".
- The Borg adapting, regenerating, a cube outside: S9 and beyond.
- Every rate is invented and in the lore ledger.
