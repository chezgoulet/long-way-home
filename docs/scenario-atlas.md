# The scenario atlas — what happens to Voyager

The authoring half of *Long Way Home*: what should happen in and to the ship now that she persists,
remembers, and can be lost. Canon is the quarry; the systems in `docs/ship-systems.md` are the tools.

## What a watch feels like

Deck 11, 0300 ship time. Three crew are on the board and the rest are asleep in quarters you can walk
past and see. Power is at sixty-two percent because engineering locked out the holodeck to keep the
shields fed. A gel pack aft has been running warm for two days. You are nine days out at warp 6.2, and
the only interesting thing on the sensors is nothing. Then the board lights: a contact, a mass reading,
no transponder. And the decision is not *how do I fight this* -- it is **who do I wake up.**

That is the game. Not a shooter with a ship around it. A watch on a ship that is already tired.

## The principle: the ship's state is the story engine

Three rules make authored scenarios feel like a living ship:

1. **The state recruits the scenario.** Scenarios have breeding conditions, not timers. Deuterium under
   thirty percent plus a nebula-class contact gives you the fuel dilemma; a wounded crew list plus a
   battle gives you a sickbay with three biobeds and eleven casualties. The author writes the pieces;
   the ship's condition decides which ones can fire. This is how authored content produces emergent
   narrative without a generative system.
2. **Ignoring something is not failing -- it is a different state.** A fire nobody fights becomes a deck
   nobody can use. A crew member nobody visits remembers it. Nothing resets, which means every scenario
   ends by *writing residue*: damage, a memory, an empty post, a depleted store, a debt.
3. **Warmth funds the loss.** Attrition without joy is a spreadsheet. The mess hall, the kitchen, a
   night on the holodeck, a birthday, a promotion, Seven learning to make small talk -- these are cheap
   (dialogue, place, existing assets) and they are what make the losses land. Battlestar understood
   this: the boxing episode is why the deaths hurt.

## Where the pressure comes from (canon's seven veins)

- **Maintenance and scarcity.** The unglamorous lethal: conduits, gel packs, coolant, deuterium, spare
  parts. Canon runs on this -- deuterium shortages, replicator rationing, a plasma manifold nearly
  destabilising, an infection in the bio-neural grid, forty-seven spare gel packs and no way to make
  more.
- **Walking wounded.** The ship herself degrades: hull, hull plates peeling at high warp with structural
  integrity down, a warp core that can be ejected and recovered, decks made uninhabitable and never
  restored.
- **Encounters that are not fights.** An anomaly that wants studying, a derelict, a species who needs
  something at a cost you can pay only by taking it from somewhere else. The best canon here is a
  dilemma, not a firefight.
- **Opportunity with a price.** A shortcut, a cache, a technology, a deal. Canon's rule: the shortcut
  always costs a principle, a crew member, or a year.
- **The crew as the drama.** Maquis against Starfleet; a crew member who wants to stay behind on a
  planet; someone who disagrees with the captain when the price is other people's lives; a stranger
  aboard who is not crew and must be housed, fed, watched, trusted or not.
- **Home.** A live link for eleven minutes a day changes everything -- morale, and also command, because
  now Starfleet is watching you make the calls you have been making alone.
- **The Borg.** Not an encounter: a clock. Already specified, and the reason the rest of it matters.

## The moral spine: four decisions, endlessly reshuffled

Every scenario in the set asks one of four things, and the whole game is their permutation:

1. **What do we spend?** Power, torpedoes, fuel, parts -- knowing each is finite and nothing resupplies.
2. **Who do we risk?** The crew are records with names, posts and memories. Assigning someone is a real
   trade: their post empties, and the person who was at that post remembers being pulled away.
3. **What do we keep?** Secrets between officers, a promise to a species, the Prime Directive, the
   truth told to a frightened crew.
4. **When do we turn away?** The hardest one, and the one canon returns to constantly: who we decline
   to help because helping them ends us.

## The authoring contract

Each scenario declares, in data:

- **Breeding conditions** -- which ship state makes it eligible.
- **Systems stressed** -- the systems it reads and writes, by our system IDs.
- **Staff needed** -- posts it requires, so the crew model is the cost.
- **Cost to answer**, **cost to ignore**, and **residue** -- what is permanently different afterwards.
- **Visibility** -- what the player sees *where they are*, since a consequence they cannot perceive did
  not happen.
- **Warmth** -- the human beat it leaves behind. A scenario with no warmth is a hazard, not a story.

