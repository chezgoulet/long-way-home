# Evidence: S7 — intruders and control of the ship's systems, first slice

Date: 2026-10-06. Gate S7 of `docs/ship-programme.md`. This slice is the rules, in the ship core
(`module/ship/ship_core.*`), evidenced by `tests/ship`. In the game it is reachable from the
console (`ship board <deck> <n>`, `ship counterhack <system> <0..1>`); nothing is embodied.

## The rules

- **Boarders** arrive on a deck. Those not pinned by defenders work on the systems stationed there,
  and each system's **control** falls. Below half, the system is **hijacked**: it still draws the
  ship's power but delivers nothing to her, and it refuses its console.
- **Control comes back** three ways: the station's own crew win an uncontested system back in
  twenty minutes; a **counter-hack** returns up to half at a stroke; and with no boarders left
  there is nobody to hold it. **Cutting a system's power freezes it** for both sides.
- **Security** on duty with no station goes to the boarded decks, the worst first, enough to
  outnumber each party. Each side wears the other down at the same rate; defenders take wounds and
  are out of the fight, injured, when they have taken enough.
- **Boarders with nothing left to take move on**, a deck at a time, toward the bridge or Main
  Engineering, whichever is nearer.
- **Venting a deck kills boarders** as it kills crew.
- **The breach puzzle** (`MakeBreach`, `BreachScore`): a 5x5 grid of codes, three target sequences
  of two, three and four codes worth one, two and three, and a buffer of seven picks made down a
  column then along a row, alternately, from the top row. Its score is the counter-hack's strength.

## What the tests establish

| property | test |
|---|---|
| Two unopposed boarders take the sensors in under five minutes; systems on other decks are untouched | boarding and control |
| A hijacked system draws power, delivers nothing, and ignores on/off and priority from the console | boarding and control |
| A counter-hack frees it; with the boarders still there it is taken again | boarding and control |
| With its power cut, control does not move in ten minutes | boarding and control |
| Security arrives in numbers greater than the party, clears two boarders within six minutes, and takes wounds doing it; the boarders, pinned, never get to work | boarding and control |
| The station's crew restore a system left at 20% within half an hour, and security stands down | boarding and control |
| Four boarders on deck 3 with no security aboard reach the bridge and take shields and communications within two hours, none lost on the way | boarding and control |
| A vented deck has no boarders left alive on it | boarding and control |
| Hijack state and boarders survive a save | boarding and control |
| For forty seeds: every puzzle can be solved in full (found by exhaustive search), and the same seed is the same puzzle | the breach puzzle |
| Illegal paths score nothing: not starting in the top row, the wrong direction, a reused cell, too many picks, a cell off the grid | the breach puzzle |

Two outcomes came from the rules rather than from the tests' first expectations. Four defenders
against two boarders win without anyone being put out of the fight — wounded, not injured. And
boarders on a vented deck do not all die: those who had already taken the torpedo bay had moved on.

## What S7 still needs

- **Bodies: done for the player's deck** (below). Still to do: the ship's own security fighting
  them as bodies too, rather than by a rate, when the player is there to see it.
- **The puzzle on a screen: done** (`scripts/s9-check.sh`): any console's countermeasures key asks
  the ship for a puzzle for the selected system and presents it; the picks are sent back and scored
  by the ship. It runs against a thirty-second trace: when time is up, what has been entered is what is sent. Nobody has played it by hand.
- **Who the boarders are.** One kind, one behaviour. Species, weapons, objectives: later.
- **How they arrive** — transporters, breaching pods — belongs with the outside loop (S9).
- Every rate is invented and in the lore ledger.

## Boarders as bodies (later the same day)

With `g_crewFromShip` on, the boarders the ship counts on the player's deck are hostile NPCs there
(`SyncIntruders`, `module/crew/g_crew.cpp`): raiders are the game's Klingons, drones its Borg, up to
six at once, placed away from the player. The tie runs both ways. `scripts/s7-check.sh`:

```
    boarder body lwh_boarder_000: type Klingon2, team 4, hostile to team 1
    boarder body lwh_boarder_001: type Klingon2, team 4, hostile to team 1
    boarder body lwh_boarder_002: type Klingon, team 4, hostile to team 1
    boarding test: 3 bodies for 3 boarders the ship counts
    a boarder is down on deck 11; the ship counts 2 left there
    boarding test: after one was killed the ship counts 2
PASS  three raiders board, are embodied hostile to the crew, and one killed is one fewer aboard
PASS  three Borg drones board, are embodied hostile to the crew, and one killed is one fewer aboard
```

Team 1 is Starfleet. That the raiders are Klingons is a stand-in — they are the hostile humanoids
the game ships — not a statement about who boards Voyager.
