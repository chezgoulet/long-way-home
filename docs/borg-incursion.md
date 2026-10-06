# The Borg incursion: taking, holding, and reclaiming the ship

Owner's framing, 2026-10-05: the Borg board and assimilate ship and crew; the crew then have to
**de-assimilate** what the Borg changed, and the fight to retake strategic parts of the ship becomes a
Battlefront-like contest of places. This records the shape, the canon behind it, the one field the ship
model needs for it, and the two traps that would spoil it.

## Canon gives us the whole arc already

- **Boarding** happens after shields fail: tractor lock, shield defeat, then a cutting beam, transported
  drones, or both. So the incursion is a *consequence* of a defensive failure -- never a scripted ambush.
- **Drones ignore intruders** on their own decks unless interfered with. The Borg are not a patrol; they
  are an infection with a purpose, and that is more frightening.
- **A taken ship is stripped for parts** and, if possible, towed into a hangar. Capture is the real loss
  condition, not destruction.
- **Ships regenerate from damage and self-destruct when critically compromised** -- which is the shape of
  the counter-play: push them past a threshold, or they cannot be pushed.

## The one field the ship model needs

Everything below hangs on a single addition, and it should land *before* the incursion code does:

**Per compartment: `controller`.** One of `crew`, `borg`, `contested`, `sealed`, `uninhabitable`. The
Borg pressure system writes it; the crew's actions write it; the deck adapter reads it on load to decide
what the player walks into. Infection spread, retaking, sealing and scuttling are then all operations on
one field rather than a set of special cases. Build this first and the incursion is mostly data.

## What the Borg gain from each place they take

Strategic places are strategic because of what holding them *gives* the Borg. This is the raid map:

- **Main engineering and the warp core (deck 11)** -- power, and the loss condition: with the core they
  can breach it, eject it, or turn the ship's own energy against it.
- **The computer core and gel-pack grid** -- coordination, and **the crew roster**. This is the
  terrifying one: if they take the computer, they inherit what the ship knows, *including where
  everybody is*. Our own "know where everybody is" model becomes their intelligence. Nothing else in the
  design makes the ship's memory feel as dangerous as this.
- **Sickbay (deck 5)** -- bodies. Assimilating the wounded converts our own casualties into their
  reinforcements, which is the cleanest reason in canon to defend a medical bay.
- **The transporter rooms (deck 4)** -- mobility. With them, drones move between decks without using the
  corridors, which is how they bypass every chokepoint the crew sets up.
- **The shuttlebay (deck 10)** -- our way off and their way in. Losing it removes the option to leave.
- **Sensors and astrometrics (deck 8)** -- they see what we see, so hiding stops working.
- **Environmental control (deck 12)** -- atmosphere. Canon precedent for decompression as a weapon: with
  this, the Borg can empty *our* compartments rather than merely occupy them.
- **A vinculum aboard** -- if they establish one, they coordinate and adapt faster. Canon: every Borg
  vessel has one; destroying it severs local coordination, **but severed drones keep executing secondary
  objectives**. So a vinculum raid is a reprieve with a cost, never a win condition.

## No dwell, no write: the clean intercept

Owner's addition, 2026-10-05: if security reaches an intruder quickly -- before they touch a system,
assimilate anyone, or hold ground -- then **nothing happens to the ship.** Not a smaller consequence:
none.

That is the rule that makes security a job rather than a damage report, and it is best expressed as an
implementation principle:

> **The ship's state is only written by things that last long enough to write it.**
> Intrusion effects are *writes* to the model -- `compromise`, `controller`, crew records, the job queue
> -- and every write requires **dwell time**. Below the threshold, nothing is written.

So an intrusion is not an event that causes damage; it is a **clock the crew can beat.** Which turns the
whole mechanic into a race between two durations:

- **time to effect** -- how long the intruder needs, unopposed, to reach a system, to begin slicing, to
  start assimilation, to hold a compartment;
- **time to intercept** -- detection, alarm, the duty roster, where the security team is standing, how
  fast the corridors and turbolifts carry them, whether the hatch can be sealed to buy minutes.

