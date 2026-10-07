# Voyager's systems, and how we implement them

The design that turns canon into a working ship. Inputs: four sourced research briefs in
`docs/research/` (propulsion and power; defence and sensors; interior and life systems; damage and
Borg), the measured inventory of what the game already exposes
(`docs/research/game-systems-inventory.md`), and the ship state model (`docs/ship-model.md`).

Load-bearing canon numbers were re-verified independently against Memory Alpha rather than trusted from
the briefs: warp 9.975 sustained / 6.2 cruise, 141 crew at launch, fifteen decks, 257 rooms, 700,000 t,
fourteen external shield grids, fourteen phaser arrays, four thousand teradynes per second, and the
three independent power sources all check out from source. One number the briefs cited to a page that
does not carry it (teradynes) was confirmed on the correct page instead. Where canon is silent we say so
and decide for ourselves, marked **[our call]**.

## The contract every system must satisfy

A system is not implemented when it has state. It is implemented when all five of these exist:

1. **State** -- named fields in the ship block, with units and a range.
2. **Control surfaces** -- something a person can operate, on a specific deck, that changes that state.
3. **Observable effects** -- visible or audible consequences, *where they belong*: a shield failure must
   be visible on the bridge, not only in a number.
4. **Failure modes** -- what it does when it breaks, and what that takes down with it.
5. **Location** -- the deck and compartment it lives in, so damage to a place is damage to a system.

The ruling constraint, from the owner's own criterion: **a system that cannot be seen failing is not
modelled, it is decoration.** Every tier below is ordered by how many of the five it can satisfy with
work that already exists.

## Canon constants worth honouring

| quantity | canon value | source |
|---|---|---|
| warp core output | four thousand teradynes per second, class 9 warp drive, tricyclic input manifold | Warp core (VOY "Drone") |
| sustainable speed | warp 9.975; ~warp 6.2 normally; over 9.95 only a couple of hours; ~9.97 structural collapse | USS Voyager ("Threshold", "Pathfinder") |
| impulse | fusion-powered, doubles as secondary power; on the aft pylons | Impulse engine |
| crew | 141 at launch, ~150 in service, operable with 100, capacity ~160 | USS Voyager ("Caretaker", "The 37's") |
| hull and decks | 15 decks, 343 m, 700,000 t, 257 rooms | Intrepid class |
| shields | fourteen external grids; six sections (fwd/port/stbd/aft/dorsal/ventral) | Intrepid class ("Equinox II") |
| weapons | fourteen phaser arrays; four torpedo tubes (2 fore, 2 aft); Type 6 photon torpedo | Intrepid class; Type 6 photon torpedo |
| structural integrity | measured in %; 100% = the field carries the whole load | Structural integrity field |
| force fields | rated level 1-10; level 10 can cut a drone from the Collective | Force field |
| transporters | Deck 4, rooms 1 and 2, six pads; 40,000 km standard, 10 km emergency | Intrepid class; Transporter |
| independent power | shuttlecraft, life support and the holodecks are the only three | USS Voyager ("Macrocosm") |
| holodeck reactors | separate from the main grid; **matrix incompatible** -- tapping it blew half the ship's relays | Holodeck reactor ("Parallax") |
| computer | 47 million data channels, 575 trillion calculations per nanosecond; gel packs run half of critical systems; 47 spares | Intrepid class; Bio-neural gel pack |
| replicator ration | a quantity of energy, not a use: a clarinet cost a week, a locket two weeks | Replicator ration |

## Tier 1 -- the spine: power, and the ship you can actually run

**Power (EPS) is the first system, because everything else is downstream of it.** Canon gives the
topology: M/ARA core in main engineering (Deck 11), electro-plasma routed through EPS conduits, taps
converting to electricity at each subsystem, plasma manifolds as the fragile middle layer. **[our call]**
Canon never states a brownout order, so we define one, and the budget squaring sets its arithmetic
(`docs/budget-squaring.md`, Part six): **the cheapest thing is shed first and the warp drive last of
all, after even the comforts**, so the last thing she gives up is the way home. As implemented, the
ladder the crew argue about is:

1. shed first: cargo handling, non-essential lighting, the holodecks, the replicators
2. then: gravity plating, the tractor beam, the turbolifts, sickbay, the science labs
3. then: the transporters, communications, the navigational deflector, astrometrics, the torpedoes
4. then: the phasers, the sensors and the impulse drive
5. kept longest: the warp drive — and, before everything, the spine (life support, structural
   integrity, inertial dampers, the computer core), which is fed first and never shed by choice

