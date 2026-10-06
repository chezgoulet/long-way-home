# Full-scope mandate

You have the whole programme, and the owner has approved the full scope. This is the order to finish it in,
what "done" means, and the two problems that will not fall to ordinary work.

## Read first, in this order

1. `AGENTS.md` — the project brief, the rules, the commands.
2. `docs/gates.md` — the ledger: what is closed, what is measured, what is next, and the owner-approved order.
3. `docs/ship-programme.md` — the S-gates and their exit evidence.
4. **Every `docs/evidence/*.md` you are continuing.** These are the most important documents in the repository,
   because each one ends with a section titled *what this still needs*. That section **is** your work list.
5. The design corpus, by area: `docs/ship-model.md`, `docs/ship-systems.md`, `docs/damage-and-budgets.md`,
   `docs/exploration-and-science.md`, `docs/failure-is-content.md`, `docs/scenario-atlas.md`,
   `docs/start-states.md`, `docs/access-and-authority.md`, `docs/borg-incursion.md`,
   `docs/crew-roster.md`, `docs/crew-manifest.md`, `docs/morale.md`, `docs/character-attributes.md`,
   `docs/memory-and-consequence.md`, `docs/memory-boundaries.md`, `docs/crew-work.md`,
   `docs/navigation-counter.md`, `docs/outside-the-ship.md`, `docs/shuttles.md`,
   `docs/ship-master-map.md`, `docs/authoring-a-location.md`, `docs/location-brief-template.md`.
6. `docs/design-creed.md` — the ethos. Read it last and hold yourself to it; it is not decoration.
7. `docs/lore-ledger.md` and `docs/confidence-and-verification.md` — every number sourced or marked invented,
   and what to trust.

Do not re-derive any of it. If a design document and this brief disagree, the design document wins, and you say
so in your report.

## The rule of done

**"First slice" is a state of being started, not a state of being done.** The phrase appears in the evidence
documents as an honest label for work in progress. It is not an acceptable completion state for any gate.

A gate is done when its **own stated exit evidence exists**: the transcript, the measurement, the scenario run,
the table of stations → systems → controls. Not when the code compiles, not when the test passes in isolation,
and not when the mechanic works in the simplest case. If a document of yours says "what this still needs", the
gate is not closed until that list is empty or the remaining items are explicitly moved to the ledger as their
own entries with reasons.

## The order

### 1. Finish what is in flight, to the criteria each one named

S3–S10 and the five approved gaps each have an evidence document with a *what this still needs* section. Work
those lists to empty, in gate order — the small named items first, because they are cheap and they make the
next gate's measurement honest:

- **S4** — each station's real purpose: tactical targeting and firing, conn setting a course, the transporter
  with a target and a recipient, sickbay with triage, astrometrics with the survey. The replicator panel still
  opens its retail screen. The live panel-surface placement needs the authoring fix.
- **S6** — damage must be **visible in the world**: someone walking to a damaged system, something visibly
  broken. Hull repair must cost crew and parts rather than being a bare call. More injury causes. **Fatigue
  must have an effect.** Triage, supplies, rations, salvage.
- **S7** — the ship's own security fighting as bodies, not a rate, when the player is there to see it. More
  than one kind of boarder. The countermeasures puzzle must be **played by hand** before it is called done.
- **S8** — the two hard problems, below.
- **S9** — an opponent with systems of its own rather than three numbers; something to see and hear; choices at
  a beacon; **the pursuit pressure that stops the ship sitting still to repair**; more sectors; an end to reach.
- **S10** — orders and character creation reached from a panel in the world; the player's character being the
  body the player walks in; a wall-clock absence actually spanning a real absence.
- **The five gaps** — each to its full criteria, not the slice: the air countdown at the environmental-control
  panel itself, not only on Operations; the log attributing events to people rather than to subsystems, and
  searchable; the rest likewise.

### 1b. The community harvests -- two of them, in this order

Found and verified on 2026-10-05. Full detail, including the verbatim licence and the class diff, is in
`docs/community-inheritance-audit.md`.

**The licence rule, which splits the opportunity.** RPG-X's legal notice permits **code** reuse provided
UberGames is credited and it does not conflict with Raven's game-source terms -- and their game module carries
Raven's own `STEF Game Source License.doc` in `game`, `cgame` and `ui`, the same licence our module rides.
Their **assets are the opposite**: models, textures, sounds and maps may not be used without explicit permission
from their creators. So: **adopt code, never assets.** Credit UberGames in `NOTICE` and in the file headers of
anything ported. The embedded Lua layer is MIT; read it as prior art, **do not adopt it** -- we are committed to
ICARUS and a second scripting story would fork the module.

#### Harvest A -- Elite Reinforce's fixes (small, do it first)

A third independent pass over the same single-player source, focused on bugfixes. Four fixes are verified absent
from our tree -- no reference to borg1, forge3 or a holodeck save restriction exists in our module or upstream,
and none of our fourteen patches covers them:

1. **The borg1 freeze**, with a cvar to disable the fix. **borg1 is the map our own mission evidence plays on.**
   Confirm whether we are exposed before assuming we are not.
2. **A forge3 infinite loop** -- they needed two attempts, so read both commits rather than the first.
3. **An occasional menu softlock** -- and we are actively working in the menus.
4. **A command allowing saves on holodeck maps** -- directly relevant to the Virtual Voyager and holodeck save
   work already in flight.

