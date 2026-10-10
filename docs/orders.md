# Orders — one to one, and the third use of the same machine

**The owner's design, 2026-10-10**, in his words: *"Orders are similar to meetings but they are one to one.
They're similar to policy set by a particular character in their role in rank and authority, but it differs in
that policies are set within the computer UI. Orders are dispatched to a character or group of characters,
similar to meetings in that they can be scripted, should probably expose a common set of orders that would be
associated with the receiving character and role to the order giving player, and accommodate bespoke orders in
the same way that bespoke orders through a meeting would work... Deterministic via the metrics and characters
related but scripted for the spoken content by the background LLM."*

## What an order is, and what it is not

Three things sit near each other and must not be confused:

- **A meeting** is *many*, in a **place**, synchronous, and it decides something together: the room, the table,
  the argument, the consensus (`docs/staff-meetings.md`).
- **A policy** is set **in the computer UI** — a standing answer to a class of situation, not addressed to
  anyone: the triage order worst-first, the power band, the phaser setting.
- **An order** is **dispatched to a person or a group** by someone with the authority to give it. It is
  *directed*, and it **moves work**: it is the moment a decision becomes somebody's job.

## What already exists — the verbs are built; the act is not

- **The order verbs exist as hooks**: `OrderBuild`, `OrderRepairFirst`, `OrderSecurityTo`, `OrderEvacuate`,
  `OrderTriage`, `OrderAdvance` (`module/ship/ship_core.h`). Each already pushes work into the ship.
- **The work queue exists** (`docs/crew-work.md`): a job with a kind, a target, progress and priority, and
  **deferred maintenance** for an undermanned post. **An order's effect is a job.**
- **The authority exists** (`docs/access-and-authority.md`): rank, department, clearance, and `MayCommand`. **Who
  may order whom is already answerable** — an order a person may not give must be refused for that reason, in
  the same way the console refuses a control to a station that does not own it.
- **The character layer exists**: a person takes an order according to their traits, conditions, drives, morale
  and the marks they already carry (`docs/character-derivation.md`, `docs/rising-to-the-occasion.md`,
  `docs/memory-and-consequence.md`). **That is the "deterministic via the metrics and characters" half.**
- **The meeting machinery exists** — brief, script, authored skeleton, enumerated options with costs, the
  free-text pill, the novelty seam, delivery direction per line, rendered voice, the cue track, and the
  refusal for anything outside the set (`docs/staff-meetings.md`, and the evidence for M2–M6).
- **And the enumerated-API discipline exists** — one place a program can read, guarded in both directions
  (`module/ship/console_api.def`, `scripts/api-check.sh`).

## So an order is this

**A one-recipient brief.** Its enumerated options are the orders that *this* recipient can be given; choosing
one **pushes a job**; and its spoken content is scripted exactly as a meeting's is — authored skeleton with no
model present, bespoke lines from the background model, and **the delivery direction carried per line**.

- **The common set is derived per recipient, not written out**: from their **post** (which systems they hold),
  their **department**, their **rank**, and **the giving player's authority over them**. A chief engineer can
  be told to hold the plant; a security officer can be sent to a deck; a new ensign can be told to go and look.
  **This is the same discipline as the ship's API**: one place a program can read, and a check that fails when
  the surface grows and the enumeration does not.
- **A bespoke order takes the free-text path**: the novelty seam (M4) classifies it, the simulation resolves
  what may be ordered at all, and the model speaks. **An order outside the set is refused in character** —
  the same invariant the computer keeps — never invented.
- **The recipient answers as a person**: their morale, conditions and marks decide how they take it, and an
  order that costs them something may write a mark, exactly as a meeting outcome can.

## The standing rules do not change

**The simulation decides; the model speaks.** The model may never write ship state, may never become a
dependency, and a save must replay identically — the order's *text* lives in the script or the log, never
regenerated. No audio is committed.

## Open questions, for the owner rather than for a lane

1. **Does an order have a place?** A meeting's room is what buys the render window (the ruling in
   `docs/staff-meetings.md`). An order is not a scene — so is its spoken content rendered in the async window
   the same way, and is that window opened by *the order being drafted* rather than by a room?
2. **One to one, or one to many?** A group order (a department, a watch, the security squad) is a different
   act from an order to a named person — the design names both, and the enumeration differs.
3. **What does an order that is refused or resented cost?** `MEM_OVERRULED` exists for the chief engineer's
   advice; an order that a person takes badly is the same family, and where it lands is not yet decided.
4. **Does an order appear in the log as its own kind of act**, distinct from a decision and from a job?
