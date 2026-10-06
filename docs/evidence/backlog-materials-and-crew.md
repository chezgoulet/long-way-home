# Evidence: the backlog — materials, travel systems, and the crew tied to the ship

Date: 2026-10-07. The approved gap backlog in `docs/gates.md` (*Then, in rough order*). This is the
first batch, in the ledger's order: the tractor beam and salvage as the door to a materials economy,
then structural integrity, the navigational deflector, the inertial dampers and the EMH; and the crew
systems the later items need (training and credentials, the brig, grief, the career, the player's
body, the Maquis split, Borg adaptation, a persistent pursuer). The rules are in the ship core and
tested by `tests/ship` (`TestMaterialsAndTravel`, `TestCrewJusticeAndBorg`); the controls are driven
through the console by `scripts/backlog-check.sh`.

## The materials economy (the backlog's first item)

- **The tractor beam.** `SYS_TRACTOR_BEAM` now does its two canonical jobs. It holds a derelict to
  strip it fully (`TractorWreck`: `SALVAGE_PARTS` and `SALVAGE_MATERIALS`, once per wreck, tracked by
  `Beacon.looted`), and it locks a contact (`TractorHold`) so an enemy with its hull going and its
  engines intact cannot break off and run — the pursuit below.
- **Salvage.** Arriving at a derelict still yields the quick look's parts (S9); the tractor strips it
  for the raw material.
- **Fabrication and the galley.** `FabricateParts` turns raw material into spare parts at the
  replicators, and `FabricateRations` turns it into food (the galley), so repair, build and eating all
  draw on the same store. `Stores.materials` is the new number. Console: `ship tractor`,
  `ship fabricate <n>`, `ship rations <days>`.

## The travel systems

- **Structural integrity** is what holds the hull together: a hit's damage scales with `2 - output`,
  so a ship with failing integrity loses hull faster (`UpdateOutside`).
- **The navigational deflector**: a jump with it below `TRACTOR_MIN` lets dust through — a deck is
  breached on arrival.
- **The inertial dampers**: an undamped jump shakes the crew — one or two are injured.
- `TravelSafe` reports all three; the three each matter to a jump's cost, not its possibility. The
  same three are what `docs/ship-systems.md` names.

## The EMH

`ActivateEMH`/`EMHActive` (console `ship emh on|off`, Sickbay's `B` companion): the Doctor is a
program, so it needs the computer core, and with the medical staff down it keeps the ward at half
output. It closes the triage gap's last named item.

## The crew, tied to the ship

- **Training and credentials.** `Train` grants a cross-qualification (a `credentials` bitmask); a
  credential lets a crew member operate a station their department does not own, so `MayOperate`
  consults it. `Qualified` reads it.
- **Discipline and justice.** `Brig`/`Brigged`: a confined crew member stands no watch, mans nothing
  and operates nothing, until released (`ship brig <crew> on|off`).
- **Grief.** A death is **notified to command and the casualty's quarters sealed** (a log entry,
  `CrewMember.quartersSealed`), and `HoldFuneral` (`ship funeral`, given by whoever commands) opens
  the sealed quarters and lifts the crew who have been lost and are still missed, the drag the morale
  gap models. Tested in `TestCrewJusticeAndBorg`.
- **The player's career.** `Promote` (`ship promote <crew>`) raises a rank within the complement, by
  whoever commands, up to captain.
- **The player's body.** `PlayerIncapacitated` reports the player's crew record's state; a player who
  is injured, dead or assimilated is a state the game can act on.
- **The Maquis split.** `CrewMember.faction` distinguishes Starfleet and Maquis; a crew divided
  against itself is a measurable drag on morale (worst when the split is even), and the field is in
  the save and the roster, ready for the integration arc.
- **Borg adaptation.** A Borg vessel adapts: each phaser and torpedo hit adds to its `adaptation`, and
  the next does less — the same fire that destroys a raider is survived by a cube.
- **A persistent pursuer** (already from S9): a raider whose engines survived follows between beacons.

## Memory and consequence (the relationships the later items rest on)

`docs/memory-and-consequence.md`'s model is built (`TestMemoryAndConsequence`): every crew member
keeps a **bounded** set of marks — event, the person involved, **provenance** (saw / told / rumour /
log), time, valence and salience — with a hard cap of `MEMORY_MAX` and eviction of the least salient
(oldest on a tie). Salience decays unless the mark is reinforced, so a memory fades if nothing retells
or re-experiences it. A mark is written where it happens (`Remember`) or by command telling the crew
(`Brief`); **a character who was not present and was not told cannot recall it**. Valence toward a
person, summed, is a **bond** (`Bond`): repeated good is friendship, repeated bad a grudge, one
extreme event either in a stroke. The three acceptance questions are answered by `Recall`,
`RecallSource` and `MemoryCount`; console `ship remember|brief|recall|bond`.

## Resource acquisition, population pressure and justice (2026-10-07)

