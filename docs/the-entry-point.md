# The entry point: how a run begins, and where it is chosen

**The scope for the walkthrough's G1 and G2.** G1: *there is no way to start Long Way Home from the game's
menus.* G2: *the opening situation does not exist.*

**They are one piece of work, and the design says so in one line:** *"The menu grants the situation. The
fiction supplies the reason. The simulation then holds you to it."* (`docs/start-states.md`) **The surface
that starts the game is the surface that chooses the start state.** Splitting them would build the menu and
then rebuild it.

This document owns **the entry point**. `docs/client-modes.md` owns the three modes; `docs/start-states.md`
owns the opening's design; the walkthrough (`docs/walkthrough.md`) owns the findings.

## Part one — G1, the way in

### What is actually missing

The only launch is a developer line:

```sh
scripts/run-engine.sh +set com_hunkMegs 768 +set g_ship 1 +set g_shipDeckPitch 3072 +map voyager
```

Nine tokens, five of which are `+set`. `g_ship` defaults to **0**, so the game is off unless someone knows the
cvar. **A person with no knowledge of the code cannot start it.**

### What has to exist

1. **A launcher** — one thing that sets the cvars and loads the map.
2. **A "Long Way Home" line on the main menu.**

And the mode contract made visible: `docs/client-modes.md` names **three modes** — the retail campaign, retail
multiplayer, and Long Way Home. **The menu is where that contract becomes a choice rather than a document.**

### The forks, with a recommendation

**The cvars stay off by default. The menu is the switch.** `AGENTS.md` requires every engine extension to be
cvar-gated and **off by default**, keeping retail behaviour and saves intact. So the menu entry *sets* the
cvars for the session; it does not change their defaults. Changing `g_ship` to 1 would boot the ship for
someone who wanted the campaign.

**The menu is the one surface where a broken build looks correct.** `AGENTS.md` records the trap in its own
words: *an engine rebuild wipes the module symlinks beside the binary, which silently boots the retail menu
instead of our code.* **So the acceptance has to read the rendered menu and the ship that loads behind it — a
green build proves nothing here.** This is the item on the list most likely to be signed off on a false green.

**Mode 1 must be re-proven, not assumed.** The contract is that the retail campaign is *unmodified in
behaviour*. A new main-menu line is exactly the kind of change that can disturb it. **So the acceptance
includes the retail campaign still starting and playing**, and it is not a formality.

**`g_shipDeckPitch 3072` is a magic number in a launch line.** It should not be a thing a person types. Either
it earns a default or it belongs in the launcher's own config — **say which, and record the value's source.**

### Acceptance — G1

- **The ship loads from the menu, with no console and no cvars typed**, demonstrated **from the rendered menu**
  and not from a build log.
- **The retail campaign still starts and plays** — mode 1's contract, re-proven.
- **Retail multiplayer is unaffected.**
- The cvars are **still off by default** with the game unset; only the menu sets them.
- **The launcher's line is the only place the values live** — no second copy to drift.

## Part two — G2, the opening

### What is actually missing

`docs/start-states.md` designs the opening as a **situation**: a casualty list, vacancies **derived** from it,
the Caretaker aftermath, a first log entry, and variants from ensign to captain. **What ships is the whole
canonical default only**: 141 crew, day 0 08:00, condition green, intact chain of command. The design exists;
none of it is implemented.

### And it is cheap, for a reason worth stating

**"A start state costs almost nothing to author. It is not bespoke content; it is a different initial
condition on the same systems."** A start state is **data**: `casualties[]`, the vacancies **derived** from
them, the player's record and post, the ship state, and a `log_seed`.

**That derivation is the load-bearing requirement, and it is not a detail.** Vacancies must be produced by
**the same rules that run during play** — never hand-written. Hand-writing them would make a start state and a
mid-run loss behave differently, which is exactly the divergence *saves must replay identically* will catch
later, and it would put two sources of truth on one rule.

### Where the selector lives

**On the same menu as G1** — the design's own line, and the reason these two are one scope. The menu grants
the situation.

**And it must compose with character creation (S10, built).** The player already chooses a character and a
role. **So define the interface: the start state chooses the *situation*; character creation chooses the
*person*.** Neither may contradict the other, and the composition has to be demonstrated rather than asserted.

### The recommendation, and the one thing to decide

**The default stays canon** — the senior staff survive, the chain intact, the player a junior officer. That
protects the retail feel and the canon cast, as the design says, and it keeps the option meaningful rather than
mandatory.

**For this pass, build the mechanism plus two states: the canon default and the captain.** Reason: the captain
state is the one that exercises **the empty chair** and the promotion derivation — the hard path — while the
middle two ranks (department head, executive officer) are the same mechanism with a different casualty count.
**Proving "any number" is affordable is the point, and two states prove it.** The other two then cost data.

### Acceptance — G2

The four items the design names, because a start state is a situation rather than a difficulty slider:

- **The log entry** states the losses, the ship's condition, and that no rescue is coming — and later entries
  are measured against it.
- **The empty chair, or its absence.** Where the captain died, the room and the roster say so **without a
  cutscene**: the acting officer is in the chair, the roster shows the gap, the crew talk about it.
- **Who is doing what now**, as records: the officer at the conn holds a post they did not expect, and their
  old post is held by someone else.
- **The player's own position stated plainly** — what they hold, what they may authorise, who reports to them.

And then:

- **Casualties, promotions and vacancies are visible in the roster and the log before the player does
  anything.**
- **Vacancies are derived, never hand-written** — the same rules as play, demonstrated.
- **Every start state carries the reason the crew accept it, in the log — including the chair.** No post is
  reachable without the fiction supplying the vacancy that created it.
- **Authority is honoured afterwards**: the same decision issued by a junior is refused where the captain's is
  not.
- **The default state is the canon one**, and the ship still starts whole when no state is chosen.

## What is explicitly not in this scope

- **The scenario set** (the walkthrough's G4). A start state is an initial condition; a scenario is a
  situation that arrives. They compose, and this scope lands first so the scenarios have something to start
  from.
- **Track D / multiplayer.** The design says multiplayer *"is simply a start state with more players in it"* —
  which means this mechanism is a prerequisite for it, and building it here is the cheap half.
- **The overlay, the in-game playback and the live model call** (the meeting programme's phase three).
