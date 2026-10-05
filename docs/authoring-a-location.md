# Authoring a location for Long Way Home

How to add a deck, room or area to the ship so that it is **lore-accurate, cheap to build, and useful to
the simulation**. Written for an agent starting from this file alone.

## The prime directive: reuse first

The game already contains an enormous amount of starship: 147 map sources, four retail paks, seventeen
families of character skins, and the whole Virtual Voyager deck set with eight holodeck programmes. **A
new location is almost always a re-dress of an existing one, not new construction.**

So the first act is never to open an editor. It is to find the closest existing room:

```sh
# what rooms already exist, and where?  (entity census across every shipped map source)
cat /tmp/allmaps/extracted/*.map | grep -oE '"classname" "NPC_[A-Za-z_0-9]+"' | sort | uniq -c | sort -rn
cat /tmp/allmaps/extracted/*.map | grep -oE '"classname" "func_[A-Za-z_0-9]+"' | sort | uniq -c | sort -rn
```

Then ask which existing room affords the function you need. Only author new geometry if no room can carry
it -- and then author **the smallest space that works**, because every new brush is a thing to get wrong.

## Order of operations

1. **Locate it in canon.** Check the deck list in `docs/research/voyager-interior-systems.md` (every claim
   carries its source) and the ledger of sourced numbers. If canon is silent or contested -- the second
   holodeck's deck is contested -- **decide once and record the decision** in the location's own notes.
   Never quietly pick one.
2. **Measure the candidates, by affordance.** For the role the space must play, find rooms that already
   afford it: work objects with navigation furniture at *standing distance* (24-64 units), doors,
   circulation. The rule from the crew work stands: **a space is judged by what it affords, not by its
   size.** A deck with the densest navigation and nothing to stand at is an entertainment deck.
3. **Author the difference, not the room.** Copy the nearest map source and change what makes this room
   *this* room. Record what you copied in the commit message.
4. **Compile, then verify the artifact, never the compiler.** q3map2 exits 0 on a map containing no
   geometry at all. `tools/mapgen/check-bsp.py` reads the BSP and fails when shaders, brushes or surfaces
   are empty. Wire it into whatever script builds the map.
5. **Wire it into the ship.** A deck with no lift edge is unreachable: the turbolift is a
   `target_level_change` entity carrying a `mapname`, and the graph is data. `tour/deck04` is the model to
   copy -- it declares the most destinations. A room inside a deck needs a way in and a way out (door,
   hatch, Jefferies tube), and if it is a compartment it needs an entry in the compartment table.
6. **Hook it to the model.** Every location should host something the simulation uses: a system node, a
   compartment with `controller` and `compromise`, a job site, a control surface, or crew posts. **A
   location that hosts nothing is scenery**, and scenery is the most expensive kind of content because it
   costs the same as the rest and pays nothing back.
7. **Bake the navigation.** Place `waypoint` entities; the engine writes `maps/<map>.nav` itself on first
   load. No bot-navigation compiler step is needed for single player.

## The four traps, each of which cost us real time

1. **Brush winding.** A face's three points must be ordered so the computed normal points *into* the
   brush, matching Raven's own maps. Ordered the other way every brush is inside-out and q3map2 discards
   it *without a warning*, exits 0, and emits a BSP with empty lumps. Copy the convention; do not derive it.
2. **Face texture names are relative to the `textures/` root.** Write `engineering/enggrey`, never
   `textures/engineering/enggrey` -- the compiler prepends the prefix, so a doubled path resolves to
   nothing and every face is dropped.
3. **`-fs_basepath` must point at a directory whose child is spelled exactly `baseEF`.** The installation
   spells it `BaseEF`, and on a case-sensitive filesystem that never matches: no assets resolve, exit
   code 0. Point it at our `build/` directory, which holds the correctly-named symlink.
4. **`map <name>` and `spmap <name>` are different routes.** `map` is the single-player route and is what
   the menu emits; `spmap` takes the Holomatch route and demands `maps/<name>.aas`. Use `map`.

## The quality bar: interesting, lore-accurate, usable

Accuracy is a checklist; *interest* is a composition. A location passes when it has:

- **a reason to be there** -- an affordance, a job, a console that matters;
- **a route** -- circulation, so the space has a far side and you can be surprised coming round a corner;
- **an asymmetry** -- a mezzanine, a raised section, a hatch, one wall that differs from the other three;
- **light by function** -- working areas bright, plant rooms harsh, quarters warm. Lighting is the cheapest
  way to make a room feel like somewhere rather than a box;
- **one human trace** -- a mug, a tool left down, a PADD: reuse an existing prop, place it deliberately.

And it must satisfy the owner's rejection criterion, which is the one that decides:
**unrealistic and unnatural actions for the characters in their environment.** In practice: posts clear of
geometry, facing derived from the object the post belongs to, spacing reviewed as a *set*, and no two crew
on the same idle cycle.

## Verification gates

1. `scripts/check.sh` -- the validator: entity dictionary and scripts.
2. `tools/mapgen/check-bsp.py` on every compiled map -- structure, not exit codes.
3. A headless load: `./scripts/run-engine.sh +map <map>` and confirm `CM_LoadMap`, `Munro connected`, and
   that `maps/<map>.nav` appeared.
4. The crew harness on the new space, if crew will hold posts there -- coverage, yields, stuck events.
5. The human judgement of the space. That one cannot be automated, and it is the one that closes it.

## Dispatching this work — what to hand over, what to require back

The failure mode of a dispatched build is not a crash. It is a room that looks fine and is wrong: plausible
geometry, wrong proportions, invented canon detail, a post that faces a wall. So the handover is shaped to
make wrongness *visible* rather than to make success likely.

**Hand over:** the location brief (filled in), `docs/authoring-a-location.md`, the canon anchors it cites,
and the map to copy. Nothing else is required, and the brief names everything.

**Require back, in the return itself:**

1. **A blockout before any detail** -- a top-down layout or a bare-brush pass, with the entries, the
   circulation and the post positions marked. This is reviewed and approved before detail work starts. It
   is the cheapest possible place to be wrong.
2. **The transcript, not the claim.** Log lines for the compile, the `check-bsp.py` verdict, the headless
   load (`CM_LoadMap`, `Munro connected`) and the `.nav` appearing. A statement that the map loads is not
   evidence that it loads.
3. **A source line for every canon claim**, or an explicit "invented, because canon is silent". The repo
   has the sourced deck list; if the agent reaches past it to model memory, the invented detail must be
   labelled as such.
4. **What it copied** -- the existing map it re-dressed, named in the commit message.

**Then three spot-checks, which are cheap and catch most of it:**

- **affordances at standing distance** -- run the post generator against the new map; anything reported
  "without standing room" is a post that will read as unnatural;
- **no dead ends** -- walk the plan from the blockout, or load it and try to leave every room;
- **the `.nav` exists** -- if the engine did not bake navigation, no crew can work there and the location is
  scenery regardless of how good it looks.

**And one thing that cannot be delegated:** a walkthrough by the owner. The criterion that decides is
whether the space reads as somewhere a person works, and no transcript can answer it.

## What not to do

- Do not commit game assets or third-party images into this repository.
- Do not invent canon. Contested placements are decided and recorded, not assumed.
- Do not build a deck before its lift edge and its system hook exist -- an unreachable, pointless space is
  a liability that looks like progress.
- Do not judge a room by its entity count. Judge it by what someone can do in it.
