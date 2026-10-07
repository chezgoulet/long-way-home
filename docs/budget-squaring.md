# The ship's capability against her budgets

**Status: implemented 2026-10-07, on `feat/the-budgets`. The number set below is the one in the code.**

The **demand** table in Part four stands. The owner's headroom ruling of 2026-10-07 (**Part five**)
**supersedes Part four's constant 230-EPS shortfall** and replaces it with a supply curve that starts *above*
demand and tightens as the dilithium crystal ages. Part four's *constant* does not survive; its *demand* does.

Implemented from this document: the five added systems and a demand of **1,730**; the sources at warp core
**1,400** (scaled by the crystal's ceiling), impulse reactors 250, auxiliary fusion 90, batteries 60, for a
fresh **1,800**; the shed order with the **warp drive last**; the ceiling-scaled warp core; the engineering
console's two power figures (fresh and now); and the torpedo complement at 38. `scripts/test.sh`,
`scripts/check.sh` and the engine checks are green, and the evidence is
`docs/evidence/the-budgets.md`. Save format 50 (the five systems raise `SYS_COUNT`).

Read this with `docs/damage-and-budgets.md`, which owns the two budgets, and `docs/ship-systems.md`, which
owns the systems. Every figure below was computed rather than recalled, and the canon it rests on is in
`docs/lore-ledger.md`.

---


**A squaring, for the owner's ruling.** 2026-10-07. Torc.

Sources: `docs/ship-systems.md` (canon constants, the brownout ladder), `module/ship/ship_core.cpp`
(`SPECS[]`, `SOURCES[]`, the allocation tick), `docs/damage-and-budgets.md` (the two budgets),
`docs/research/voyager-damage-borg.md` (the Year of Hell cascade), `docs/lore-ledger.md`.

Nothing here is implemented. This is the number set, with its reasoning, so it can be ruled on before a
lane touches it.

---

## Part one — what canon actually gives us

**The hull and the people.** Intrepid class, 15 decks, 343 m, 700,000 t, 257 rooms. Crew 141 at launch,
~150 in service, **operable with 100**, capacity ~160. *("Caretaker", "The 37's")*

**The plant.** A class 9 M/ARA core rated at **four thousand teradynes per second**, deuterium and
antimatter, plasma routed through EPS conduits with manifolds as the fragile middle layer. Impulse is
**fusion-powered and doubles as secondary power**, mounted on the aft pylons. *("Drone")* Canon states
**no numbers** for consumption, allocation or a brownout order — so every figure in our model is ours,
and the model says so ("EPS units, an invented scale: canon gives no figures").

**The drive.** Sustainable warp 9.975 at the absolute limit, ~6.2 normal, over 9.95 for only a couple of
hours, structural collapse around 9.97. *("Threshold", "Pathfinder")*

**The fit.** Fourteen phaser arrays. Four torpedo tubes — two fore, two aft — Type 6 photons. Fourteen
external shield grids in six sections. Structural integrity measured in per cent. Force fields rated 1–10.
Transporters on deck 4, rooms 1 and 2, six pads, 40,000 km / 10 km emergency. Computer: 47 million data
channels, 575 trillion calculations per nanosecond, gel packs running half of critical systems, 47 spares.

**The three independent sources, and this is the important one.** Canon names exactly three things that
keep running when the main grid does not: **shuttlecraft, life support, and the holodecks.** *("Macrocosm")*
And the holodeck reactors are not merely separate — their **matrix is incompatible**, and jumping from one
blew half the ship's relays. *("Parallax")*

**The ration.** A replicator ration is a **quantity of energy, not a count of uses**: a clarinet cost a
week, a locket two weeks.

**The attrition, as a timeline.** The Year of Hell is our only canon cascade with dates, and it is the
budget target:

- **Day 1** — fifteen wounded, main power down, environmental control dead on Decks 7–8, main computer
  core offline.
- **Day 32** — EPS conduits across Deck 5, sections 10–53 "obliterated"; two crew dead; **eleven photon
  torpedoes remaining.**
- **Day 47** — nineteen main power relays severed; turbolift network disabled.
- **Day 65** — Deck 11 power grid destroyed; replicators badly damaged → emergency rations; environmental
  control failing, **seven decks uninhabitable**; crew in bunks; bridge "totally wrecked"; astrometrics
  offline.
- **Day 70** — escape at warp 7 with structural integrity failed, hull peeling away; transverse bulkheads
  seal sections; abandon ship.
- **Day 226–257** — port saucer largely gone; **six photon torpedoes left**; she is destroyed.

---

## Part two — what our model says today

**Supply — four sources, 1,500 EPS nameplate.**

| source | capacity | deuterium/day at full | antimatter/day at full |
|---|---|---|---|
| warp core | 1,000 | 0.004 (250 days) | 0.003 (333 days) |
| impulse reactors | 300 | 0.003 (333 days) | — |
| auxiliary fusion | 120 | 0.001 (1,000 days) | — |
| emergency batteries | 80 | — (3 hours at full draw) | — |
| **total** | **1,500** | | |

**Demand — eighteen systems, 1,540 EPS at full.**

| # | system | deck | station | demand |
|---|---|---|---|---|
| 0 | life support | 12 | Environmental Control | 60 |
| 1 | structural integrity | 11 | Main Engineering | 80 |
| 2 | inertial dampers | 11 | Main Engineering | 40 |
| 3 | computer core | 10 | Computer Core | 60 |
| 4 | shields | 1 | Bridge, Tactical | 200 |
| 5 | sensors | 8 | Astrometrics | 60 |
| 6 | impulse drive | 10 | Impulse Engineering | 100 |
| 7 | phasers | 1 | Bridge, Tactical | 150 |
| 8 | torpedo launchers | 10 | Torpedo Bay | 30 |
| 9 | warp drive | 11 | Main Engineering | 400 |
| 10 | navigational deflector | 11 | Deflector Control | 50 |
| 11 | communications | 1 | Bridge, Operations | 20 |
| 12 | transporters | 4 | Transporter Room 1 | 60 |
| 13 | sickbay | 5 | Sickbay | 30 |
| 14 | turbolifts | 1 | Bridge, Operations | 20 |
| 15 | tractor beam | 10 | Shuttlebay Control | 60 |
| 16 | replicators | 2 | Mess Hall | 60 |
| 17 | holodecks | 6 | Holodeck 2 | 60 |
| | **total** | | | **1,540** |

**The knife-edge, and it is a good accident.** 1,500 supplied against 1,540 demanded: **the ship is forty
EPS short of being able to run herself completely.** There is no configuration in which nothing is given
up. And the cheapest single cut that closes the gap is the **inertial dampers**, exactly forty — or the
**intercom and the lifts**, twenty each, which also comes to exactly forty.

That is the right shape and **nobody chose it.** It should be made deliberate rather than left as an
artefact of two independently invented number sets.

**The critical four.** Priority ≤ 3 is life support, structural integrity, inertial dampers, computer core —
**240 EPS**, fed before anything else. Against the batteries alone (80) they cannot be held; against
auxiliary fusion (120) they cannot; against impulse reactors alone (300) they are covered with sixty to
spare. That ladder is correct and reads exactly as it should.

**The core-less ship is too capable.** Lose the warp core and 500 EPS remain against a critical 240 — so
the ship still runs her weapons, her shields, her sensors and her comms. In canon, losing the core costs
you *the ship*: she lives, she limps, and she does nothing else.

**The drive is shed as a cliff, and the consequence is unstated.** At priority 9 and 400 EPS, the warp
drive is the largest single load by half again, so dropping it funds the entire rest of the ship. That is
almost certainly the right mechanic and it means **giving up the drive is the decision that keeps the ship
alive — and it is the decision that means never getting home.** The document's brownout ladder does not
place the drive at all; the code has placed it, and nothing says so out loud.

---

## Part three — where our model and canon disagree

**Finding one: the model is a warship's budget on a science vessel's hull.** The tactical fit — shields,
phasers, torpedoes, tractor — is **440 EPS, 28.6 per cent of the grid.** The science fit, as modelled, is
**one sensors row at 60 EPS: 3.9 per cent.** There are **no labs, no astrometrics, and no long-range
sensor** anywhere in the model.

So the ship whose *model* is a warship is not the ship canon describes — and the owner's instinct was
half right in a way worth naming: **the model is the warship; the ship is the science vessel.** A science
vessel whose power model spends twenty-nine per cent on weapons and four per cent on the mission she was
built for. This is the finding that matters most, and it reframes the tweak the owner suspected: the
direction is to raise the science load until it can compete with the tactical one.

**Finding two: canon's best gift is not implemented.** Life support and the holodecks are modelled as
**ordinary loads on the main grid**, not as independent sources. So the state canon hands us free —
*this deck still has air while that one does not* — **cannot occur in our model.** The systems document
describes the three independent sources as the thing that makes deck-by-deck survival dramatic; the code
has no mechanism for it. This is the largest structural gap in the ship's model.

**Finding three: the brownout ladder has five rungs in the document and about three in the code.**
The document sheds holodecks, replicators and non-critical labs first; then non-essential lighting, cargo
handling and astrometrics; then sensors, comms and transporters. In the code, only the holodecks and the
replicators exist to shed first — **there is no lighting, no cargo handling, and no labs or astrometrics
to lose.** A brownout therefore cannot *look* like a brownout. It goes from whole to serious with nothing
in between.

**Finding four: canon answers the question the document left open.** `docs/ship-systems.md` says of the
torpedo complement, *"totals not stated — set a number, make it deplete."* Canon supplies the waypoints:
**38** as the ship's own stated load, **11 at Day 32** of Year of Hell, **6 at Day 226.** So the number to
set is one the player can be *down to eleven at* and *down to six at*, and those two moments should be
built as moments.

---

## Part four — the proposal, for ruling

### 4a. Move the sources so the core is the plant, and the coreless ship is nearly dead

| source | was | **propose** | why |
|---|---|---|---|
| warp core | 1,000 | **1,100** | canon's plant; losing it should cost most of the ship |
| impulse reactors | 300 | **250** | still the secondary that flies her home |
| auxiliary fusion | 120 | **90** | keeps the spine alive after the core |
| emergency batteries | 80 | **60** | exactly life support, at full, for three hours |
| **total** | 1,500 | **1,500** | unchanged — the grid is right |

**The test it passes.** With the core gone, 400 EPS remain; the critical four take 240, the impulse drive
100, the navigational deflector 50 — **390 of 400, ten to spare, and nothing else on the ship runs at all.**
She lives, she moves at sublight, she has her deflector. No weapons, no sensors, no comms, no transporters,
no labs. That is the Year of Hell, and it is what canon implies.

**And the batteries alone hold life support at exactly full for three hours.** Not a system short of it.

### 4b. Add the science and the comfort, so the ship is short of a *subsystem* rather than of a trim

| add | demand | why |
|---|---|---|
| **astrometrics** | 60 | canon's dedicated facility with its own arrays; currently hidden inside sensors |
| **science labs** | 60 | she has labs; the model has none, and the mission is the ship's identity |
| **gravity plating** | 30 | canon's plating draws power; currently implicit in life support |
| **non-essential lighting** | 20 | the first thing that goes in the show, and the rung the ladder is missing |
| **cargo handling** | 20 | the ladder's other missing rung |
| **total added** | **+190** | |

**New demand: 1,730 against a 1,500 grid — a shortfall of 230 EPS.**

That gap is deliberately larger than the current forty, and the reason is the game. A forty-EPS gap
resolves with one cut: shed the dampers and you are whole. **A 230-EPS gap means the ship is permanently
short of a whole subsystem, and which one is the player's strategy.** That is the north star with a number
on it.

### 4c. What it costs to run everything else

Cut the comforts and the science and the whole rest of the ship runs:

| cut | saved |
|---|---|
| replicators | 60 |
| holodecks | 60 |
| non-essential lighting | 20 |
| cargo handling | 20 |
| **science labs** | 60 |
| **turbolifts** | 20 |
| **total cut** | **240** |

Demand falls to 1,490 of 1,500. **So the price of running the entire ship — drive, shields, weapons,
sensors, astrometrics, transporters, sickbay, tractor — is the labs, the replicators, the holodecks, the
lights, the cargo handling and the lifts.**

Dark corridors. Emergency rations. No holodecks. No science. And she can fight and she can fly. That is one
sentence, it is checkable, and it is precisely what Year of Hell looks like on screen.

### 4d. Give the drive a rung, and say what it costs

Place the warp drive explicitly, at the top of the shedtable order, with its consequence written down:
**shedding the drive funds the rest of the ship, and means the ship has stopped going anywhere.** It is the
one brownout decision that is not a brownout.

### 4e. The torpedoes

Start at **38**. Let the count come down. Build the two canon moments: **eleven**, and **six**.

### 4f. Three checks the model should have, from canon

1. **Operable at 100 crew** ("The 37's"). The ship's 22 posts must be coverable by a hundred people, and
   the state below that should be flyable but degraded.
2. **At least one deck holding air independently of the main grid** ("Macrocosm"). The three independent
   sources are the mechanism canon gives us for deck-by-deck survival, and we do not have it.
3. **The holodeck matrix is a trap, not a solution** ("Parallax"). Jump-starting from a holodeck reactor
   destroys relays. A designed failure mode — a tempting, canon-backed way to make things worse.

---

## Part five — the headroom correction (owner, 2026-10-07), and it is better than Part four

**The owner's ruling:** the ship should **leave port with headroom on the power budget**, and the conditions
that tax it should be *created* — not assumed. His mechanism: the **dilithium crystal**. Re-crystallising
extends its life but **diminishes total output**, so a fresh crystal gives headroom and the budget tightens
over time.

**This supersedes Part four's 230-EPS shortfall, and it should.** A fixed shortfall taxes the ship from
minute one, and **a budget that starts tight can never be felt to tighten.** The owner's version gives her a
beginning with margin, which is what makes the loss of margin legible at all. Part four's *demand* stands;
Part four's *constant* does not.

**And the mechanic is already built — nearly to the letter.**

| what the owner proposed | what ships today |
|---|---|
| fresh crystals give headroom | `dilithium` (0..1) is the crystal's remaining life; `crystalQuality` (≥1) is efficiency |
| re-crystallising extends life | `Recomposite()` — "Engineering buys back life in the crystal's frame" |
| ...but diminishes output | `crystalCeiling` ages: **each recomposition permanently lowers what can be restored** (`RECOMPOSITE_CEILING_DROP = 0.15`) |
| the budget tightens over time | `JUMP_DILITHIUM = 1/40` — a fresh crystal lasts forty cruising jumps; `MIN_WARP_DILITHIUM = 0.02` and below it there is no warp at all |

Implemented 2026-10-07, save format 30, with `TestDilithium` and `scripts/dilithium-check.sh`. And the
fiction is canon-sourced in `docs/exploration-and-science.md`: dilithium **fractures** under use;
recrystallisation "significantly increased the lifespan of a dilithium crystal, but occasional replacement
was still necessary" (Memory Alpha); **recrystallisation is not universally available and access to it has
been restricted**, so it is a capability the ship must hold onto; and **Voyager found a new form of
dilithium in the Delta Quadrant in 2372** that stayed stable in conditions the old kind could not — so
*finding better crystal through exploration is canon, not our invention.*

**The one difference, and it is the piece the owner adds.** Today `crystalCeiling` limits **how much *life*
can be restored** — an endurance ratchet. The owner's mechanic has it bear on **output** — a power ratchet.
Those are different things, and the second is the one that connects the crystal to this budget:
**`crystalCeiling` should also scale the warp core's output.** Then recomposition is a trade of *capability*
for *time*, and the ship's power budget is what pays for it.

### The corrected supply model

Demand stands at **1,730** (Part 4b). So the fresh grid must exceed it, or there is no headroom to lose:

| source | fresh crystal | at ceiling 0.85 | at ceiling 0.70 | at ceiling 0.55 |
|---|---|---|---|---|
| warp core (1,400 × ceiling) | 1,400 | 1,190 | 980 | 770 |
| impulse reactors | 250 | 250 | 250 | 250 |
| auxiliary fusion | 90 | 90 | 90 | 90 |
| emergency batteries | 60 | 60 | 60 | 60 |
| **supply** | **1,800** | **1,590** | **1,380** | **1,170** |
| **against 1,730** | **+70 spare** | **−140** | **−350** | **−560** |

**She leaves port with seventy EPS in hand.** Then each recomposition costs her a fifteenth of her plant
forever, and the squeeze arrives in sequence: the comforts and the science first, then the operations, then
the drive itself. At a ceiling of 0.55 she is running on two-thirds of the ship that left port, and the only
way back is **a new crystal — which costs a detour, a trade, or a research programme.** That is the loop
`docs/exploration-and-science.md` already describes: decline, recomposite, replace, improve.

**And it makes the coreless state a slope rather than a cliff.** At a spent crystal the ship converges on the
Part 4a coreless condition continuously: she lives, she limps at sublight, she keeps her deflector, and
nothing else runs. Smooth is better — the player *watches* the ship become the ship that came home in Year
of Hell.

**One display consequence, and it is cheap.** The navigation counter already shows **nominal against
current capability** for *time* — "71 years nominal, 76 at current capability." The power budget needs the
same two numbers, and the same gap, on the engineering console: **what the plant can deliver fresh, against
what it can deliver now.** It costs a string, and it is where the player learns the budget is shrinking.

**Marked invented:** that the ceiling bears on *output*. The fracture, the limited availability of
recrystallisation, and the better-crystal discovery are canon; this consequence is ours, and it goes in
`docs/lore-ledger.md` as `[inv]` like the fraction rates beside it.

---

## Part six — what this costs to build, and what I could not settle

**It is not free.** Adding five systems changes `SYS_COUNT`, which touches `SPECS[]`, the allocation order,
the console system rows, the check scripts and `SAVE_VERSION`. Older saves are invalidated, as every bump
has invalidated them. The five additions are the whole cost; the source rebalance and the priority
placement are number changes.

**What I could not settle, and it is the owner's.** Whether 230 is the right shortfall — it is a difficulty
number, and difficulty is his, and **his headroom ruling has superseded it anyway** (Part five). Whether
shields stay at 200 while the tactical fit is already twenty-nine per cent of the ship; my recommendation is
to leave them and let the science compete rather than to cut the weapons, because *labs or phasers* is a
better dilemma than *a smaller phaser*.

**And one question that was open and is now settled by the arithmetic.** I asked whether the drive belongs
above or below the comfort systems in the shed order. Under the headroom model the answer is not a
preference — it is forced: **the drive must be shed last of all, after even the comforts, or the ship's
priorities invert.** If the drive is shed at its current priority the crystal has only to age a little for
the ship to give up going home in order to keep the holodecks lit.

**So shed from the cheapest thing upward, and this is what actually happens** (verified arithmetic, demand
1,730 against the Part five supply curve):

| crystal ceiling | supply | short by | what the ship gives up |
|---|---|---|---|
| **1.00** | 1,800 | — | **nothing.** She leaves port with seventy to spare |
| **0.85** | 1,590 | 140 | cargo handling, non-essential lighting, the holodecks, the replicators |
| **0.70** | 1,380 | 350 | …plus gravity plating, the tractor beam, the turbolifts, **sickbay**, and the science labs |
| **0.55** | 1,170 | 560 | …plus the transporters, communications, the navigational deflector, astrometrics, and the torpedoes |
| **0.50** | 1,100 | 630 | …plus the phasers. She flies, and she holds her shields, and that is all |

**Read down that column and it is the show.** At 0.85 the corridors are dark, the holodecks are off and the
crew are on rations — and everything that matters still runs. At 0.70 there are no lifts, so the crew walk
the Jefferies tubes; there is no sickbay; there is no science. At 0.55 she cannot talk, cannot beam, cannot
see far and cannot fire a torpedo — **and she still has warp, phasers and shields.** She keeps her ability
to go home until the crystal is very nearly dead, because going home is shed *last*.

That is the whole design of the ship in one table: **every step down the crystal curve takes a capability
away, and the last thing she gives up is the way home.**

**What I did not do.** I changed no code, and I ran no build. This document is the number set.