`TestResourcesAndPressure`:

- **Resource acquisition.** A new beacon kind, a **resource belt**: `MineBelt` (console `ship mine`)
  yields `BELT_MATERIALS` and siphons deuterium, once per belt, and needs a working tractor or the
  sensors to reach it. Mining is the material source the economy's build side named.
- **Population pressure.** `TakeSurvivors` (console `ship survivors <n>`) puts survivors or refugees
  aboard — not in the roster, but mouths: `Ship.refugees` eat from the galley each tick and their
  crowding is a morale drag proportional to the complement.
- **Justice.** `Hearing` (console `ship hearing <crew> guilty|innocent`) is the brig's other half: an
  acquittal releases and lifts the confined, a conviction keeps them and weighs on them.

## The holodeck's uses, trauma and living conditions (2026-10-07)

`TestHolodeckAndQuarters`:

- **The holodeck** (`RunHolodeck`, console `ship holo <recreation|training|therapy|forensic> <crew>`)
  now has the uses the design names: **recreation** lifts the mood, **training** grants a credential
  (the same qualifications `Train` gives, earned with holodeck time), **therapy** fades the salience
  of what a character carries, and **forensic reconstruction** reports their account.
- **Memory is read by the simulation, not only by a query.** `Trauma` is the summed weight of a
  character's negative, still-salient marks, and it drags on their morale target: a crew carrying a
  death is measurably worse off than one that is not, until therapy fades it. This is the first place
  the memory model changes behaviour rather than answering a question.
- **Living conditions.** Each crew member has a `quartersQuality` (senior quarters better than
  bunked); it colours the morale target, and `ImproveQuarters` (console `ship quarters`) raises it at
  a cost in material.
- **The program that will not end.** Time in the holodeck accumulates (`holoCompulsion`); enough of
  it and a crew member stops standing watch, lost in the program, until command pulls them out
  (`EndHolodeckProgram`, console `ship holoend <crew>`).
- **The nacelle pylons.** `Ship.pylonHealth` is damageable — a hit on Main Engineering can buckle a
  pylon — and a ship without them **cannot go to warp** (`PylonsIntact` gates `Jump`). Console
  `ship pylon <amount>`.
- **The mobile emitter** as an artifact: `SetMobileEmitter` (console `ship emitter on|off`) lets the
  EMH hold the ward at 0.8 instead of 0.5 when the medical staff are down.

## EVA, first contact and the Maquis arc (2026-10-07)

`TestEVAAndContact`:

- **EVA.** A suited party already on the site reaches what the tractor and sensors cannot — the
  inside of a wreck, a belt's rock — with a spare EV suit (`EVA`, console `ship eva`).
- **The Prime Directive.** First contact with a pre-warp civilisation (`BEACON_PREWARP`): `ObservePreWarp`
  is free and good, `InterferePreWarp` takes material and medical supplies but records a **Prime
  Directive violation** as a `MEM_VIOLATION` mark on the whole crew and raises resentment.
- **The Maquis arc.** Resentment (`Ship.resentment`) seeds from the roster (a Starfleet crew with
  Maquis among it) and drives the morale drag; `ReconcileFactions` (console `ship reconcile`, by
  whoever commands) works it down, and at zero the two factions are one crew.

- **Airponics.** `SetAirponics` (console `ship airponics on|off`): the bay grows food without the
  replicators while life support runs it — Kes's answer to a tight power budget — so a crew out of
  rations can eat without spending material on replication.

The beacon-kind names were also centralized into one `BeaconKindName`, so adding a kind can no longer
index a stale name table (two such tables were already wrong when the sector grew).

## Borg strategic awareness, and memory moving the sim again (2026-10-07)

`TestAwarenessAndLoyalty`:

- **Borg strategic awareness.** `Ship.borgAwareness` rises with every Borg contact and while the Borg
  assimilate; above a threshold it makes more of the next sector theirs and speeds their adaptation
  (the same fire does less to a cube the Collective has already met).
- **Memory read by the simulation, a second time.** A **grudge toward whoever commands** (remembered
  valence below −0.3) is going through the motions: the post-holder's contribution is multiplied by
  `1 + bond`, so the post delivers measurably less — alongside the trauma drag. `Loyalty` reads it,
  and `CommandingIndex` finds who commands.

All of it survives a save and a load (format **version 25**; the memory model raises the save to a
few tens of kilobytes, the budget `docs/memory-and-consequence.md` names).

## The job queue (2026-10-07)

`docs/crew-work.md`'s queue now has a face and is saved (`Ship.jobs`, bounded, format **version 28**):
one **job** per thing that needs doing, with a **kind** (repair / seal / reclaim), a target, how far
it has got, and its place in the order. The queue is rebuilt from the ship's state each tick and
**ordered the way it is worked** — the captain's "see first to" puts a repair at the head (priority
−1), then the system priorities (0–17), then hull seals (20), then Borg reclamations (30). Console
`ship jobs` lists it, and a job that appears for the first time is written to the log
(`work ordered: repair sensors`), so a later failure can be traced to the work that was due.
`TestJobQueue`.

