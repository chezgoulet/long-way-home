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

> **Resolved 2026-10-08 (`feat/the-way-in`), and it belongs to the launcher's config.** It does not earn a
> default: `0` means "an ordinary level", and a non-zero default would turn the deck-scoped name lookup and
> the crew layer's per-deck arithmetic on for every retail map and every single-deck scenario. It is set once,
> in `configs/lwh-start.cfg`, which both `scripts/run-lwh.sh` and the menu line execute. **The value's source
> is the stitcher itself:** `tools/shipmap/stitch.py` places deck *n* at `(n-1) * pitch`, its `--pitch` default
> is `3072`, and it requires a multiple of 1024 so world-aligned textures land where they were authored —
> `3072 = 3 x 1024`. The shipped map was built with that default. The record is in
> `docs/evidence/the-way-in.md`.

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

---

## Part three — the owner's ruling: it is a configurator, not a preset list (2026-10-08)

Part two asked how many start states to build. **The owner's answer is that the player should not be choosing
from a list of them at all.** Verbatim:

> I think the UI that lets you launch a long way home should give the player full control of the start state. If
> they want to play as any member of the command crew, that means that character died in the beginning and the
> playing character is replacing them. The player should also be able to decide if any of the rest of the command
> crew survived or did not. in this way they can start with a completely fictitious crew and none of the show
> characters or a mix and match. The player can also choose a totally different character path and career path.
> If they want to play lower decks, they can play lower decks. If they want to play be maquis trying to integrate
> with the ways of Starfleet, they can do that. And, of course, if they want to be the captain, they can do that.

**So the entry point is a configurator, and it has four dimensions:**

| dimension | what the player decides |
|---|---|
| **who they are** | the player's record: register, rank, department, and the path that brought them here |
| **who died** | per command-crew member — **survivor or casualty. Any, all, or none.** |
| **who fills the gaps** | the vacancy derivation promotes from the roster — or **the roster is fictitious entirely**, and none of the show characters appear |
| **the career path** | Starfleet junior, lower decks, Maquis integrating into Starfleet, and the chair |

**And the principle it is:** `docs/start-states.md` already says *"there is no post the player is not allowed to
start in."* **The ruling extends it — no crew member is protected from being a casualty, and no career is
closed.** That is the same sentence as *"the menu grants the situation"*, taken all the way out.

## The constraint this creates, and it must be settled before any scenario is authored

**If the player can field an all-fictitious crew, then every authored line, every scenario, every meeting brief
and every log entry must address people by POST and ROLE — never by name.**

The design says *"the scenarios are identical; the position in them is what changes."* **That only holds if the
scenarios do not name anyone.** A line written as *"Tuvok, seal the breach"* is a line that cannot be spoken in
a run where Tuvok died at the Caretaker and a fictitious ensign holds security — and it is not one broken line,
it is the whole slate.

**So: content addresses the post; the simulation resolves the post to a person.** This is a law for the scenario
work (the walkthrough's G4, and the register's O15), recorded here because the scenarios land *after* this scope
and must be authored under it. **It is cheap now and unaffordable later** — every scenario written the other way
has to be rewritten, and the failure would surface as "the scenarios don't work with the configurator" long
after the cause.

## And the voice follows the cast

The voice review settled the split: **the canon few are cloned from the retail assets the player owns, and our
own crew get voices we choose.** The ruling composes with that rather than fighting it:

- **a canon character who survived** — their retail voice, cloned on the player's own machine;
- **a canon character who died** — no voice is needed for them at all, and their lines are not authored;
- **a fictitious crew member** — a voice from our own casting, chosen per character and held for the campaign.

**Which means the casting map is cast-state data rather than a fixed table**: it is derived from the same start
state that decides who is aboard. That belongs to the meeting programme's phase three, and it has a dependency
now that it did not have before.

**And the owner's ruling on the cast, 2026-10-09: use the voices we have.** The pool is **not** pruned and no
audition pass is run for it now — *"I'd rather have a completely working model that needs cosmetic polish than
not get a completely working game"*, which is the same posture as everywhere else in this build. **Polishing
passes are promised** and two facts are recorded for them rather than acted on: some pool voices are thinly
provisioned (one of them has fifty-four seconds of speech in the entire install), and with roughly fifteen
voices across twenty-four people **some crew share a voice**. And the polish need not be ours to record — the
owner's own words: *"some people may even be willing to donate their voice acting to the project."* **So the
cast is good enough to ship as it stands, and better voices are a later pass rather than a blocker.**


## What the first pass builds

- **The configurator**, with those four dimensions, on the menu that G1 puts there.
- **Three proving cases, and they are chosen because each exercises a different path**: the **canon default**
  (nobody dies, the chain is intact), **the captain** (the chair is vacant and the derivation fills it), and
  **an all-fictitious crew** (none of the show characters appear, and nothing in the run depends on their names).
- **The derivation**, unchanged: casualties, then vacancies **by the rules that run in play**.
- **The `log_seed`**, opened on the default and describing whatever the player configured — because that entry is
  how the game *tells* the player what they chose.
- **The POST-not-NAME law** above, written into the scenario authoring rules so the content work inherits it.

**And what is not in this pass:** the **arcs** the career paths imply. A Maquis integrating into Starfleet is a
*played experience* — resentment, affinity, who trusts you and when — and that is the affinities and allegiance
work, not the configurator. This pass lets the player **choose** that path and records it in the record; the arc
that follows is authored later and against the same post-not-name law.

## What this does not change

- **The mechanism is the same one.** The configurator writes the casualties list; the derivation is untouched.
- **The default is still canon**, and the configurator opens on it — so a player who wants the retail feel gets
  it by accepting what is already there.
- **Agents' rule unchanged**: the menu grants the situation, the fiction supplies the reason, the simulation
  holds you to it. It is simply doing more work now.