Both are clocks, both are tunable, and neither needs a shooting engine to be tense. A turbolift that is
two minutes away because it is carrying engineering to a repair is a *tactical* consequence of the crew
economy in `docs/crew-work.md` -- which is exactly the kind of coupling this programme is for.

### Thresholds, not a switch on boarding

Per intruder group: `arrived` -> `undetected` or `detected` -> `engaged` -> dwell. The dwell timer writes
nothing until it crosses its first threshold, then:

- **first threshold** -- the compartment's `compromise` begins (someone is in the wiring);
- **second** -- `controller` becomes contested, the intruders hold ground;
- **third** -- Borg: assimilation of people and the compartment; others: systems seized outright.

Security action reduces or resets the dwell; killing or capturing the group ends it. And the thresholds
should be visible to whoever is watching -- a security console that says *contact, deck seven, no systems
touched* is the crew being told they are *winning*, which no attrition game gives them often enough.

### Undetected is the dangerous case

The corollary, and it is where the threat lives: **if the intruders are never detected, nobody is coming,
and the clock runs to completion.** With no-impact intercepts available, an enemy that wants to hurt the
ship must arrive unnoticed, arrive in numbers, or create a diversion -- which is what a competent enemy
does, and what canon shows (transported drones, a boarding party in a shuttlebay, an ally who is not
one). Detection is therefore a *security capability* the crew can be good or bad at, and the department
has something real to fail at besides shooting straight.

### A clean intercept is a win, and must be recorded as one

Because the programme's whole feel is attrition, the counter-beat has to be recorded: **no damage, and a
line in the record.** *Security intercepted two intruders in the shuttlebay; no systems compromised; no
casualties.* Named crew -- the one who spotted them, the team that responded -- belong in it. A game
where the crew can only lose slowly also needs to tell them, explicitly, when they did the job well.

**One tuning warning.** If interception is easy, the threat is empty; if the first threshold is tight,
every stray intruder is a catastrophe and the big ones stop being frightening. The dial is the ratio
between time-to-effect and time-to-intercept, and it is the single number to tune hardest.

## Growth, not respawn

Battlefront's *feel* transfers: sector-by-sector fighting over places that matter, with chokepoints and
consequences. Its **economy must not.** There are no Borg spawn points and no reinforcement tickets. The
Borg instead **grow from what they hold**: a captured sickbay produces drones from our wounded, a
captured engineering feeds their systems, a captured transporter moves them. Retaking a section is
permanent -- the Borg do not take it back by spawning, only by spending what they have left. That is how
a Battlefront-like contest survives a game with no reset button.

## De-assimilation: possible, and never complete

This is the best part of the owner's idea and canon is unusually generous with it:

- Reversal is possible only in a **narrow window** -- nanoprobes are counterable in the first minutes.
- Recovered people are **never fully restored**: Picard's trauma and alterations lingered for decades;
  Seven of Nine was never fully restored after twenty-four years.
- 80,000 severed drones removed "most, but not all" implants and became their own society.
- A vinculum that survived destruction later infected Seven.

So the rule for us: **you can reclaim a deck, and the deck is not the same afterwards.** Borg
modifications leave permanent residue -- a conduit that runs hot, alcoves that cannot be fully stripped,
a bulkhead that was cut open and plated over. And a crew member who was assimilated and recovered carries
it in their record forever, which the crew model already supports. The reclaimed ship is a **Changed
ship**, which is exactly the north star: no reset, only consequence.

## The counter-play kit, and the pace trap

Canon gives the crew real tools: **force fields** (rated 1-10; a level-10 field can cut a drone from the
Collective), **explosive decompression** (canon: an entire cargo bay, with only Seven surviving by
clinging to a ridge), **weapon remodulation** (the phaser adapter's rotating modulation allowed at most
twelve shots before adaptation), **nanoprobe reversal** in the minutes after injection, and **the
vinculum raid** above.

**The trap: adaptation cannot be absolute.** Canon's Borg nullify nearly any energy weapon within
minutes and adapt collective-wide, so a literal implementation ends every firefight before it starts and
the game becomes hide-and-seek. Therefore adaptation must be (a) **per weapon type**, (b) **counterable
by remodulation** at a cost in time and attention, and (c) **tied to a resource the crew can attack** --
killing a vinculum should buy back the crew's weapons for a while. Without those three, a
Battlefront-like fight is not merely hard, it is impossible: you cannot have a firefight with an enemy
who is immune to fire.

