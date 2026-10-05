# Location brief: <deck> — <room>

A location brief is how a bespoke space is transmitted to the agent that builds it. It exists because
**adjectives do not compile**: "moody", "busy", "lived-in" and "like the show" cannot be checked, and an
agent given them builds a box with a desk in it. Everything below can be checked, and everything below is
made of things that already exist in the game.

Copy this file to `docs/locations/<deck>-<room>.brief.md`, fill it in, and hand it over with the line:
*"Read <this file> and build it."* Two passes, always: **blockout first, detail second.**

## 1. Function, in one sentence

What happens here, who works here, and what they do at what. If this sentence cannot be written, the room
is not ready to be built.

## 2. Canon anchors

- **Deck and placement**, from `docs/research/voyager-interior-systems.md`, with the source.
- **Neighbours** — what is next door, because adjacency is most of what makes a room legible.
- **Episode evidence** for how it looks or what happens in it.
- **Reference image**, if one exists (a canon screenshot). Attach it or link it. Agents can look at
  pictures; "this arrangement" beats any amount of prose about style.
- **Contested or uncertain points**, listed explicitly, with the decision taken.

## 3. Reuse target — the biggest cost saving, and it is checkable

- **Copy this existing map**: `<map name>` — and say why it is the closest.
- **Change exactly this**: the list of what differs.
- **Keep** anything you copied, including its mistakes, unless listed above.

## 4. Parts list

Name the existing objects and materials to place, taken from the pak census. A room built from a parts
list is a room an agent can build; a room described in adjectives is a room an agent will invent.

| part | existing asset | where it is used already |
|---|---|---|
| | | |

## 5. Spatial program

- **Footprint**, in units.
- **Entries and exits** — and whether one of them is a Jefferies tube.
- **Circulation**: the route through, and what you can and cannot see from the door.
- **Verticality**: mezzanine, raised plant deck, hatch.
- **Chokepoint**, if the room is meant to matter tactically.
- **Where a person stands** — the posts, as positions, not vibes.

## 6. Lighting brief

Light by function: working areas bright, plant rooms harsh, quarters warm. Name the sources (ceiling
panels, console glow, emergency red) and whether this room has an emergency lighting state, because that is
a system the simulation can fail.

## 7. The hook — what this room does for the simulation

- **System node** it hosts (`SYS_*`), and what its failure means elsewhere.
- **Compartment row**: `controller` / `compromise` semantics for this space.
- **Job sites**: what maintenance, repair, build or reclamation work happens here.
- **Control surfaces**: what a person can operate, and who is allowed to (authority).
- **Crew posts**: how many, at which positions, on which watch.

## 8. Acceptance checks

Machine-checkable: affordances present with navigation furniture at standing distance (24-64 units); no
pathing dead ends; `.nav` bakes on first load; NPC post coverage if crew are posted here; frame time holds.
Then the one that decides: **does it look like somewhere people work, or like a box with a desk in it?**
That judgement belongs to the owner, and it is made on a walkthrough, not on a diff.

## 9. Non-goals

What not to build, and what not to invent. Always includes: no new art, no new species, no canon invented
to fill a gap. Anything cut goes here rather than being silently dropped.