The five rungs exist in the code: the ladder could not *look* like a brownout until the lighting, the
cargo handling, the gravity plating and the science were systems of their own.

Model: `warp_core{output, health, coolant, breach_risk, ejected}`; `eps{sector→health, load, coolant}`;
`antimatter{containment%, pods}`; `deuterium{level, integrity}`; `power{budget, ladder_position,
sources[]}`. Control surfaces: main engineering (Deck 11 already carries 28 computer-named interactives
and engine names in the map data), plus the bridge.

Three design consequences we take from canon rather than inventing:

- **The three independent sources are resilience, not redundancy to ignore.** Life support and the
  holodecks keep running when the main grid dies -- so "this deck still has air while that one does not"
  is a state the ship can be in. That is dramatic, and it is free. **Not yet built (the largest
  structural gap):** life support and the holodecks are still ordinary loads on the main grid, so the
  state cannot occur; the cost of building it is sized in `docs/evidence/the-budgets.md`.
- **The holodeck matrix is incompatible.** Jump-starting from a holodeck reactor destroys relays. This is
  a designed failure mode, not a solution: a tempting, canon-backed way to make things worse. **Built:**
  `JumpStartFromHolodeck` ties in a holodeck reactor when the main grid is down, returns a battery
  charge, and wrecks half the ship's systems.
- **Losing the core is not losing the ship.** Impulse fusion and batteries remain; warp is gone. Ejection
  is authorized, unreliable, and the core can be recovered by tractor -- a real decision with a real
  cost. **Built:** with the warp core gone the ship runs only the survival set -- the critical four, the
  impulse drive and the deflector -- and nothing else.

**Doors, force fields, bulkheads.** Level 1-10 force fields, sealable compartments, and the transverse
bulkheads Paris improvised in the Year of Hell (a Titanic reference, and a canon precedent for the
player doing the same). Model: per-compartment `sealed`, `breached`, `force_field_level`. Controls:
existing door and force-field interactives (66 on deck04 alone; force fields on deck05 and deck08).

**Life support.** Environmental control is a Deck 12 node; per-deck atmosphere, gravity and temperature
are ours to track, with gravity failures canon-precedented on Decks 8, 9 and 11. Inertial dampers gate
atmospheric entry and warp entirely -- warp without them kills the crew.

**Movement.** The turbolift is already a level-change graph in the game (139 links across the VV decks
plus the brig). Model it as the deck graph's edges, powered: no power, no turbolift, and the Jefferies
tubes become the only way -- which canon supports and which makes the authored crawlways matter.

## Tier 2 -- the systems that make her a character

**Shields**: six sections, fourteen grids, percentages that buckle, bypass conditions (phased polaron
weapons ignore them entirely). **Weapons**: fourteen arrays with a power setting from stun to vaporize,
four tubes, a finite torpedo inventory that canon refuses to total -- so **[our call]** we set a number
and let it run out. **Sensors and astrometrics**: long and short range, array health, and astrometrics
(Deck 8) as a data-quality modifier with a real cost when it is offline. **Transporters**: rooms 1 and 2
on Deck 4, six pads, 40,000 km range, 10 km emergency, a block list (shields, ion interference, certain
minerals), the biofilter, and the canon rule that a pattern already in the buffer cannot be blocked.
**Medical**: sickbay on Deck 5, three biobeds plus a surgical bed, the EMH and its mobile emitter, and
casualties as crew-record states rather than abstractions. **Replicators and supply**: rations as energy
against a reserve, which is what makes scarcity a mechanic rather than a line of dialogue. **Holodecks**:
separate reactors with the incompatibility hazard above.

## Tier 3 -- attrition: the ship that does not reset

Canon's Year of Hell gives us a dated ledger of what degradation looks like: wounded on day 1, main
power down, environmental control dead on Decks 7-8; a deck section obliterated on day 32 with eleven
torpedoes left; nineteen severed relays killing the turbolifts on day 47; seven decks uninhabitable and
emergency rations by day 65; escape at warp 7 with the outer hull peeling away; abandon ship. **And canon
erases all of it with a timeline reset -- the exact move our design forbids.**

Model: per-deck `hull_integrity`, `pressure`, `eps`, `life_support`, plus ship-level `sif%`,
`torpedoes[]`, `shuttle_roster[]`, `materials`, and the crew roster's `able/wounded/dead`. Damage is
written by events (weapons fire, breaches, overloads) and by passage of time: breaches decompress,
damage spreads along the EPS graph, and uninhabitable decks displace their crew into the survivors.

