# Exploration and science: the other half of the game

The ship spends most of her time between crises, and canon's own answer to what happens then is *science*.
Voyager is an explorer before she is a warship: astrometrics was built by two crew members, probes are
spent like ammunition, and the ship's identity is that she studies things. This is the design for that half,
so the game is not only attrition with the occasional firefight.

## The four rules that make exploration a mechanic rather than a feature

1. **Exploration writes to the ship.** A scan is not a reveal; it is a row in the ship's **chart** that
   changes what the ship can *do* -- where she can go, at what risk, and with what confidence. If knowledge
   does not persist and change decisions, it is a minigame.
2. **Results are fallible.** Every chart entry carries a **confidence**. Low confidence is how a sensor
   reading becomes a trap: the crew course toward a quiet planet and find a gravitational shear. Without
   fallibility there is no tension in knowing things.
3. **Exploration costs the same currencies as everything else.** Deuterium, ship wear, and **crew hours** --
   the same people who hold posts and work the job queue. A science detour is therefore a *choice*: it
   competes with repairs, and it empties stations.
4. **Sometimes the science says do not touch it.** The most Star Trek outcome available, and the one that
   gives the crew agency: incomplete data is a legitimate reason to leave. Canon ends its best science
   episodes that way.

And the currency of exploration is **time**, which is the ship's own clock: a survey takes hours, the
journey home is measured in years, and every detour is legible in one number -- **distance remaining**.

## The chart, and the journey as a graph

Canon gives a hard number to hang it on: **75,000 light years**, about seventy-five years at warp 6.2. So the
Delta Quadrant is not a backdrop, it is a graph the crew progressively chart:

- each **system, anomaly, or object** scanned becomes a `chart` row: type, distance, confidence, hazard
  flags, resource flags, first-contact flags;
- a **course** can only be set toward something charted, or taken blind as a deliberate risk;
- each hop costs deuterium, warp-coil wear and crew fatigue -- the attrition model, with a direction;
- and the **astrometrics wall** (deck 8, canon: built by Kim and Seven with Borg technology) is the display
  of all of it: where we have been, what we know, and how far is left. It is the emotional centre of an
  exploration game, and the single best place to make the distance *hurt*.

## The mechanics

**1. The survey.** From the bridge or astrometrics: choose range, resolution, and focus -- stellar, planetary,
subspace. Longer scans reveal more and leave the ship sitting still; narrow focus is fast and misses
context. *Writes:* chart rows with confidence. *Fails:* a wrong row is worse than no row, because the crew
act on it. *Visible:* astrometrics, and the course-plotting screen that will or will not offer the option.

**2. The probe.** Consumable exploration (stores), and the safe way to look at something hostile: launch a
probe instead of the ship. Probe classes carry different sensor packages. *Writes:* stores down, chart up,
telemetry recorded. *Fails:* the probe is lost -- which is itself information -- or its telemetry arrives
with something on it that demands a response.

**3. The phenomenon.** A record with hidden properties: an anomaly whose attributes are revealed one scan at
a time, where the *correct* response (shield harmonics, warp geometry, distance, do not touch) depends on the
set revealed. *Writes:* the phenomenon's discovered attributes, damage if the crew guess wrong, and a
science reward -- data, a material, a shield modification -- if they get it right. *Fails:* misdiagnosis,
and the failure is a number the crew can watch getting worse. The two canon models to copy: a nebula the
crew enter for fuel that turns out to be a living thing, and a phenomenon that has been experimenting on
the crew without their knowledge.

**4. The away mission.** The ship sends *people*, which is the expensive part: crew leave posts, carry a
loadout from stores, and need a way home -- a transporter lock (canon range: 40,000 km standard, 10 km
emergency; blocked by shields, ion storms, and certain minerals) or a shuttle. *Writes:* crew location and
status, stores, chart, first-contact flags. *Fails:* the window closes, the interference rises, and the ship
must decide whether to leave people behind. That decision is the whole point of having a crew model.

**5. First contact.** A detected civilisation carries a **technology level** and an **awareness** value;
hailing, approaching, beaming down and scanning too hard all move awareness, and awareness decides whether
they hide, panic, arm, or follow. Reputation also travels ahead of the ship -- the Delta Quadrant is a small
place and stories move faster than Voyager does. *Writes:* reputation per faction, awareness, crew memory.
*Fails:* a first contact that goes badly and *stays* bad, breeding future scenarios.

**6. The lab.** Small, domestic science: samples brought back, analysed in the science lab (deck 8) or
sickbay (deck 5). *Writes:* research progress, which unlocks a treatment, a material, or a sensor
modification. It gives the labs a reason to exist, and it makes the *build* jobs in the crew economy produce
something the ship did not have.

