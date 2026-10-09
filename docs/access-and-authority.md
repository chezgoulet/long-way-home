# Access and authority: two different locks

The owner's model, 2026-10-05: **rank and role grant technical access to the ship, and the constraints lift as
you go up.** The captain can therefore operate the entire ship alone if everyone else is dead -- a capability
that is always held and almost never used, because in practice the captain simply tells people what to do. The
first officer holds the same kind of ship-wide access, and every post below is scoped down the chain.

## Two axes, kept apart

This only works if two different things are not confused:

- **Access** is *technical*: will this console accept input from this person? Hard, binary, rank-derived, and
  testable. The captain's console accepts anything; a junior's accepts its own post.
- **Authority** is *social*: will the crew carry the order out? Soft, probabilistic, and governed by morale,
  respect, and how the last order went -- which is why command can be lost (see `docs/start-states.md`).

So: **rank controls access absolutely; rank governs compliance probabilistically.** A captain alone with the
crew dead has full access and no compliance problem. A junior at the captain's console has no access at all.
A captain giving an order the crew believe is suicidal has access to the console and a mutiny on their hands.
Both locks, both real, and they should be modelled separately because they fail differently.

## The scope ladder

| scope | who | what it means in practice |
|---|---|---|
| **ship-wide** | captain, first officer | every control on every deck, plus allocation, authorisation and override |
| **department** | department heads | all systems in their section: engineering over power, warp, EPS; medical over sickbay and the EMH; security over doors, force fields and the brig |
| **post** | everyone else | the systems their station actually operates, and nothing else |
| **read-only** | crew in transit, visitors, passengers | status without command |
| **none** | intruders, and anyone the crew have cut off | the console does not answer |

## Canon already has this, and it is specific

Warp core ejection requires **the chief engineer's or senior staff's authorization code** -- not a rank in the
abstract, a *credential* held by particular people. Red alert can be raised by more than the captain. The ship
is canonically operable with as few as a hundred crew. Every one of those is the same mechanic: some controls
are behind a name, and the name can be delegated.

Which gives access four sources, in order of how often they matter:

1. **The post you hold.**
2. **Credentials you have earned** -- cross-training, a qualification, a code given to you for one task.
3. **Delegation** -- a department head grants a junior their section's access for a shift, and can take it back.
4. **Emergency override** -- slow, loud, logged, and usually needs two people to agree.

## When everyone is dead

The full-access fallback exists for exactly this, and the paths down to it should be graceful:

- **Field promotion first.** If the senior officer is gone, the roster promotes by the same rules that run
  during play, and the surviving crew inherit the access that comes with the post. This is the normal path.
- **Override second.** A skeleton -- or one person -- can force the ship to accept them, at a cost: it is slow,
  it is logged, and it is the kind of thing a board of inquiry asks about later.
- **The EMH third.** Canon gives the hologram authority in emergencies, which makes the Doctor a legitimate
  holder of the keys when nobody else is left. A ship run by its doctor and its computer is a very Star Trek
  emergency, and it is free content.

## What the console shows, which is how the hierarchy teaches itself

A locked control is **visible and named**: not hidden, and not silently inert. It says what it needs --
*engineering authorisation required*, *medical override*, *bridge command* -- which does three things at once.
It teaches the player the chain of command without a manual, it answers "who do I wake up" mechanically, and
it makes the moment access is granted feel like something.

## The cost of using the top

The captain can do everything and can only be in one place. Operating a console means leaving the bridge, which
means the bridge is unstaffed. So the admin capability is a *rescue valve*, not the normal mode -- and using it
should feel like an emergency even when it works, because the ship's command is now standing in the coolant
bay instead of watching the sensors.

## Three edge cases worth modelling, because they are the interesting ones

- **A compromised system does not obey rank.** A sliced console (see `docs/borg-incursion.md`) refuses
  everyone, including the captain, until someone clears it. Hardware beats hierarchy.
- **A dead officer's credentials are retained; using them is reported, not built — corrected 2026-10-09.**
  The computer does not know they are dead, so the credentials are **retained**; but *using* them is a
  deliberate act the console does not yet offer. This is **reported, not built**, and the older phrasing
  ("still usable") is withdrawn rather than kept as a promise the code cannot keep (see the acceptance
  item at the end of this document, and `docs/omissions.md` **W1**). A silent use of a dead officer's authority would
  weaken the scene the edge case is for. The original line read *"Using them is possible, sometimes
  necessary, and ethically its own scene"*; that is the withdrawn promise, not the build.