**The second trap: the fight has to be winnable at the level of places.** Retaking needs to be possible
with the crew we have, which means the crew are **defenders at posts first and a squad second**. Our crew
layer positions people and holds stations; it does not yet command a fireteam. So sequence it: security
holds a junction and engineers seal a hatch *before* anyone expects the crew to advance room by room.

## The loss condition, and it must be legible

Borg reach the core: the ship is taken, or scuttled by her own crew. The player must be able to *see*
that path -- decks lost, systems taken, the countdown -- because a loss nobody saw coming is not
attrition, it is a bug report. The time-to-organise clock in `docs/ship-systems.md` is the visible
instrument; this document is what the clock is counting toward.

## Sequencing

Needs, in order: the compartment model and `controller`; power, doors, fields and life support (tier 1);
weapons and shields (tier 2); damage and the repair economy (tier 3) -- because a Borg incursion is only
frightening if the ship can be hurt and the crew can be lost; then the incursion itself. Building it
before tier 3 gives a threat that cannot wound and a crew that cannot be spent.

## Owner's answers (2026-10-05)

The three questions are settled, and they change the model:

- **The player can be assimilated, and can be pulled back out.** Assimilation is a window, not an ending
  -- for the player as much as for the crew.
- **Incursions can end well.** The Borg can only assimilate a place if they are left alone with it for a
  while, so the crew's job is often *interruption* rather than extermination. One precision to keep both
  halves true: **wins are possible; clean wins are not.** You get the ship back, never for free -- the
  residue stays.
- **Reversal gets harder the longer it has been.** So it is a progress value with a rising cost, not a
  binary window.

## Two kinds of intruder, and why the model needs two fields

The owner's third answer is a design correction: **other races do not assimilate.** Only the Borg take
people and places. Everyone else -- scavengers, raiders, whoever boards -- takes **systems**: they hack,
slice, spoof and seize control. That is why the system-hacking features exist, and it means a single
`controller` field is not enough.

Two payloads, therefore two fields per compartment:

- **`controller`** -- who is physically there: `crew`, `borg`, `contested`, `sealed`, `uninhabitable`.
  The Borg write this, and only the Borg do. Assimilation converts *bodies and places*.
- **`compromise`** -- whether the systems in that compartment can be trusted: `clean`, `sliced`,
  `spoofed`, `locked_out`. Intruders of any race write this, and so can a clever enemy who never sets
  foot aboard. Hacking converts *systems*, and costs no lives directly.

The combination is the interesting part: a compartment can be **held by the crew and compromised** --
you are standing in it, and the door is theirs, the sensor feed is lying to you, and the console will not
answer. Borg takeover and system compromise are orthogonal, and a ship can suffer both at once: drones
coming up a Jefferies tube while the bulkhead controls are in someone else's hands. That is a much better
disaster than either alone.

They also need **different detection**, which is a gift for tension. The Borg are *visible* -- drones,
tubules, the green of a converted conduit. A compromise is *invisible* until it bites: a door that will
not open, a reading that does not match the window, a crew roster with a name on it that should not be
there. **The Borg are a fire. A hack is a leak.** The crew need a way to find the leak -- diagnostics,
log audits, an engineer who says the numbers are wrong -- or the threat has no counter-play at all.

## Reversal as a decay, not a window

With the owner's answer, reversal becomes a value rather than a gate:

- per crew member, an assimilation **progress** (the canon five stages) that advances while they are held;
- such a person can be recovered, and the **cost of recovery rises with progress**: more medical time,
  more materials, more risk of failure, and a higher chance of lasting damage;
- and reclamation of the ship is the same shape, done by *work* rather than medicine -- see
  `docs/crew-work.md`, where stripping Borg modification is one of the four activities the crew do.

## Implemented (2026-10-07)

The keystone and the owner's clean-intercept rule are in, in `module/ship/ship_core.cpp` (save format
32; `TestIncursion`, `scripts/controller-check.sh`):

