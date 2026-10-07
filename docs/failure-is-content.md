# Failure is content

The owner's rule, 2026-10-05: **system fail states and player error are canonical content, and embraced.**
Mishandle a transporter and someone comes back wrong -- or comes back twice. Stress the warp core too long
and you risk a breach. This is the identity of the game, not a concession to it.

## Why it is the right rule

The north star is attrition with no reset, and a ship that cannot fail in *interesting* ways has no stakes.
More to the point: **canon is made of these failures.** Voyager's own episodes include a crew member created
by a transporter merge, a duplicate left behind by a transporter accident, a duplicate ship and duplicate
crew in "Deadlock", transporter psychosis from inadequate pattern buffers, warp breaches that are "almost
always irreversible" once safety systems fail, core ejections that are "often unreliable", holodeck
safeties defeated, inertial dampers and gravity failures that kill people, and consoles exploding in
people's faces because the EPS grid was overloaded.

None of that is decoration. Each one is a scene, and in our game each one should be a **consequence of
something a person did**.

## Two clocks at different scales

This is the rule that reconciles failure-as-content with the earlier intrusion design:

- **Ship systems need dwell time to fail.** They degrade where the crew can watch: a gel pack running warm
  for two days, a conduit reading high, a crystal down to sixty-two percent.
- **Operators can fail instantly.** A transporter mistake, a sealed hatch behind someone, a decompression
  ordered without checking the compartment, a core stressed one command too far.

So the ship's degradation is slow, legible and arguable; the operator's error is fast, personal and
irreversible. Both belong in the design, and they are different kinds of threat. The first asks *what do we
spend*; the second asks *did you understand what you were doing.*

## Three severities, every system

Design each system with three failure states, so failure has a range rather than a switch:

1. **Degraded** -- it still works, worse. Output down, range shorter, a scar that will not heal. Recoverable
   with maintenance in the job queue.
2. **Acute** -- it hurts now: the breach countdown, the pattern starting to drift, the injury, the deck
   losing atmosphere. The crew act, or it becomes catastrophic.
3. **Catastrophic** -- permanent. Death, loss, transformation, or the ship.

**The catastrophic tier must open content, not merely subtract it.** Canon shows how: a transporter accident
that duplicates a crew member does not kill the episode, it creates a *person* with an identity problem and
a captain who has to decide what to do about them. A merge creates one crew member out of two and asks who
they are. A mangled arrival is a sickbay problem, a morale problem and a command failure all at once, and it
leaves a mark on the ship's record.

## Condition sets the odds, and stress sets the severity

The owner's ruling, 2026-10-06, and it is the mechanism that makes the three rules further down true rather
than aspirational: **a system's chance of failing is a function of its condition, and this is general -- it
applies to every system, not to the transporter alone.**

- **Within the top tenth of capability** -- power, maintenance, parts -- the system is nominal: it does what it
  says, with no consequence. A well-kept transporter does not mangle anyone.
- **As capability degrades, the odds of an anomaly rise.**
- **At severe degradation, or under severe stress, it lets go visibly** -- the console arcs, sparks and burns
  at the station the operator is standing at.

Which settles an old question cleanly. A duplicate, a merge or a mangled arrival is neither a prohibition nor
a roll: it is **what a degraded transporter does**, and a competent operator maintains the system or declines
to use it. Copy, merge and mangle are all back in play, and none of them is a reset button, because a copy is
still not the person -- a new record, an ambiguous legal status, and a crew who know.

Two refinements:

- **Degradation sets the odds; stress sets the severity.** A forty-percent system run light gives a misaligned
  beam; the same system at battle stations gives a merge. That makes *do not run the degraded system under peak
  load* a real operator skill, and it gives the crew's skill ratings (`docs/crew-roster.md`) something to tilt.
- **The severity ladder is the one above.** Degraded, acute, catastrophic. One curve, one doctrine, no new
  vocabulary.

**Two consequences worth carrying.**

The instrument must be honest and specific. `docs/the-record-and-the-log.md`'s rule that instruments cannot lie
means the console states *this* system's condition before it is used, at the level of *"the pattern buffer is
at sixty-two percent, and below forty I would not send anyone."* A competent operator has the information, and
that is a promise the panel has to keep.

And **the sparking console forces the player's own body to exist.** `docs/gap-analysis.md` lists the player's
body as unspecified, and a station that can maim the person operating it makes it load-bearing: bridge
consoles erupt because the EPS grid is degraded, at the one place on the ship the player is standing. The
captain cannot fix anything, and the bridge is not safe either.

## Failure writes to the crew records

The crew model is what makes failure land, and each kind of failure writes differently:

- **Mangled**: the record gains injuries, permanent if they are bad enough. The person is still the person.
- **Cloned**: a **new record** with the same face, an ambiguous legal status, and every relationship they had
  now doubled. Whether both live is a decision the ship has to make.
- **Merged**: one record replaces two. Somebody is gone, and the resulting person is not either of them.
- **Assimilated and recovered**: the record carries it forever, per the Borg design.
- **Dead**: the record closes, the post empties, and **the roster promotes to fill the gap** -- which is how
  rank rises in a run with no reset button.

## The three rules that keep it fair

Failure-as-content becomes cruelty if it is random. So:

1. **You can see the risk before you act.** The console says the pattern buffer is degraded, the core says the
   coolant is falling, the compartment says pressure is dropping. A competent operator has the information.
2. **The consequence traces back to what you did.** *I stressed the core for twenty minutes and it blew* is a
   story. *It blew* is a bug report.
3. **A competent path avoids it.** There is always a way to do the thing carefully, slower, at a cost. Failure
   should be what happens when you decide the cost is worth paying, or when you did not understand the system.

Rolled catastrophes feel like bugs. Earned ones become war stories, and war stories are what the crew's
memory is made of.

## The one exception

**The warp core breach is the real ending.** The ship is gone, and unlike a transporter accident or a lost
shuttle, there is no content on the other side of it. It should therefore be reachable only by a *chain* of
decisions -- skimping on coolant, running past the limits, refusing the ejection -- and never by a single
roll. Everything else in this document is a fork; that one is a wall, and the crew should be able to see it
coming for a long time before they reach it.

## Acceptance

- Every modelled system has three failure states designed, not just one.
- Each catastrophic failure has a written consequence in the crew records, the ship state or the chart.
- Warnings exist for every failure that a competent operator could avoid.
- After any catastrophic failure, the ship's log can narrate *why* -- action, consequence, and the decision
  that made it.
- The core breach is gated behind a chain of decisions, and is the only unwinnable end state in the design.
- Condition sets the odds across every system: one in its top tenth of capability behaves nominally and carries
  no consequence, while the same system degraded and under load can spark at the station the operator holds.

## Implemented (2026-10-07)

The warp core breach is now a **chain with interrupts**, in `module/ship/ship_core.cpp` (save format
39; `TestCoreCascade`, `scripts/core-check.sh`). A failing warp drive loses **coolant**; low coolant
raises the core **temperature**; over half temperature, **containment** falls; at containment
critical a **breach countdown** begins. The console (`ship core`) shows coolant, temperature,
containment and the countdown — the risk is visible before the wall. The chain is interruptible at
several points, exactly as the design asks: `ship core coolant` (refill the loops, at a cost in
material), `ship core shutdown` (stop the cascade and give up warp), `ship core restart` (when
containment is back above half), `ship core eject` (the canon last resort — no warp until a new core),
and plain repair of the warp drive. Ignored past the countdown, the ship is **lost** — the one
unwinnable end.

Not built: each modelled system's three designed failure states (this covers the core; the other
systems still fail one way), and the written consequence of every catastrophe in the crew records.

### System failure states (2026-10-07)

The acceptance's first line -- *every modelled system has three failure states designed, not just
one* -- is now mechanised (`FailureStateOf`, `SystemStateName`, `UpdateSystemStates`; save format 40;
`TestSystemStates`). A system is **nominal**, **degraded** (health below two-thirds), **offline**
(health below a third, or switched off) or **destroyed** (health zero), and every change is written
to the log by name (`"<system> is degraded"`, `"<system> is restored"`) -- the risk visible before
the wall, and the consequence traceable. The state is stored so a load does not re-narrate it.

### Condition sets the odds, and stress sets the severity (2026-10-06)

The ruling of this section is now mechanised, generally, in `module/ship/` (`SystemCondition`,
`AnomalyOdds`, `AnomalySeverityFor`, `RollAnomaly`, `UseSystem`; save format 42;
`TestConditionOdds`, `TestTransporterAnomaly`, `scripts/condition-check.sh`). A system's condition --
its health capped by the power reaching it -- sets the chance of an anomaly, zero in the top tenth;
the load at the moment of use sets the severity (degraded, acute, catastrophic); a bad use is written
to the log with its condition, load and author, and at the severe end the console lets go at the
operator. The transporter is worked end to end -- the pattern buffer states its condition before the
act, a nominal beam mangles no one, and a degraded one yields a misaligned beam, a mangled arrival, a
copy or a merge, each written to the records. Evidence: `docs/evidence/condition-sets-the-odds.md`.
The measurement over twenty thousand draws is in that document; the same degraded condition is
degraded-light and catastrophic-battle, as the ruling asks.
