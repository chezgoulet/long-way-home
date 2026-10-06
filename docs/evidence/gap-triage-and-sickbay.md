# Evidence: the triage-and-sickbay gap, first slice

Date: 2026-10-07. The second of the five owner-approved gaps (`docs/gates.md`): *state* casualties
with severity, beds, the surgical bay, medical supplies; *control* who is treated first, who waits,
what is replicated; *visible* who is on a bed, who is on the floor, who is dead; *failure* a
casualty deteriorating because no bed freed up; *location* sickbay, deck 5. *Acceptance:* casualties
exceeding beds produce a decision rather than a queue that resolves itself.

## What is built

**State.** `CrewMember` gains `severity` (0 minor .. 1 critical) and a derived `underCare` (has a
bed this tick; not saved). An injury sets severity from its cause — airless time scales it, a fight
sets it at half — and treatment clears it. Sickbay is four beds: three standard and one surgical
(the surgical bay is lore; it is simply the fourth, given to the graver case). Stores gain
`medicalSupplies`, spent as treatment runs. Save format is now **version 9**.

**Decision.** `TreatCasualties` sorts the injured and gives the beds to the top of the order, then
treats them (as fast as sickbay's output, consuming supplies) while everyone left waiting gets
worse — `DETERIORATE_PER_HOUR`, and at severity 1 they die. With no bed, no sickbay output or no
supplies, nobody recovers, so the ward only holds and the queue does *not* clear itself. The order is
a standing order: `Ship::orderTriage`, **worst first** (default) or **rank first**, set by whoever
commands (`OrderTriage`). That is the decision the acceptance asks for.

**Visible.** The Sickbay console's readout is now
`INJURED n  BEDS n  WAITING n  LOST n  ASSIMILATED n  MORALE n%  FATIGUE n%  TRIAGE WORST FIRST`;
the command console lists `SICKBAY: WORST FIRST` among the standing orders and toggles it with `T`.

## What was proven

```
PASS  more casualties than beds is a decision, not a queue
```

`tests/ship/test_ship_core.cpp` (`TestTriage`): six casualties, exactly `SICKBAY_BEDS` under care;
under worst-first the beds hold the graver cases and the rest wait, and under rank-first the senior
cases do; with no medical supplies the untreated deteriorate and die while the stores stay empty.
`TestCasualties` still shows the airless deck injuring the engineering watch and sickbay bringing the
first patients back. The whole suite passes, as do `test.sh`, `scripts/s2-check.sh`,
`scripts/s4-check.sh`, `scripts/s10-check.sh` and `scripts/g3-measure.sh`.

## What is left for this gap

- **A triage screen.** The ward is a line of text; the contract's "who is on a bed, who is on the
  floor" wants a list a person can read and act on, one row per casualty. The console has the data
  (`underCare`, `severity`); it needs the drawing.
- **Replication as the control.** Medical supplies restock through the replicators; that tie is not
  made yet (the drivers exist — replicator output and stores).
- **The surgical bay's force field** and the EMH are named in the contract and not modelled.
