# Design North Star — "Year of Hell, run as a series"

Added 2026-10-04, at the project owner's direction. This document states what the program is
*for*. It supersedes the implicit assumption in the charter that the goal was inhabitation for
its own sake: inhabitation is the substrate, not the point.

---

## 1. The intent, in the owner's words

The ambition is the playstyle demonstrated by Voyager's **"Year of Hell"** — a ship ground down
over time, accumulating damage and loss, with no reset. As a fan of Ronald Moore's *Battlestar
Galactica*, the owner reads BSG as the thing Moore wanted to do with Voyager and was not
permitted to, and wants that experience implemented here, single-player or multiplayer.

The fandom consensus is the same reading, and it is well documented: Moore left Voyager over
exactly this frustration — the reset button, the unearned repair, the shuttle that is always
available — and took the ideas to BSG. **BSG is Year of Hell at series length.**

**"Galactified" is adopted as the internal design adjective** for this intent. It is not a name
and never becomes one: it references another rights holder's mark. It is a useful word in design
conversation, in this document, and nowhere public.

## 2. What that means mechanically

The enemy is not difficulty. The enemy is **the reset button**. Everything below follows from
removing it.

1. **Persistent ship state.** Damage is stored per compartment and per system — hull, power,
   propulsion, weapons, sensors, life support, atmosphere. A fight you survive can leave a deck
   uninhabitable and a system offline for the next three missions. Repair is an activity with a
   cost, not a cutscene.
2. **A resource economy that bites.** Energy, munitions, spare parts, rations, medical supplies.
   Consumption exceeds replenishment by default; the gap is closed by salvage, trade, or risk.
   Every engagement has a price.
3. **The crew as a load-bearing system.** Not ambience — mechanics. Watch rotation under
   staffing pressure, fatigue that degrades performance, injuries that do not heal on a schedule,
   and deaths that leave a post empty and a name in the log. The reason to have a living crew is
   so that losing one matters.
4. **Campaign continuity.** The ship's state carries between missions; the save is the ship, not
   the level. Failure is not a reload — it is attrition, and the game continues.
5. **A pressure director.** Something drives the clock and denies the safe harbour: pursuit,
   deadlines, a shortage arriving before the fix does. The BSG rhythm — the fleet cannot stop,
   the enemy is always behind — is a *system*, not a mission script.

## 3. Why the existing substrate fits this better than it fits inhabitation

The same released source that supports a living crew supports an attrition sim, and some of it
maps directly:

- **Structure is already tagged in the maps.** The shipped campaign maps carry 458
  `misc_model_breakable` and 473 `ref_tag` entities, plus `func_breakable` and a family of `fx_*`
  damage effect entities (sparks, steam, smoke, electrical explosions). The hook points for a
  damage model exist and are in use.
- **Crew are already individuals.** `NPCs.cfg` parameterises aggression, vigilance, aim,
  intelligence, sight and hearing per character; `say.h` gives a structured bark vocabulary
  (acknowledge, refuse, react); the named-crew entities are already placed on the decks.
- **The behaviour-state machine already has fatigue-adjacent and non-combat states** — `BS_WAIT`,
  `BS_SLEEP` (with a startle response), `BS_INVESTIGATE`, `BS_MEDIC`, `BS_TAKECOVER`,
  `BS_FORMATION`. Attrition states are extensions of these, not a new machine.
- **The navigator is waypoint-based**, so "this corridor is depressurised, reroute" is a change to
  path validity — which the navigation system can express.

## 4. What this changes in the charter

**Track C is promoted from ambience to mechanics.** "Reactive crew" (G3) remains the correct first
milestone — five to ten NPCs on one deck, no new animations, posts and acknowledgement — but it is
now step one of a crew *simulation*, not a decorative layer. The autonomy layer is the thing that
makes an empty post mean something.

**A new track is required: Track D — Ship and fleet state.** Persistent damage, systems, power and
atmosphere routing, and the resource economy. Track D is what the player is actually managing.

**The director changes meaning.** In the v3 charter the director existed to keep the ship coherent
(restaffing posts, quiet corridors). In this design it exists to keep the ship *under pressure*.

**A new gate is implied: G6 — campaign continuity**, and separately **G7 — co-op**, if co-op is
pursued. The capstone (G5) now has a sharper acceptance test than "it feels inhabited": a
multi-mission run in which damage, casualties and shortages from mission one are still present and
consequential in mission three, and in which the player never once sees the reset button.

## 5. The multiplayer question, honestly

The owner's phrase was "single or multiplayer"; they are not the same project.

- **Single-player** is the current technical path: the released singleplayer module, the native
  engine, on Linux. Everything in the charter serves it.
- **Co-op campaign** is a separate bet. It has a strong precedent: **HaZardModding's co-op mod for
  Elite Force II converted that game's entire singleplayer campaign into co-op for up to eight
  players**, built on Ritual's released source. So campaign co-op is demonstrably achievable in
  this engine lineage — on the sequel, with released code.
