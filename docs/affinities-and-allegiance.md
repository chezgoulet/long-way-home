# Affinities and allegiance: taste as the engine of dynamic relationships

The owner's suggestion, 2026-10-06. Canon is unusually rich here — Star Trek characters are defined as much by
what they make, play and love as by their posts — and the crew model has a real gap this fills.

## The gap

Relationships currently form only from events: a mark carries a valence, and a bond is the sum of
`valence × salience` over marks naming a person. That works, and it has one weakness. **Two crew members at the
same event come away with the same feeling**, because nothing distinguishes them except what they have
previously experienced. A crew of 141 is therefore one person with 141 histories.

Likes, dislikes and allegiances are the missing **priors**: what makes the same event land differently in
different people, and what makes a person *seek* one thing and avoid another before any event happens at all.

## The design rule, and why it is cheap

**Affinities are modifiers on valence and on choice — never a second relationship store.** The bond fold stays
exactly as it is; taste enters at two points only:

1. **Valence.** `effective_valence = base_valence adjusted by like/dislike strength for whatever the event
   touched.` An event that mentions Klingon opera lifts one crew member's bond and lowers another's.
2. **Choice.** What they seek in downtime, and what they volunteer for. The liked program is the one the
   schedule picks; the loved task is the one they ask for from the job queue. This is what turns preference into
   *observable* behaviour rather than a hidden modifier.

The moment preference becomes its own table of relationships, the model forks and the content cost doubles. One
mechanism, one store, heterogeneous outcomes.

## The record

Per crew member, static content — generated for the 141 from the roster seed, authored for the named few:

- **`likes[]` / `dislikes[]`** — a small number of *named* things: a food, a program, a music, a game, a craft,
  a kind of person. Each with a strength. **A few strong preferences beat many weak ones**, both because it is
  cheaper to author and because it is more human.
- **`pursuits[]`** — what they *make*: a holonovel, a meal, a model, a poem, a piece of music. Pursuits matter
  more than likes, because they generate **artifacts and events**, which is the fuel the whole memory system
  runs on.
- **`allegiance`** — Starfleet, Maquis, Delta native, civilian, Borg-recovered; with a strength, and with what it
  actually *does*: whom they confide in, whose orders they bend, and which decisions cost them morale.

## Canon material, verified against Memory Alpha

Confirmed, and each one is a hook into a system we have already built:

- **Harry Kim** — loved music and played clarinet in the Juilliard Youth Symphony; he forgot his clarinet when
  he joined *Voyager* and **later replicated a new one**. That last clause is the whole design in one line: a
  preference that *costs replicator rations*, tying taste to the economy.
- **Kathryn Janeway** — coffee, and two specific holodeck lives: a **Gothic holonovel** in the first year, and
  later **Leonardo da Vinci's workshop**, casting herself as his apprentice.
- **Tom Paris** — *The Adventures of Captain Proton*, explicitly one of his favourite programs.
- **Tuvok** — **kal-toh**, lessons from a master from the age of five, and meditation candles; and a teenage
  rebellion against Vulcan logic, which is a trait worth having.
- **Neelix** — an enthusiastic cook who ran the mess hall and experimented, adapting the crew's own recipes.
- **The Doctor** — an author: *Photons Be Free* (with the canonical arbitration over its release) and *Love in
  the Time of Holograms*. A crew member who **creates** content for the holodeck.

**Two I could not verify in this pass, and will not assert:** Seven of Nine's music studies (the phrase does not
appear in the article extract I checked) and Chakotay's boxing (the boxing reference is to a hallucination
sequence and his grandfather's condition, not to a stated hobby). Check both before authoring them.

## Allegiance is where the drama is

**The Maquis/Starfleet split is the canon engine**, and the record already proves it can be modelled: Torres is
described as "a former Starfleet Academy dropout-turned-Maquis".

An allegiance is **not a team**. It is a weighting on two things:

- **How an event lands.** A decision that reads as Starfleet procedure lands negative on Maquis-weighted crew and
  positive on Starfleet-weighted crew; a decision that reads as pragmatic and Maquis-flavoured, the reverse.
  Identical order, opposite marks, and the bonds across the line shift because of it — the model already sums
  negative valence into tension, so the machinery is in place.
- **Whether the order is carried out, and how.** This is the compliance half of the two-lock design: someone
  with a strong Maquis allegiance does not refuse to exist; they bend the order, or carry it out and remember.
  The cost is a mark, and marks are what morale reads.

## Arbitration is untouched

Taste changes what a crew member chooses **when nothing above them applies**, and how they feel about what
happened. It never touches the precedence: a hobby is not a reason to ignore a scripted sequence, a hostile, or
the director. Preference operates at `LEVEL_IDLE` choosing a `LEVEL_DUTY` post, and in the valence of the marks
that follow.

## What it buys, using hooks that already exist

- the **holodeck** — preferred programs, and a crew member who *authors* content;
- the **mess hall and rations** — a liked dish, a replicated clarinet, a disliked one;
- **downtime** — they seek what they like, which is visible on the deck;
- the **job queue** — volunteering for the loved task;
- the **log** — attributable reactions: *she has not touched her coffee since*;
- **morale** — a decision landing on the wrong side of an allegiance, with the mark to say why.

## What not to do

- **No flat skill or stat modifiers.** Taste is not competence; the species-bonus refusal applies here too.
- **No long lists of weak preferences.** Three strong ones beat twenty faint ones.
- **No separate relationship table.** It forks the bond fold and doubles the content.
- **Never let preference outrank the arbiter.**

## Acceptance

- The same event gives two crew opposite valence, attributable to a declared affinity, and the bond fold reflects
  it.
- A Maquis-weighted crew member's morale drops on a Starfleet-protocol decision, and the mark records why.
- A crew member's liked program appears in their downtime and can be observed on the deck.
- A pursuit produces an artifact or event that the log can point at.
- A personality difference is **visible in behaviour** — someone volunteering, someone avoiding, someone nursing
  a grudge about the coffee — rather than existing only as a number.