**The post-not-name law, and it binds every line.** `docs/the-entry-point.md`, part three: because the
player may field an **all-fictitious crew** in which none of the show characters appear, **content
addresses the post, and the simulation resolves the post to a person.** A line written as *"Tuvok, seal
the breach"* is a line that cannot be spoken in a run where Tuvok died at the Caretaker and a fictitious
ensign holds security -- and it is not one broken line, it is the whole slate. **No scenario, meeting
brief, log entry or voice line may name a crew member; every one addresses a post, a system id or a
role, and the simulation substitutes whoever holds it.** The register's `law` column
(`docs/hook-register.md`) marks which hooks take a resolved person and therefore must be fed by a post.
This is cheap now and unaffordable later: every scenario written the other way has to be rewritten, and
the failure would surface as *"the scenarios do not work with the configurator"* long after the cause.

## Rank and role: one event, many positions

The owner's addition, and it is the thing that turns a scenario set into a ship. But the naive version --
*unlock different content for different classes* -- wastes it. The strong version is: **author every
scenario once, and instantiate it through the post you hold.** The same event arrives as a different
problem depending on where you are standing in it.

Rank and role change four separate things, and conflating them is the failure mode:

1. **What you can see.** Information is the first expression of rank. The captain's board shows the whole
   ship's condition; a console on deck 12 shows that deck and its own systems. Filtered status is the
   cheapest and strongest signal that you hold a job rather than a class.
2. **What you are allowed to do.** Authority is canon and specific: ejecting the warp core needs the
   chief engineer's or senior staff's authorization code; red alert is the highest status and can be
   raised by more than the captain. Gating controls by authority means some things need **two people** --
   which is the cooperative ship the owner asked for, expressed as a rule rather than a wish.
3. **Who you are responsible for.** A department head owns people, not systems: their roster, their
   posts, their casualties. The security chief's version of an intruder is *deck seven and three people
   with no relief*; the engineer's is *the manifold is screaming and there are two hours of coolant*.
4. **What you are told, and by whom.** Rank decides who reports to you and whom you report to. The same
   emergency reaches a junior as a status update, a department head as a decision request, and the
   captain as a choice between two bad outcomes -- or as an order you disagree with and must carry out
   anyway. That last position is the most interesting one in the whole game, and canon lives in it.

**A worked example.** Scenario 2, the severed relays: as captain you decide whether to take power from
the shields mid-engagement; as the engineer you are the one whose console is on fire and who must tell
the captain the truth about the time; as security you are holding a corridor with no relief and a
wounded crewman behind you; as the young officer on the deck where the lights went out, you are the one
sent into the junction. One authored event. Four experiences. Four decisions, each with its own stakes.

**And rank must move.** The ship's roster promotes to fill its own gaps: canon's crossing cost the first
officer, the chief engineer, the entire medical staff and the transporter chief, and the ship sailed on
with people in jobs they were not trained for -- a Maquis made chief engineer, a young ensign left on the
bridge, a civilian given a department. In a run with no reset, **your rank rises because people died.**
That is the attrition north star reaching the player's own shoulders, and it is also the scenario set's
answer to how a junior post stays interesting: you inherit the job when it empties.

**Where this pays off most is multiplayer.** In single player the player holds one post and the rest are
NPCs; the lens is theirs alone. Under one-ship-one-server it becomes the whole point: the captain, the
engineer and the security chief are three people with partial information, gated authority, and no
single view of the truth. Asymmetric posts with real decisions is what makes a ship server more than a
lobby. Keep the role model serialisable and the authority table in data, and this costs nothing later.

**One honest friction.** A post with nothing to decide is a spectator, and "nobody will listen to me" is
only fun if it can eventually matter. So every post needs a decision at its own level, and every junior
post needs a channel that escalates -- the report that nobody acted on, which the player can later point
to. That is not a nicety; it is the difference between an organisation and a menu.

### What a scenario must therefore declare

In addition to the contract above: **the positions it can be experienced from**, and for each one, the
information they receive, the authority they hold, the people they are responsible for, and the decision
they own. Author the event once; author the positions as data.

## What each faction can threaten

The owner's conclusion: with no new species assets we still have plenty to work with. The reason that
is true is not the *number* of factions -- it is that **each one writes a different part of the model**,
so the kinds of trouble differ and not merely the strength of the enemy. Enemies are drawn from
`docs/research/game-alien-roster.md`.

- **The Borg** want the ship and the crew. They write `controller`, crew records and `compromise`, and
  they are **the only faction that can take the ship outright**. Their scenarios are boarding, dwell
  thresholds, reclamation work and the assimilation window.
- **Species 8472** want nothing -- they are pure lethality, with no capture and no compromise. Canon puts
  them beyond most of what the crew has, which makes their scenarios about *not fighting*: sealing,
  escaping, and deciding what a fight is worth. Their real function is to make the weapons unattractive.