- Two candidate routes for EF1, both needing investigation before commitment: (a) port the campaign
  into the multiplayer game module and run it as co-op, following the EF2 precedent; or (b) give
  the singleplayer module a listen-server model, which is research rather than configuration.
- **Recommendation:** finish the single-player crisis-survival experience first, then open co-op as
  its own gated investigation (G7) with a written scoping spike. EF1's multiplayer lineage (ioEF,
  cMod, the freeware Holomatch base) is mature and already native on Linux, which may make the
  multiplayer module the better *host* for a co-op crisis mode than the singleplayer one.

## 6. Naming record

Names considered for a project whose intent is attrition, and why each was kept or rejected.

**Rejected for collision:**
- **Continuing Mission** — three established Trek fan properties already use it (the audio drama,
  the Star Trek Adventures RPG site, the Trek.fm podcast). Legal risk low; association risk certain.
- **Skeleton Crew** — *Star Wars: Skeleton Crew* premiered on Disney+ in December 2024. Same failure
  mode, adjacent fandom.
- **Damage Control** — Marvel's Damage Control is an established property.
- **Equinox**, **Year of Hell**, **Ragtag**, **Tylium** — direct references to rights holders' marks
  or invented proper nouns.

**Shortlist (unclaimed, evocative of the intent):**
- **Half Rations** — scarcity as the core loop; half-fed and half-crewed.
- **Watch and Watch** — the naval two-watch rotation: the crew system, fatigue as a mechanic.
- **The Long Year** — Year of Hell translated into plain speech, without the episode title.
- **Hull Integrity** — engineering-forward; says plainly that the ship is the health bar.
- **Keep Her Flying** — the damage-control ethos, stated as a promise.
- **Attrition** — coldest and most design-forward; names the system, not the story.

**Component names (unaffected by the choice above):** `shipwright` for the code, `drydock` for the
build pipeline, `watchbill` for the crew roster and rotation, `bioneural` for the autonomy layer,
`shakedown` for the playtest harness, `second-star` for story authoring. With this design,
`watchbill` and `bioneural` stop being decorative names and become load-bearing ones.

## 7. Risks specific to this intent

- **Balance is the hard part, not the systems.** Attrition without fairness is a grind; scarcity
  without agency is a punishment. This needs playtest iterations, which is what the playtest
  harness is for.
- **Failure states multiply.** Every persistent system is a way to be stuck. The design must define
  what happens when the ship is reduced below viability — retreat, surrender, scuttle, or a
  narrower scope of play — rather than leaving it undefined.
- **Save-state weight.** A crew simulation with per-NPC state, per-compartment damage and an
  economy is a much larger save than the original game ever produced. This must be designed with a
  budget from the start.
- **The register must not become misery.** Year of Hell works because the crew keep choosing to
  continue. The systems exist to make that choice meaningful, not to make the player quit.

---

## 8. The multiplayer model: one ship, one server

Added 2026-10-04. This **supersedes §5**. Multiplayer here is not campaign co-op — it is not the
single-player experience with eight concurrent players. In the owner's words: a massively multiplayer
experience where **one ship equals one server**, where **one whole ship has its storyline**, and where
**one whole ship succeeds or fails by its crew's ability to work together**. Very Star Trek.

### Why this fits the engine better than it looks

id Tech 3 is poor at one large persistent world and good at many small, snapshot-networked servers.
"One ship, one server" is therefore not a compromise — it is the choice of the MMO shape the engine
can actually do well. No sharding. No mega-server. Per-ship isolation. Each ship is a modest,
container-sized service. The scale of a ship's crew — single digits to low tens of players — sits
squarely inside the engine's comfort zone.

### What it requires that single-player does not

- **Persistent state**: ship condition, resources, roster, injuries, mission history and storyline
  position must survive restarts and absences.
- **Crew roles**: the player is a person with a post aboard a ship, not a free-floating avatar.
- **Server orchestration**: many ships, each an isolated instance, with lifecycle management.
- **Identity and accounts**, because a persistent community has returning people and a roster that
  must belong to someone.
- **Operations**: deployment, upgrades, backups, moderation, and uptime are part of the product.

This is why the freeware Holomatch base matters more than anything else in the multiplayer
discussion: a persistent community needs people who can join **without buying anything**.

### The crew model, settled: a canonical complement, players first, NPCs filling the deficit

Final direction from the owner: **the crew complement is a canonical number, and any deficit is filled
by NPCs.**

**Amended 2026-10-07, owner:** the ceiling is **64 played characters** — the engine's `MAX_CLIENTS` —
and NPCs fill the rest, rather than raising the client ceiling to the complement. The reasons, in order:
64 requires no engine change, where roughly 150 requires raising `MAX_CLIENTS` **and** the memory pool,
the reliable-command buffer and the snapshot limits with it — a count no shipped id Tech 3 project has
been found at; and the deficit-filling machinery is already a first-class requirement, so the design
does not change, only the number does. **The remaining risk moves rather than disappears** — onto many
NPCs and many players on one server, which is already a named question with a spike assigned to it. A
side effect worth having: with 64 players the majority of the complement is the ship's own AI, so
*which posts are held by people and which by the ship's own AI* becomes the normal condition rather
than an edge case.

