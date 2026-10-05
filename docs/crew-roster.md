# The crew: 141 people

The owner's requirement, 2026-10-05: a crew of 141, each with relationships, desires, needs, allegiances,
technical skills and traits. Canon gives the number -- 141 at launch, about 150 in service, operable with
100 -- so the roster size is fixed. What is not fixed is how to afford depth across a hundred and forty
people without writing a hundred and forty variations of *competent, loyal, haunted*.

Two decisions carry it: **depth is earned by attention**, and **static character data is content while
dynamic state is save**.

## 1. Three tiers, and one of them is a promotion pipeline

- **Tier A -- the named few (8-15).** Senior staff, department heads, and anyone a scenario puts on screen.
  Authored by hand: drives, fears, allegiances, history, voice, and hooks. These are characters.
- **Tier B -- the backbone (~40).** Named, with post, skills, watch, temperament, allegiances and a couple of
  ties. Generated from curated tables, checked for collisions, occasionally promoted to the screen.
- **Tier C -- the complement (~90).** Generated: name, species, post, skills, a quirk. They fill posts, hold
  watches, and wait.

The tiers are not a caste system, because **Tier C becomes Tier A by attrition**. When the chief engineer is
killed in a coolant bay, the roster promotes by the same rules that run during play, and whoever lands in that
post is now a character with an arc -- because the game put them there and the player has to live with it.
That is the whole emotional engine in one sentence: *the spotlight is inherited.* Canon works exactly this
way; most of Voyager's crew were nameless until an episode needed them.

## 2. The record

Identity: name, species, age, pronouns, department, post, rank, watch, quarters.

**Skills** -- seven, rated 0-5: engineering, medical, science, security, operations, command, flight. They
set repair speed and failure risk, so a skill-1 rating plus a bad roll is how a repair goes wrong
(`docs/failure-is-content.md`).

**Qualifications and credentials** -- earned, not given, and they are a source of *access*
(`docs/access-and-authority.md`). Cross-training is how a junior ends up able to run someone else's station.

**Traits** -- few, legible, behavioural rather than percentile: steady under fire, needs less sleep, good with
people, claustrophobic, first-contact trained, poor with authority. A trait should be recognisable in a log
line, never a hidden modifier.

**Drives, in three parts:**
- **Desire** -- what they want: promotion, home, a particular person, to prove something, to be left alone.
- **Need** -- what they are short of: sleep, food, company, purpose, medical care. Needs are deficits, and
  deficits degrade performance.
- **Fear** -- what they avoid: dying alone, decompression, the Borg, being useless, being seen as a coward.
  Fears are what make an order *refused* rather than carried out.

**Allegiances** -- faction ties (Starfleet, Maquis, Delta native, civilian, Borg-recovered) and personal
loyalties: who they trust, who they blame, whom they serve. This is where the Maquis split lives.

**Bonds** -- relationships as edges with a kind and a valence: mentor, friend, rival, lover, debtor, survivor
of the same thing. Stored bidirectionally, because blame is not symmetric.

**State** -- morale, fatigue, health, injuries, assimilation progress, current duty, and the flags that are
actually memories: *saw the brig sealed*, *was left behind*, *pulled off post to cover a breach*.

## 3. Why all of it is mechanics and not decoration

Every drive above is an **input to something the simulation already needs**:

- **Need** degrades performance at post -- the acknowledgement latency and error rates the crew harness
  already measures.
- **Desire** drives voluntary risk: the person who wants promotion volunteers for the dangerous job, and the
  person who wants to go home considers staying on the planet. Canon does both.
- **Fear** is the visible face of the **soft authority gate**: under stress, an order is refused, delayed or
  carried out badly depending on the person. This is what turns the hierarchy from a menu into a crew.
- **Allegiance** decides who reports what, to whom, and how late.
- **Skills and traits** decide whether the job is done well, slowly, or dangerously.

## 4. Bonds are grown, not only authored

Authored seeds for Tier A; everything else emerges from play, which is cheap and produces stories nobody
wrote:

- two people who held a breach together;
- someone who was left behind and came back;
- someone pulled off a post to cover another's absence;
- a survivor of a deck the rest of the crew wrote off;
- the person who sat with someone in sickbay, and the person nobody sat with.

Both kinds live in the same graph, and both surface in the log and in what crew say. **A bond the simulation
recorded is worth more than a paragraph of backstory**, because the player was there when it formed.

