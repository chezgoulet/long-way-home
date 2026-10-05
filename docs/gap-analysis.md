# Gap analysis: what the design is missing

A pass over everything designed so far, checked against our own research briefs (which are sourced) and
against what canon actually does. The gaps fall into three kinds, and they need different treatment:

- **Unfunded systems** -- things our own briefs document and our systems table lists, but which have no
  mechanic. These are omissions, not open questions.
- **The social and emotional layer** -- canon's crew drama, which we have referenced constantly and built
  almost none of as state.
- **The player's own body, kit and career** -- the shooter half, which is currently unspecified.

Ordered by what it costs the game to leave undone.

## 1. Systems we documented and never mechanised

| system | in our briefs as | missing mechanic |
|---|---|---|
| **structural integrity field** | measured in %; coils rupture if it collapses; plates tear off at warp | SIF % as a gate on warp speed and hull loss -- currently a footnote in the systems table |
| **navigational deflector** | distinct from shields; micrometeoroid damage when offline; can be burned out | maintenance state, and travel that is unsafe without it |
| **tractor beam** | forward emitter under the deflector, aft on deck 14; sub-warp only | **salvage and towing** -- this is the missing door to the materials economy |
| **emergency power** | impulse fusion plus batteries survive a dead core | battery *endurance*: how long can she last, and what runs down first |
| **inertial dampers** | warp impossible without them; failure kills the crew | the failure state itself, and atmospheric entry as a gated operation |
| **nacelle pylons** | variable geometry; raise above idle warp | pylon/jam damage, and what a stuck pylon costs |
| **sickbay capacity** | three biobeds plus one surgical bed | **triage**: casualties exceeding beds is a canon staple and we have no queue |
| **the EMH's confinement** | cannot leave sickbay or holodecks without the mobile emitter | the emitter as a single irreplaceable artifact, and the Doctor's absence as a real loss |
| **air as a resource** | environmental control per deck; a deck can be sealed | a **clock**: how long a compartment or the ship sustains its crew with life support down |

## 2. The social and emotional layer, which is barely built

The programme's north star is attrition, and attrition means nothing without this layer. All of it is canon,
most of it is cheap, and almost none of it exists as state:

- **Morale and fatigue** -- referenced constantly, never defined. Drivers: sleep, food, losses, hopelessness,
  and warmth. This is the single largest gap in the design.
- **Food, the kitchen and the airponics bay** -- replicator rationing exists; the galley economy does not.
  Canon created the airponics bay specifically to reduce replicator drain, which makes it a system and a job
  site, and Neelix's kitchen is a morale engine.
- **Living conditions** -- quarters, bunking, "five months in a bunk" as a condition crews endure. We have
  "crew life" as a label and nothing underneath it.
- **Relationships** -- the crew model has *memories* but no *bonds*: who is friends with whom, who blames
  whom for what. Every good crew drama runs on this, and it is a small data structure.
- **Loss and grief** -- canon buries its dead. Funerals, casualty notifications, sealed quarters, the wall of
  names. The cheapest possible content for the highest emotional return in an attrition game.
- **Discipline and justice** -- the brig exists as a map; there is no hearing, no confinement, no
  consequence. Canon runs on this (a betrayal aboard, a captain's judgement, a war-crimes reckoning).
- **The Maquis split** -- named in the multiplayer premise as "the players' problem" and never built: no
  factions, no resentment, no reprisals, no integration arc. Canon's richest internal conflict.
- **Training and qualification** -- credentials as a source of access assumes crew *earn* them; the training
  itself (drills, cross-training, teaching) has no mechanic.

## 3. Names without mechanics

Things the design asserts but never defines:

- **The ship's log** -- "the log records it" appears in nearly every document. There is no specced log:
  what it contains, whether the crew can read it, whether the player can search it. It is the natural
  narrative spine and the story-so-far.
- **The holodeck's uses** -- we specced the room and the safety interlock. Canon's actual uses are training
  simulations, recreation, forensic reconstruction, therapy, and the dangerous edges (a programme that
  refuses to end). All of it is free: eight programme maps already ship.
- **Tricorders and away-team kit** -- scanners are canon's exploration verb at human scale, and the game
  ships the Hazard Team's gear. No kit, no loadout, no scanning UI.
- **Resource acquisition** -- mining, salvage, siphoning, EVA work. Dilithium says "mine it"; nothing says
  how, and the tractor beam (above) is the missing half of salvage.
- **Trade and economy** -- referred to for dilithium; no model of what we have, what they want, or what a
  fair trade costs us.
- **Population pressure** -- taking refugees, survivors or prisoners aboard is canon and it spends stores,
  quarters and life support. No game state for how many people the ship can carry and what it costs.

## 4. Canon states we have excluded or not decided

- **The player's own body** -- we specced damage to the ship and injuries to crew records, and nothing about
  the player being wounded, incapacitated, assimilated or killed. In a shooter, that is a hole.
- **Prime Directive consequences** -- first contact has an awareness meter; there is no institutional
  consequence for violating the directive or for the crew's judgement of you.
- **Discipline from home** -- a live link to Starfleet changes what orders you can give. Named, not modelled.
- **Borg strategic awareness** -- we specced a local incursion. There is no state for *the Collective knowing
  where Voyager is*.
- **A persistent pursuer** -- canon's Kazon chase Voyager across a season. We have no mechanic for a faction
  that trails the ship from system to system. (The Kazon are also not in the game's assets -- but any
  faction we do have could play this role.)
- **Deliberately refused:** time travel and timeline resets. Canon uses them constantly; they are the reset
  button this programme exists to reject. Worth stating as a design boundary rather than leaving implicit.
- **Q-class encounters** -- canon-adjacent and nearly free (a voice, an unreasonable demand), but the risk is
  it becomes a licence to break the rules. Low priority, deliberate decision needed.

## 5. The player's career

Start states set the opening; nothing yet describes the player's *progress*: qualifying for credentials,
earning trust, being promoted, being given more access. The access design implies it and the promotion rules
in `docs/start-states.md` gesture at it, but "how a career grows" is unwritten -- and it is the single-player
counterpart to everything the multiplayer premise sets up.

## The five I would close first, and why

1. **Morale and fatigue.** Nothing else in the design pays off without it: damage, losses, hunger and fear
   are only drama if they register somewhere.
2. **Triage and the sickbay queue.** Casualties are the currency of attrition, and right now they have
   nowhere to pile up.
3. **Air and endurance clocks.** Two numbers -- how long the air lasts, how long the batteries last --
   convert every breach and every blackout into a countdown.
4. **The log as a browsable artifact.** It is the story, the memory and the evidence, and every document
   already assumes it exists.
5. **Tricorders and away-team kit.** The cheap half of exploration at human scale, using gear the game
   already ships, and the hook that makes away missions playable rather than narrated.