- **`controller` per deck** -- `crew`, `borg`, `contested`, `sealed`, `uninhabitable` -- computed each
  tick from the ship's own state and reported at the console (`ship controller`).
- **`compromised` and `dwell` per deck.** `dwell` is the ship-seconds the intruders have held
  *unopposed*; `compromised` is set the moment any system on the deck loses control to them (or
  assimilation begins). The console shows both.
- **The clean intercept, recorded as a win.** When intruders are cleared from a deck they never
  compromised, the ship writes it down and increments `cleanIntercepts`: *"<team> cleared the
  intruders on deck N; no systems compromised"* -- names in the log's author field.

`scripts/controller-check.sh` boards deck 4, lets security outnumber the raiders, and shows the field:
`deck 4 crew, intruders 2` at first contact, then `0 deck(s) not wholly ours; 1 clean intercept`.

- **No dwell, no write, enforced, in three thresholds.** An unopposed intruder must hold a deck for
  `DWELL_COMPROMISE` (**120 ship-seconds**) before anything here is written: a system's control does
  not fall, the Collective does not learn, and nobody is taken until the clock crosses it. At
  `DWELL_HOLD` (**240 s**) the deck counts as **contested** -- they hold ground. At `DWELL_SEIZE`
  (**360 s**) their systems are **seized outright** (for anyone but the Borg, whose path is
  assimilation). Below the first threshold the crew still hold the deck and nothing is written;
  security reaching them inside the window is a genuine reprieve, and the clean intercept above is
  that reprieve recorded. `TestIncursion` walks all three thresholds on one deck: compromised but
  crew-held, then contested, then the transporter seized.

- **The counter-play kit: remodulation and a vinculum raid.** `Remodulate` rotates the phaser
  modulation, subtracting 0.5 from the Borg's `adaptation` so the next shots land again, with a
  120-second cooldown (the cost in attention). `RaidVinculum` needs Borg here (aboard or a vessel),
  sets `adaptation` to zero and suppresses its growth for 300 seconds -- a reprieve with a cost, never
  a win -- and spends a security party (one is wounded). Console `ship remodulate|vinculum`;
  `scripts/counterplay-check.sh` shows a 60% adaptation broken to 10%.

- **De-assimilation: a narrow window, and never whole.** A crew member the drones have begun to take
  (their `wounds` in `(0, 0.8)`) can be recovered: `RecoverCaptive` spends sickbay and supplies,
  *rising with how far it got*, and returns them **changed** -- a permanent `assimScar` that drags
  their morale target forever, and a `MEM_VIOLATION` mark (*"I was taken, and I came back changed"*).
  Past 0.8 it is too late. Console `ship recover <crew>`; `scripts/recovery-check.sh` shows a 50%
  captive recovered with a 50% scar; `TestDeassimilation`.

- **Force fields, rated 1-10.** `SetForceFieldLevel` rates a field 0..10 (the air-holding field is a
  level 10). While a field stands it **holds** the intruders on that deck -- no hacking, no advance --
  and **drains** under their pressure (`free intruders × minutes / 5` levels), failing when it empties.
  A **level-10 field on a Borg deck cuts them from the Collective**: adaptation is suppressed while it
  holds. Console `ship field <deck> <1-10|on|off>`; `scripts/forcefield-check.sh` shows a level-10
  field holding two raiders and draining to 9; `TestForceFieldKit`.

- **The security squad: a fireteam command sends to retake a deck.** `OrderAdvance` (console
  `ship advance <deck>`) musters security at deck 9 and advances **one deck at a time** (3 minutes a
  deck); in position the squad fights there until the deck is clear, then stands down and the deck is
  ours again (`compromised` cleared). Console `ship advance`; `scripts/squad-check.sh` shows a squad
  ordered to deck 4 and the report `0 deck(s) not wholly ours`; `TestSquad`.

**Still to come:** the crew layer embodying the squad as bodies moving deck by deck (the design's
"room by room", which needs a session to see); the threshold ratio is the number the design says to
tune hardest (120 / 240 / 360 seconds is our first cut). Otherwise the incursion is built to the
design: controller, compromise and the three thresholds; the clean intercept; remodulation, a vinculum
raid, rated force fields; de-assimilation; and the squad.
