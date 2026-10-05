# Proposal: a Procedural Starship Workshop

The question: what would it take to build a tool for creating starships — exteriors, interiors, systems,
upgrades — at the same scope as the Procedural Pixel Creature Workshop? This is the answer, and the answer
starts with a warning: **the exterior is a shape problem, and the interior is a research problem.** Most of the
work is in the second one.

## Why it is worth doing, and why it is not a fourth side project

A ship generator that emits **both geometry and a simulation-readable ship definition** is the missing upstream
of two projects that already exist. Long Way Home hand-authored its ship model, its compartments, its decks and
its posts; ReachLock wants a seed protocol that generates coherent content. Both need the same artifact: a ship
definition that a simulation can read and a level a player can walk.

So the reusable core is not the meshes. It is the **intermediate representation**: decks, compartments with
function tags, systems on a graph, posts, and the wiring between them. Get that right and the geometry, the deck
plans and the playable level are three exports of one thing.

## What is reusable from the creature workshop, and what is not

Reusable, nearly unchanged:

- **the architecture**: a deterministic Rust core, exposed as a GDExtension, with a Godot app on top;
- **seed determinism**, including the exact-RNG-stream trick that makes a seed reproduce a result bit-exact;
- **the shape-block pipeline**: gene schema → archetype → blocks → assembly with soft-union fillets. Hulls are
  this pattern with saucers, hulls, pylons and nacelles instead of torsos, limbs and heads;
- **the verification discipline**: `self_test.sh` running build, unit tests, headless probes and an app
  self-test, exiting non-zero on any failure; golden tests; per-family quality reports;
- **release plumbing**: export templates, the Linux export, Flatpak, `check_release.sh`. Copy, do not re-derive;
- **UI patterns**: an overview grid of generated results, a studio for editing parameters, a test area where you
  look at the thing moving.

Not reusable, and this is the project:

- **volumetric functional layout.** A creature is a surface with an animation; a ship is a *volume with jobs in
  it*. Decks must fit inside the hull, compartments must be adjacent by function, corridors must reach
  everything, and a human has to walk it and find it sensible;
- **scale range.** A shuttle and a seven-hundred-metre capital ship are the same generator, three orders of
  magnitude apart, and the interiors have to work at both ends;
- **the coupling to gameplay**, which is what makes it interesting and what makes it expensive.

## The pipeline, domain by domain

1. **Class genes → hull.** Class, era, faction, role, crew complement, length. The *design language* is the
   archetype layer: a freighter, a Starfleet explorer and a Klingon warship are three grammar profiles, in the
   same way the creature tool has nine body plans. This is where visual quality lives, and where the tool either
   produces ships people recognise or produces boxes.
2. **Hull → decks.** Slice the hull volume into deck plates. Validate: every deck lies inside the hull; deck
   count matches the complement and volume; there is vertical access between decks.
3. **Decks → compartment graph.** The hard part. Assign functions — bridge, engineering, quarters, cargo,
   sickbay, labs, mess, corridors, turbolifts — under constraints: engineering near the power plant, the bridge
   high and central, quarters where the hull is widest, cargo with an exterior door. Then route circulation and
   validate that every compartment is reachable, that egress exists on every deck, and that no space is dead.
4. **Systems layout.** Place the reactor, EPS runs, life support, sensors, weapons and stores on a graph, and
   close the budgets: power generated ≥ consumed, volume ≥ systems, crew ≥ posts required. This is the same
   bookkeeping the simulation will do, so it should be the *same code* reading the same schema.
5. **Interior detail.** Affordances, furniture, doors, force fields, lighting, signage. The rule from Long Way
   Home transfers directly: **count what a space affords** — work objects with navigation furniture at standing
   distance, checked programmatically, not eyeballed.
6. **Upgrades and refits.** Variants as modifications to a hull: weapon mounts, science packages, cargo
   conversions, armour. With constraints and a budget, so a refit is a *trade*. Lineage — this hull, with these
   refits, over time — is what makes the tool useful for a campaign rather than a single ship.
7. **Exports**: the ship definition (JSON), deck plans (SVG and PNG), meshes (glTF), 8-direction sprite renders
   for pixel games, and a **playable level** into an engine we already have a pipeline for.

## The milestones, in the creature tool's shape

Gated, each with a transcript, and no time estimates:

- **M0** toolchain, workspace, one primitive exported end to end — proves the seam.
- **M1** hull grammar: blocks, assembly, soft-union, one class rendered in eight directions.
- **M2** design languages: three or more grammar profiles with silhouette gates.
- **M3** volume to decks: slicing, fit validation, vertical access.
- **M4** compartment graph: function assignment, circulation, the reachability and egress gates.
- **M5** systems layout and the closing budgets; emit the systems table.
- **M6** interior detail and the affordance check.
- **M7** exports: definition, deck plans, meshes, and one playable level.
- **M8** the workshop app: overview grid, ship studio, deck viewer, test area.
- **M9** refits, variants and lineage.
- **M10** quality gates, per-class quality report, generation benchmarks.
- **M11** release plumbing, copied from the creature workshop.
- **M12+** polish: fillets, family refinement, flagship.

## The honest risks

- **Interiors are most of the work** — call it sixty per cent or more. Exteriors alone would be a solved shape
  problem and a much smaller tool; the interior generator is where the research is.
- **Two products in one.** A generator is batch and procedural; an editor is manual and iterative. The creature
  tool is mostly generator with a light editor. Ships invite more hand design, which means more UI. Scope
  decision to make early: **generator first, editor second.**
- **The intermediate representation is a long-lived commitment.** It is the export contract for every consumer,
  including Long Way Home and ReachLock. Design it against the ship model already written, and version it —
  changing it later means regenerating everything.
- **Plausibility is the quality bar, and it is checkable.** Volume against decks, power against load, crew
  against cabin space, circulation against reachability. These are invariants, which means they are testable,
  which means the quality gate is honest rather than a vibe.
- **Recognition is the aesthetic bar.** A bad creature is a blob and forgivable; a bad ship is obvious. The
  archetype grammar is the whole defence.

## The smallest version that is worth building

Strip it to what proves the thesis: **one class, three design languages, decks and compartments generated with
the reachability gate, a systems table that closes its budget, deck plans and one playable level exported.**
That is a tool which hands Long Way Home its next five decks and hands ReachLock a deterministic ship seed —
without a single hand-authored room. Everything after that is polish, and polish is where a project like this
goes to die if it starts there.