**Repairs are the counter-play, and they cost.** Materials come from replicator reserves, cargo and
salvage; crew time comes from the same people who hold posts -- assigning a repair empties a station.
That single link is what makes the crew model and the damage model one system instead of two.

## Tier 4 -- the Borg, as a progression with a threshold

The owner's requirement -- board, and if left long enough to organise, take the crew and the ship -- maps
onto canon cleanly, with numbers:

- **Boarding** happens by transporter or cutting beam after shields fail (canon: tractor, shield defeat,
  then boarding). So the Borg are the *consequence* of a defensive failure, not a scripted ambush.
- **Adaptation is a counter per weapon**: canon's phaser adapter lets at most twelve shots penetrate
  before the Borg adapt, and adaptation arrives "within minutes". This is a tension loop the crew can
  play: remodulate, or lose the weapon.
- **Assimilation is a crew-record transition with five stages**, nanoprobes visibly spreading within
  minutes, and a **short reversal window** -- canon makes reversal possible only early, never fully (Seven
  of Nine was never restored). Our window is therefore minutes of ship time, and past it the record
  flips: the post is now held by a hostile.
- **The threshold the owner asked for is a timer**: time-to-organise, running while drones are aboard
  unopposed. **[our call]** start it tunable, around twenty minutes of ship time, and make the
  progression visible -- drone count, systems taken, decks lost -- so the crew can see the clock they are
  racing.
- **Counter-play exists and is canon**: level-10 force fields sever a drone from the Collective,
  explosive decompression works (Cargo Bay 2), remodulation works, and destroying the **vinculum** ends
  local coordination -- but severed drones keep executing secondary objectives, so it is a reprieve, not
  a win. Holograms are *not* canon-supported as a Borg countermeasure; we will not invent it.
- **Ship-level failure** is the Borg reaching the core: capture, or self-destruct. That is the loss
  condition, and it is where the attrition north star ends.

## Where canon is silent, and we decide

| question | canon | our call |
|---|---|---|
| brownout order | silent | the five-step ladder above |
| torpedo complement | totals not stated | 38, depleting; canon's waypoints are **eleven** at Day 32 of Year of Hell and **six** at Day 226 (`docs/budget-squaring.md`, Finding four) |
| sensor ranges | Voyager's not stated | short range by compartment, long range by light-year with a scan time |
| time-to-organise | no such mechanic | ~20 minutes of ship time, tunable |
| point defence | no such system | not modelled; rapid phaser fire is the closest thing |
| Borg versus holograms | unsupported | not used |

## Mapping onto what the game already gives us

Engineering already exists as a room with consoles and engine names (`deck11`: 28 computer-named
objects). Medical exists (`deck05`, `deck08`). Force fields exist (`deck05`, `deck08`). The holodeck and
its objects are on `deck04` -- the deck that turned out to be wrong for crew posts and is right for this.
Doors are everywhere. And the deck graph is already the ship's topology. What none of them have is a
value that matters, because there is no ship state: **our work is wiring existing controls to the model,
not authoring controls from nothing.** That is the single largest risk reduction in this design.

## Sizing and risks

- the model, the blob and the adapter (~85%): as before, shipped mechanisms, no engine change.
- tier 1 on one deck (~60% for a first working loop): power plus doors plus life support, visible.
- tiers 2-3 (~40% per system, and the bulk of the work): each is a state, a control, an effect and a
  failure mode, and there are a lot of them. This is content, and content is where programs of this
  shape die.
- tier 4 (~50%, highest design value): the Borg are the reason the rest of it matters; the progression
  is straightforward once the model exists, but the *pacing* will need playtesting.
- the honest risk (~60%): fidelity versus playability. A faithful Voyager crosses 75,000 light years in
  75 years at warp 6.2. To have a game, time must compress -- **[our call]** the ship clock runs faster
  than the player's, and every repair, assimilation and degradation number is expressed in ship time so
  that pacing is one tunable, not a hundred.
- measuring early: the save block at full fidelity (per-compartment state plus per-crew records) is the
  thing to size before building it wide, exactly as the crew budget was.

## Gates

- **G3** (reactive crew, one deck): unchanged, plus the ship block's shape and the adapter seam.
- **G4** (living ship): tier 1 wired on one deck -- power, doors, life support, and crew records that
  reflect them.
- **G5** (capstone): the loop proven across two decks, with damage persisting between them.
- **G8** (new, proposed): attrition and the Borg -- tiers 3 and 4, gated behind G4 because a Borg
  boarding is only frightening if the ship can be hurt and the crew can be lost.