That order is not just the queue's: the damage-control party already works the same way — repairs by
priority with the command's override first, then hull sealing, then stripping a Borg deck — so the
queue is the honest face of the work rather than a second opinion.

**Build jobs** are the queue's fourth kind and the player's: `ship build <n>` (whoever commands) orders
`n` spare parts fabricated, and up to four free engineers turn the ship's **material** into parts over
crew-hours (`BUILD_HOURS_PER_PART`), so the crew make what they cannot buy. Build jobs carry through
the state-derived rebuild until they are done.

**Deferred maintenance** is now modelled as the thing that closes the Year-of-Hell loop: a system the
watch keeps up never wears, but a station **left undermanned** wears out, and after `MAINTENANCE_DAYS`
of it the system **fails** (loses health) — logged as `"<system> failed for want of maintenance"` and
queued as a repair, so a later failure is traceable to the work that was due (`TestMaintenance`).

## What this still needs

*Reconciled 2026-10-07 to the sections above, which each landed after this list was first written;
every earlier bullet here is now built and tested (materials, travel systems, EMH, credentials,
justice, grief, career, Maquis arc, Borg awareness, pylons, mobile emitter, holodeck uses, resource
acquisition, population pressure). What genuinely remains:*

- **The airponics bay as a mapped place.** The rules and the galley's fabrication exist
  (`SetAirponics`), but there is no room for it in the world; `docs/ship-master-map.md` puts it on
  deck 11, a published deck, so it waits on the deck-build path (the S8 published-deck blocker's
  sibling), not on rules.
- Every rate is invented and belongs in `docs/lore-ledger.md`.

## The memory model reaching dialogue (2026-10-07)

Asked to account for themselves, a crew member now **says a line drawn from their strongest mark**
(`AccountLine`/`SpeakFromMemory`, `module/crew/g_crew.cpp`): a rescue, a death they carry, a promise
the player made, a lie, an order, or a violation of the Prime Directive — or, if they hold nothing,
"All quiet, Captain." The line is theirs, not the player's, and it is spoken when the player
addresses them (the game's own use/response path). `scripts/speech-check.sh` (`g_shipTest 37`) gives
Tuvok a rescue and addresses him:

```
    SHIP: speech test: Tuvok carries a rescue; addressing them
    CREW: Tuvok says: "You came back for us. I have not forgotten."
PASS  a crew member speaks their account from memory when addressed
```
- Every rate is invented and belongs in `docs/lore-ledger.md`.

## The career in the world (2026-10-07)

The command console now carries the player's character and the career: it shows **YOUR CHARACTER**
(the rank and name the ship publishes) and, with `P`, sends the ship a **field promotion** for them.
The ship decides and answers — `Promote` in the core, published as `lwh_ship_promote` — and the
screen draws the confirmation, or the refusal in red. `scripts/promote-check.sh` (`g_shipTest 36`),
with the player in command and a character created:

```
    SHIP: promote test: Reyes is crew number 33 at rank 0
    SHIP: promote test: the ship says "Reyes is promoted to Ensign"
PASS  command's field promotion is confirmed by the ship, and the answer is published for the console to show
```

## The dilithium constraint that forces exploration (2026-10-07)

`docs/exploration-and-science.md`'s keystone, the ratchet that makes going home and staying alive the
same game. `Ship.dilithium` is the crystal's remaining life; every jump spends it, so warp use
directly spends the future. `Recomposite` (console `ship recomposite`) buys back 0.35 of the life in
the articulation frame while the ceiling falls 0.15 each time, so recomposition eventually cannot
reach the crystal and a **new** one is needed. A new crystal is found at the beacons already
reached: **mine** a resource belt, **trade** 40 material, **salvage** a derelict, or **research** a
better crystal (+0.10 efficiency, which shortens every jump) — canon's five ways with four built
(`AcquireDilithium`, console `ship acquire mine|trade|salvage|research`). The scoreboard is
`DilithiumRange`, the light-years the crystal still buys (`dilithium × 3,000 × quality`), and below
2% there is no warp at all: the ship still runs sublight, but home stops getting closer.

`TestDilithium` covers the decline, the ceiling drop, the refusal when spent, the reset by
acquisition, the efficiency of a researched crystal, the immobility with none, and the save;
`scripts/dilithium-check.sh` (`g_shipTest 38`) drives the status and a recomposition at the console:

```
    SHIP: dilithium 100% (ceiling 100%, quality 1.00, 0 replaced), range 3000 ly, warp possible
    SHIP: dilithium now 85% (ceiling 85%)
    SHIP: dilithium 85% (ceiling 85%, ...), range 2550 ly, warp possible
PASS  the dilithium crystal reports its life and range, and Engineering can recomposite it
```