This is the strongest of the positions considered, because it does away with the ghost-ship problem
without giving up what makes the multiplayer experience Star Trek: a ship is always fully crewed, and
the drama comes from which posts are held by people and which by the ship's own AI.

Design consequences, now the record:

- **The complement is hull data, not a constant.** Canon: an Intrepid-class ship carries approximately
  150; Voyager left drydock with 153 and was down to roughly 141 after the losses in "Caretaker." The
  complement belongs to the hull class, with Voyager's default set from canon, and `crew_current` is a
  first-class ship statistic that attrition reduces and recruitment restores.
- **The unit is the post, not the character.** A post exists whether or not a person holds it. A player
  who takes a post displaces the NPC holding it; when that player leaves, the NPC resumes — including
  taking back what it was doing. That hand-over is a first-class system requirement, not a fringe case.
- **Authority stays human-anchored.** NPCs may hold posts and follow standing orders; they do not command
  players. A ship with nobody aboard is under standing orders, not commanded by its computer.
- **NPC attrition counts.** If NPC crew die, the complement drops and the ship is weaker. A loss is a
  loss, whoever was wearing the uniform — which is the whole point.
- **Track C serves both experiences**, and was never decorative: single-player gets a full NPC crew,
  multiplayer gets the deficit filler.
- **The engine ceiling binds on the player side only.** "Up to 64 played characters" is the requirement;
  past that, a server refuses entry rather than degrading.
- **New performance question for the spike:** many NPCs *and* many players on one server. Server CPU with
  a full NPC crew plus a full player complement is untested territory.

### The engine's actual ceiling, measured

From the ioEF/lilium lineage we would build on:

- `#define MAX_CLIENTS 64 // absolute limit` — the current ceiling is 64 clients. Roughly 150 requires
  raising `MAX_CLIENTS` and the constants that depend on it (memory pool, reliable-command buffer,
  snapshot limits). The source is ours to change and the community has pushed past 64 with documented
  friction, but no shipped id Tech 3 project at ~150 clients has been found.
- `#define MAX_SNAPSHOT_ENTITIES 256` — a snapshot carries at most 256 entities. This binds hardest in
  the worst case: the whole crew in one space.

**The argument in this design's favour is the ship itself.** This engine culls by potentially-visible
set, and a starship interior is the best case for that culling there is: bulkheads and decks mean a
client in engineering does not receive state for crew in the shuttlebay. A hundred and fifty people
spread through a compartmentalised hull is a fundamentally easier problem than a hundred and fifty in
an open arena. The exception — everyone in one room, battle stations, a mass evacuation — needs a
designed answer rather than an assumption.

This is a **spike before a commitment**: raise the limits, measure per-client bandwidth and server CPU
as population rises, and find the real number. It can be done headlessly on the build host, without the
client port and without players.

### And the tooling becomes the storyline engine

"One whole ship has its storyline" means storylines must be **generated and orchestrated per ship** —
templates, state-driven events, and authored set pieces bound to ship condition and crew history.
That is precisely what Track B is for. The authoring pipeline stops being a content nicety and becomes
the thing that makes per-ship story possible at all.

### New questions this forces (unresolved, for the owner)

1. **Station model**: free-roaming bodies on decks, or consoles and stations? The former is what the
   single-player client already does best; the latter is cheaper but less Star Trek.
2. **Ship clock**: real time (the ship lives while you are away, BSG-style pressure) or session-based
   missions? Real time is stronger and demands more from the NPC layer.
3. **Failure**: what does it mean for a ship to fail when failure persists? Wipe, salvage, scuttle,
   decommission, or a reduced scope of play?
4. **Interaction between ships**: independent, or a fleet layer with shared events and diplomacy?

## 9. Why this retires the Elite Force II question

EF2's contribution to the conversation was campaign co-op — eight players replaying a campaign. That
is demonstrably not what is being built. The MMO's actual requirements (persistent networked server,
maintained native-Linux engine, freeware-distributable clients, released game code) all point at
EF1's multiplayer lineage: ioEF and cMod, maintained, native on Linux, and distributed alongside a
freeware base.

The structural note from §5 still stands and now cuts differently: EF1's singleplayer and multiplayer
are **separate codebases**. So the multiplayer experience is best understood as **a new game module on
the existing open engine** — taking the released singleplayer source as the vocabulary for ships,
crew, navigation and scripting, and the released multiplayer source as the pattern for client/server
state. EF2 becomes a reading list, not a base.

**Sequencing consequence:** the single-player client remains the testbed, and remains first. Crew,
damage, resources and director systems are cheapest to develop where they can be run a thousand times
without a server or a second player. The MMO is its own programme built on those systems, with its own
gates.