## 5. Where the data lives -- and this is the engineering point that makes it affordable

**Static character data is content; dynamic state is save.**

- The **static** record -- identity, skills, traits, drives, allegiances, authored bonds -- lives in data
  files in the repository, versioned like any other content. It can be rewritten, expanded and corrected
  without touching a single save.
- The **dynamic** record -- morale, fatigue, injuries, assimilation, current duty, emergent bonds, memory
  flags -- lives in the ship blob.

This matters because of a number we already measured. G3's save cost is **25 bytes per crew member** against
a budget of 256, and that was for posts and schedules. A full psychological record is several hundred bytes;
141 of them would be roughly seventy kilobytes of save on its own. So the budget needs restating honestly --
and keeping the static half out of the save is how it stays small and how characters remain editable.

## 6. The fabrication risk, and the check that catches it

A model asked for 141 characters will produce 141 variations of the same three people. So:

- **Author a small core by hand**, curate it, and let it be genuinely particular. Ten real characters beat a
  hundred and forty silhouettes.
- **Generate the rest from tables, with a collision check.** No two crew may share the same combination of
  desire, allegiance, trait and quirk -- enforced programmatically at build time, failing the build if they
  do.
- **Let play supply the rest.** Promotions, bonds, grudges and memories are the part no author writes, and
  they are the part that makes a roster feel inhabited.

Acceptance: given two crew members at random, a log can tell them apart by what they want and who they hold
with. If it cannot, the generator has failed and the build should say so.

## Public rosters, and what each is good for

Checked 2026-10-05. There is more public crew data than expected, and each source answers a different
question.

| source | what it is | what to take from it |
|---|---|---|
| **Memory Alpha -- "USS Voyager personnel"** | canon A-Z crew manifest with roles, notes, and casualties ("Ensign Lyndsay Ballard, engineering, 2371-2374 KIA"; "Lieutenant Joe Carey, assistant engineer, 2371-2377 KIA"; "Lieutenant Commander Cavit, XO, KIA 2371") | names, roles, departments, **who died and when** -- which is directly the material for start states and for canon-shaped casualty lists. It also notes that **all Maquis crew held provisional Starfleet ranks**, which is the field-commission precedent the multiplayer premise runs on |
| **Memory Beta -- "USS Voyager personnel roster"** and its personnel category | the licensed expanded universe: novels, comics, games | volume. Hundreds of names that sound right for a Starfleet crew, to fill Tier B and C. **Non-canon**, and must be marked as such per `docs/lore-ledger.md` |
| **Ex Astris Scientia -- "Voyager's Crew Complement"** | a fan analysis of the on-screen count and its contradictions (141 at launch, 153 cited, 152 after the Maquis, ~150 later, operable with 100) | the arithmetic, and the honest reconciliation of the numbers we built the ship model on |
| **the game's own bot roster** | ~60 named characters in the retail paks, with **models and skins** | the priority list for Tier A and B, because these already have faces, and they are ours to use |
| **fan "crew manifest" projects** | hobbyist rosters, often fan fiction | texture only. Never a source of fact |

### Two traps

1. **Memory Alpha's manifest includes production staff.** Rick Berman, Brannon Braga, Ken Biller, Jay
   Chattaway, Dick Brownfield, John Chichester and others appear as background characters named after the
   people who made the show. They are real roster entries and they are not crew we want at the conn. Read
   each line before using it.
2. **Facts, not prose.** Names, ranks, roles and dates are facts and can be used. The sentences and
   characterisations on those wikis cannot be copied into this repository -- and our own characterisation is
   better anyway, because it has to work with the drives, fears and bonds in this document.

### How this lands in the data

Each character in the crew data files carries a **provenance** field: `canon`, `licensed`, or `ours`. That
keeps the roster honest at a glance, matches the discipline in `docs/lore-ledger.md`, and means a character
drawn from a novel is never presented as something the show established.

## 7. The named core we already have

The game itself supplies named faces, which is why the reuse rule holds here too: the bot roster alone gives
us roughly thirty characters with models and skins -- Munro, Telsia, Chell, Biessman, Foster, Chang, Jurot,
Alexandria, Mackey, Oviedo, Pelletier among them -- and canon supplies the senior staff. The Tier A list
should be drawn from those first, because they already have faces, voices in the manual, and a place in the
game's own fiction.