- **The crew can revoke.** If command is losable, access is too: a relieved captain's credentials stop working,
  and the ship records who turned them off.

## Remote call-up, and the lock-out (owner decision, 2026-10-07)

Item 2 of the owner's mid-flight addition, and it settles the conflict the addition names rather than
smoothing it over.

**The conflict.** "The captain can do everything and can only be in one place. Operating a console
means leaving the bridge, which means the bridge is unstaffed." On the original reading, full access
was a *rescue valve*: it was expensive because using it meant walking to the system. If a senior
officer could operate anything from anywhere, that cost would disappear.

**The resolution.** Call-up and **command** travel remotely; **physical work does not**. A senior
officer above the post-holder may call up any system's controls from any console on the ship,
command it, and lock the post-holder out. But a repair, a seal, a hatch, a valve or a hand on a
control still needs someone standing there -- which is why the crew's physical jobs (`docs/crew-work.md`)
are unchanged and still cost bodies. The bridge stays staffed because operating remotely still means
the officer is *at a console*: they have not left it, and they cannot be at two. The cost has moved
from "walk to the system" to "you are watching the ship from the one station you chose", which is
what the design wanted the top of the ladder to feel like: capable, and single-threaded.

- **The lock-out is a first-class act.** A senior officer locks a post-holder out of a station's
  controls. It is logged and it names its author; **the person locked out is told by name on the
  console that has stopped answering**; and it writes a negative, saw-it-myself mark on that
  person's record (`MEM_LOCKOUT`, `docs/memory-and-consequence.md`) -- so being shut out of your own
  engine room by your own captain is remembered, by you and by the bond between you.
- **Remote command is access, not authority.** It does not make the crew carry an order out, and it
  does not clear a compromised system: a sliced console refuses the override and the lock-out as it
  refuses rank. Hardware still beats hierarchy.

**Location appropriateness** (owner addition, item 1). A console controls what its location is for:
the transporter room's console opens on the transporters, astrometrics on the sensors, environmental
control on life support, the Conn on the helm. The system→deck map in `docs/ship-master-map.md` is
the check; two entries in the simulation's system table disagreed with it and were corrected (the
computer core and the torpedo launchers are on deck 10, not deck 9). The panel-to-console table in
`module/ui/ui_lwh_engineering.cpp` now carries a focus system per panel.

## Reading and operating are different privileges (owner ruling, 2026-10-07)

A system is **operated from exactly one station** -- the S4 invariant, unchanged. That is not the
same as being *seen*. Reading and operating are two privileges:

1. **One station operates each system.** Communications is Operations'; sensors are Operations';
   the transporter is Operations'. The station that operates a system is the only one that may change
   it (`OperatedFrom`, and the ship's command guard in `g_ship.cpp`).
2. **Reading is separate, and wider.** A station may **read** a system another station operates and
   **cannot change it** (`StationReads`). Concretely: **Tactical reads the comms traffic; Operations
   speaks.** Tactical also reads the sensor picture, internal and external. Sensors and comms are
   Operations' to *operate*; Tactical's to *read*.
3. **The screen must show the difference.** A control is drawn as a control (a priority number, a
   switch) and a read as a readout (tagged `RD`, in the read colour, with no switch). A read is never
   a control by accident: the clearance model gates the two separately, so pressing `ENTER` on a read
   does nothing and the ship would refuse the command anyway.

## Two axes: the station shapes the console; the person filters it (owner ruling, 2026-10-07)

Two rules that must not be conflated, the owner's correction of an earlier over-broad statement:

1. **The station decides how many content types the console carries, and the default is one, well.**
   A console gets more than one content type **only where that post actually needs it** -- multi-content
   is *earned by the job*, not granted by default. A junction panel that shows one readout stays a
   panel with one readout; do not put a menu on it. The failure to avoid is every surface in the ship
   becoming a swiss-army console. In the fleet as built: **Tactical** carries a read layer because the
   tactical job needs the sensor and comms picture; **Sickbay** carries one because the medical job
   needs the record and what the Doctor is running. **Engineering, Operations and the Conn carry one
   content type each** and publish no read layer.
2. **The person filters it, and that is always true.** Every console gives the player access to
   whatever the player *already* has access to. What a console offers is decided by the **permission
   boundary**, not by a hardcoded menu -- a console is never a fixed list of what someone is allowed
   to do. Whatever clearance you hold is reachable from wherever you stand: the remote call-up,
   stated as a general rule. A captain at a junction panel sees one readout and can still reach
   everything his clearance opens.

The two could quietly break each other at a **single-type console**: if "one content type" were read
as "one capability", the single-type screen would stop reaching what its holder is cleared for. It
must not. From the Conn console -- one content type, four systems -- a commander still operates a
system stationed elsewhere, and an uncleared ensign is still refused.

## Acceptance

- Access and authority are separate quantities. **Access is rank-derived, binary and testable;
  authority is soft and morale-governed; a morale of zero does not change what a console accepts**
  (unit-tested: the same crew member's access is identical at morale 0 and morale 1). *Built.*
- A locked control is **visible and named**: it says what it needs and who can open it -- the station
  and its head, a lieutenant commander, or a delegation for the shift. *Built* (`AccessRefusal`,
  drawn by the consoles; `scripts/clearance-check.sh`).
- The same action is refused at one post and permitted at another, and the refusal is legible on the
  console; a system not worked from a station names the station that works it. *Built*
  (`OperatedFromRefusal`, `scripts/s4-check.sh`).
- **Delegation and revocation** both work, and both are recorded in the log. A department head grants
  a junior their section's access for a shift (default eight hours), and can take it back; the log
  names who turned it off. The crew can also revoke a **credential**. *Built* (`Delegate`,
  `RevokeDelegation`, `RevokeCredential`; save version 48).
- **Emergency override**: slow, loud, logged, and usually needing two people to agree. One person may
  force it alone, but it takes longer (45 minutes against 15), the log says so, and the crew take a
  small morale cost. It grants technical access to one station for twenty minutes and does not clear
  a compromised system. *Built* (`BeginOverride`, `ConfirmOverride`, `UpdateAccess`).
- The captain can operate the entire ship alone -- and the cost of doing so is the *position*: one
  console, one place, and the bridge therefore unstaffed while standing in the coolant bay. *Built*:
  rank 4 and above is ship-wide, and remote call-up means they need not walk, but they must still
  stand at a console and can stand at only one.
- **Remote call-up and the lock-out** (new): a senior officer calls a system up from a console away
  from it and commands it; physical work still requires presence; a lock-out is logged, names its
  author, tells the person locked out by name, and marks their record. *Built* (`MayCallUp`,
  `LockOut`, `ClearLockout`, `LockoutNotice`; `scripts/clearance-check.sh`).
- A sliced system refuses everyone, including the captain and an override. *Built* (`Hijacked`, the
  switch/priority guard in `g_ship.cpp`).
- **Reading and operating are separate** (owner ruling, 2026-10-07): a system is operated from exactly
  one station; another station may read it and cannot change it; and the screen shows the difference,
  a control beside a readout tagged `RD`. *Built* (`StationReads`; `lwh_ship_reads<N>`; the console's
  `RD` rows; `scripts/screens-check.sh` and `TestPhaserYieldAndReading`).
- **The station shapes the console; the person filters it** (owner ruling, 2026-10-07): a console
  whose post needs one content type has exactly one -- Engineering, Operations and the Conn stay
  single-type, shown by the screenshot `lwh_conn_single.tga` and an empty read list; Tactical and
  Sickbay carry a read layer, the job named as the reason; and from every console, including a
  single-type one, the player reaches what their clearance opens (a commander at the Conn reaches
  sensors, an uncleared ensign is refused). *Built* (`scripts/screens-check.sh`, `g_shipTest 68`).
- **A dead officer's credentials** -- the fifth item of the original list. The record's credentials
  are retained (the computer does not know a holder is dead), but *using* them is a deliberate act
  the console does not yet offer. **This is reported, not built**, and the older phrasing ("still
  usable") is withdrawn rather than kept as a promise the code cannot keep: a silent use of a dead
  officer's authority would weaken the scene the edge case is for. It is an open item, and it
  belongs with the memory and consent layer (`docs/memory-and-consequence.md`), not the clearance
  table.