**7. The board.** Why the crew explore at all, in one place: chart a route home, find the things that keep
the ship alive (fuel, materials, allies), investigate anomalies, answer distress calls. Canon's existing
mechanism is already in the save -- the single-player module persists the mission objective set and tactical
info -- so the board has a shipped home to grow into.

**8. The duty that is not adventure.** Exploration also produces the mundane: a science watch, a survey
rotation, a lab backlog in the job queue. That is where the two halves of the game meet: the same crew hours
that could repair a conduit could instead chart a system, and the ship is always behind on both.

## The constraint that forces exploration: dilithium

The owner's addition, and it is the ratchet the whole game needs: **the ship cannot go home without finding
dilithium, and finding it means exploring.** Canon supplies the numbers, which is what makes it a mechanic
rather than a mood.

### What canon actually says

- Dilithium moderates the matter/antimatter reaction. It **fractures** under use, and fractured crystals can
  be **recomposited** with a theta-matrix compositor *while still in the warp core's articulation frame*,
  which "significantly increased the lifespan of a dilithium crystal, but **occasional replacement was still
  necessary**". (Memory Alpha, *Dilithium*.)
- **An Intrepid-class warp core could last up to three years before refuelling.** That is Voyager's
  refuelling interval, and it is the single most useful number in this document.
- **In 2372 Voyager discovered a new form of dilithium in the Delta Quadrant** that stayed stable at much
  higher reaction rates, allowing far more efficient travel at high speed; it was installed aboard a
  shuttlecraft. So **finding better dilithium through exploration is canon**, not our invention.
- Recrystallisation is not universally available and access to it has been restricted -- so a facility that
  buys back crystal life is a *capability the ship has to hold onto*, not a free action.

### The arithmetic that makes it legible

Seventy-five thousand light years at warp 6.2 is about **seventy-five years**. Three years per refuelling
means the crossing needs roughly **twenty-five crystal supplies**, which puts each one at roughly **three
thousand light years of progress**.

That gives the game the scoreboard it has been missing: not a fuel gauge, but *this crystal is worth three
thousand light years*. Every decision can be expressed in the same unit as the goal.

### The loop

1. **Decline.** Crystal integrity falls with warp use, faster at high warp -- so the captain's speed choices
   directly spend the ship's future. This is the same coupling as the warp-damage thresholds, and it means
   "run hard to escape" and "run easy to last" are one decision, not two.
2. **Recomposite.** Engineering can buy back most of the loss in the articulation frame -- a **maintenance
   job** in the crew economy, costing crew hours and a facility, and giving the warp core the same texture
   as every other system: watched, worked on, argued about.
3. **Replace.** Eventually recomposition stops being enough and the ship needs *new* crystal, which is not
   aboard. So she must locate a source, which means **survey, chart, detour** -- the exploration mechanics,
   now with a reason nobody can ignore.
4. **Acquire, five ways, five stories.** Mine it (an away mission into a hazard); trade for it (canon's
   rule: the price is never only material); salvage it (derelicts, wrecks, someone else's bad luck); take it
   (reputation, and the crew will remember); or **research it** -- the lab's payoff, because a better crystal
   is a permanent improvement, not a refill.
5. **Improve.** A superior crystal raises efficiency, which *shortens the journey*. This is the one positive
   loop in an otherwise degrading game, and it is canon. Exploration must be able to make the ship better,
   or the whole design is only about losing slowly.

### The failure state is immobility, not death

With no dilithium there is no warp. The ship can still run sublight, still repair, still explore locally,
still keep her people alive -- **but home stops getting closer.** That is the correct failure for this
programme: it is attrition rather than a game over, it leaves the player with something to do, and a crew
who strand themselves through a series of bad calls have earned an ending rather than a bug.

### Why this is the keystone

It makes the two halves of the game the same game. Science and exploration are no longer an optional mode
alongside survival -- they are **how survival is paid for**. The chart is the map of where the fuel is. The
crew hours spent surveying are the same hours that could have sealed a hull breach. And the number that
measures everything, in the end, is a single one: **how far is left.**

## What each mechanic needs

- **Scan, chart, course:** tier 2 sensors, plus a `chart` table and a distance-remaining number.
- **Probes:** stores, plus a probe roster.
- **Phenomena:** sensors, damage, and a record with hidden attributes.
- **Away missions:** crew records, transporters, comms, and the decision to leave.
- **First contact:** reputation and awareness, which the scenario atlas already assumes.
- **Labs and the board:** the job queue and the shipped objective set.

Nothing here needs new art. It needs the ship to remember what she has learned, and to make the crew choose
between knowing something and keeping the ship alive.