Plus two authoring tools worth having: **showing NPC paths**, which we would otherwise build for the crew and
post work, and **highlighting entities with death scripts**.

**Method:** a three-way diff between their tree, our upstream and our module. Port as patches in our series, one
concern each, cvar-gated where they offer one. Then prove it: the existing G1 mission evidence must still pass,
and the holodeck-save command needs its own test that a save inside a holodeck map round-trips.

#### Harvest B -- RPG-X's entity classes (the cheap path to functional controls)

Measured: RPG-X defines 115 classes, our retail dictionary defines 214, and **51 exist only in RPG-X** -- and
they are almost exactly the functional-controls gap and the holodeck's uses, already first-sliced. The ones that
matter most:

- **ship systems as placeable entities:** `target_turbolift`, `target_levelchange`, `target_warp`,
  `target_gravity`, `target_shiphealth`, `target_selfdestruct`, `target_repair`, `target_doorlock`,
  `target_holodeck`, `target_teleporter`, `target_zone`, `target_objective`, `target_evosuit`, `target_shake`;
- **`target_shaderremap`** -- runtime shader switching, and a candidate mechanism for the Borg *visibly* taking
  the ship, which `docs/evidence/s8-the-borg.md` lists as an engine capability that does not exist. If it needs
  engine support, it is an engine extension and falls under the policy below, cvar-gated and off by default;
- **`ui_msd`** -- a master systems display as an entity, where we were about to build panel surfaces by hand;
- **`func_forcefield`, `func_mover`, `func_targetmover`, `func_door_rotating`, `func_lightchange`** -- the moving
  parts a ship needs;
- **`trigger_radiation`** -- a damage source our injury model lacks, and `trigger_transporter`.

**Method:** port the class definitions **and** the game-code implementations, with a provenance note on each
recording that it came from RPG-X and under what terms. Add their entity dictionary entries to our validator's
dictionary so the validator covers them, and add a map fixture that exercises every adopted class -- the same
"check what it affords" discipline we apply to spaces. Adding classes does not change retail maps, which do not
use them, so the retail compatibility promise holds by construction; anything that changes *behaviour* rather
than adding a class is an engine extension and follows the policy below.

### 2. The remaining approved gap backlog, in the ledger's order

In `docs/gates.md` under *Then, in rough order*. The first item is the **tractor beam and salvage**, because
salvage is the door to the materials economy, and the repair and build work already implemented has nowhere to
draw from until it exists. Then structural integrity, the navigational deflector, inertial dampers, pylons and
the EMH's emitter; airponics and the galley; living conditions and relationships; grief; discipline and justice;
the Maquis split; training and credentials; the holodeck's uses; resource acquisition; trade and population
pressure; the player's own body; the player's career; Borg strategic awareness; and a persistent pursuer.
Time travel stays refused.

### 3. The ship's physical content

Build the decks in `docs/ship-master-map.md`'s order — **12, 13, 10, 8, 4, 6, 14, 7**, then the amenity items —
each one by the procedure in `docs/authoring-a-location.md`: reuse first, a named copy target, a parts list, a
location brief from `docs/location-brief-template.md`, the blockout approved before detail, and the affordance
check run rather than eyeballed. `docs/locations/deck12-environmental-control.brief.md` is written and waiting.

### 4. The outside loop, then the player

S9 in full, then the player's body and career: wounds, incapacitation, assimilation and death as real states;
qualification, trust and promotion as the access design implies.

## The two problems that are not ordinary work

Both are named in the evidence documents and both are engine capabilities that do not exist yet. Treat each as
its own gate with its own evidence, and read `docs/engine-extension-policy.md` before touching the engine:

1. **Runtime asset replacement.** The Borg must visibly take the ship: a section's textures and models turning
   assimiliated at run time, and stripped back when reclaimed. The state number already exists; the rendering
   does not.
2. **The whole-ship map at frame time.** S3 measured what one map costs: *it runs; it is not yet fast*. Getting
   it fast may need engine work, and that is now permitted — under the rules: cvar-gated, **off by default**,
   retail behaviour and saves intact, one concern per patch, and the patch series rebasable.

If either turns out to need something we cannot reach, **say so with the measurement and propose the nearest
thing that works.** A blocker reported honestly is worth more than a feature claimed.

## What not to do

- Do not rewrite the design documents. They are the contract; if one is wrong, say so and change it deliberately.
- Do not invent canon. Where it is silent or contested, decide once, record the decision, and mark it as ours.
- Do not add an attribute block, and do not add species bonus tables. Both are refused, with reasons, in
  `docs/character-attributes.md`.
- Do not reintroduce reset mechanics: no timeline resets, no free repairs, no respawns.
- Do not touch mode 2, which is cMod as shipped; do not let an extension change retail behaviour with the cvars
  unset; do not commit to `main`.
- Do not commit game or third-party assets. Extracted data is regenerated by script.
- Do not mark a gate closed on a green build. Evidence or it is not done.

## What you cannot close

Three items need the owner and must be reported as awaiting him rather than closed:
**G3's judgement** — whether a deck with crew on it feels inhabited; **S10's playthrough**; and **G7's
two-machine LAN match** to close the control fault. Leave them in the ledger as awaiting, with your part done.

## Where it lands

Every gate: a transcript in `docs/evidence/`, a ledger entry updated with the measurement, and the design
document updated where implementation found it wrong. When the work is complete, open a PR from
`feature/g3-reactive-crew` to `testing` — never a push to `main`.