- **The Hirogen** want the hunt, not the ship. They touch crew records as *individuals* and deliberately
  leave systems working, so the engineering problem becomes a distraction from the danger. A Hirogen
  scenario needs no damage model at all -- which makes it cheap and terrifying.
- **The Scavengers** want parts. They write `compromise` and the stores, and they *remove capability*
  without taking ground: a door, a system, plating, stores, gone -- sometimes noticed only later. That is
  a scenario shape nothing else in the roster produces.
- **The Reavers** are the same appetite at scale -- 173 placements, the heaviest enemy presence after the
  Borg. Raiding, abducting, wrecking.
- **The Harvesters and the machine family** want access and throughput. They take junctions and corridors,
  deny movement, and make a compartment serve their own function. The counter-play is about *routes* --
  turbolifts, Jefferies tubes, which way round -- rather than territory.
- **The Malon** want somewhere to put the waste. That gives a scenario where the threat is a *substance*:
  contamination spreading deck by deck, containment, and a long cleanup in the job queue. Delightfully
  mundane, and canon.
- **The Klingons** come through the holodeck, which makes them the *warmth* faction: a programme, a
  pastime, a training run -- and, on a broken ship with damaged safety systems, a genuinely dangerous one.
- **The three unattributed factions** (`imperial`, `stasis`, `avatar` in the game's own files) are not a
  gap. They are **unknown species**, which is what the Delta Quadrant runs on: the crew do not know what
  they are dealing with and neither does the player. Their scenario shape is investigation -- a first
  contact that can go either way.

The practical consequence: a Borg incursion, a Hirogen hunt, a Malon contamination and a Scavenger raid
are four *different games* built on one model, with one asset set and no new art. That is the variety
budget, and it is already paid for.

## A first slate

Nine, ordered by how soon they can be built against the systems that already exist. Each names its canon
root, its decision, and its residue.

1. **The gel pack fever.** An infection spreads through the forward bio-neural grid. Replicators and
   half of critical systems degrade deck by deck; the fix is medical *and* structural -- isolate the
   grid and treat the carriers. *Decision:* what do you turn off while the grid is down. *Residue:*
   rationing the crew will remember, and one deck's upkeep gone quiet.
2. **Nineteen relays.** Battle severs the EPS runs feeding the turbolift network. Crossing the ship now
   means Jefferies tubes, so travel takes real time and a wounded crew member on one deck cannot reach
   sickbay on another -- unless you route power to the lifts and take it from the shields while the
   shooting continues. *This is the whole game in one decision.* *Residue:* a limp, a grudge, a
   rerouted grid.
3. **The price of fuel.** Deuterium low, one source available, and the species who owns it wants
   something you should not give -- a technology, a favour, a person's expertise for a year. *Canon's
   oldest move.* *Residue:* a debt, a resentment, or a shorter journey and a compromised principle.
4. **The ones who stay.** A world offers a life. One or more crew records transition out of the crew:
   the post is empty permanently, their quarters get reassigned, and the people who loved them keep the
   memory. *Decision:* whether to argue, and whether to let them go well. *Residue:* grief that is
   actually modelled.
5. **The shortcut.** A route that cuts years off the crossing, at a structural cost: the ship arrives
   different, or the crew does. *Decision:* the trade, and who gets told the truth about the odds.
6. **Eleven minutes.** A live link home. Morale surges, and command changes: decisions you made alone
   can now be questioned, and what you tell them becomes a choice with consequences. *Residue:* a crew
   who now have someone else to be accountable to.
7. **The other captain.** A Starfleet crew who survived by doing monstrous things. The mirror is not
   *what would we do* but *what have we already done*, and the crew's records carry the answer.
   *Decision:* judge, help, or take what they have. *Residue:* the ship's own moral ledger, read out
   loud.
8. **A fire, a breach, a deck.** The physical emergency -- decompression, a hull breach, a bulkhead
   sealed with crew on the wrong side, a deck evacuated and then written off. *Decision:* who goes
   through the hatch, and whether the deck is ever recovered. *Residue:* a sealed deck you can visit
   later and see what was left behind.
9. **Organising.** The Borg aboard, unopposed, the clock running. *Decision:* the whole defensive kit --
   decompression, level-10 fields, remodulation, the vinculum -- played against a visible countdown.

## What this needs from the systems

Scenarios 1, 2, 6 and 8 are almost entirely **tier 1** -- power, doors, fields, life support, crew
records, the clock -- which means they can be authored as soon as S2 lands, *before* weapons or shields
are deep. Scenarios 3, 5 and 7 need stores and reputation; 4 needs crew-record transitions; 9 needs tier
4. That ordering is the content roadmap, and it means the fun does not have to wait for the plumbing.
