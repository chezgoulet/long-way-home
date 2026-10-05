# Morale: the resource you spend to survive

The first gap in `docs/gap-analysis.md`, and the thing that makes attrition mean anything. Defined here so it
cannot degenerate into a ship-wide bar that drains quietly until nobody reads it.

## The rule: morale is a property of people, not of the ship

There is no single morale meter. Each crew member carries three things, each independently legible, and
behaviour is what falls out of them together.

1. **Deficit** -- what they are short of: sleep, food, medical care, comfort, company. Objective and
   trackable. Canon runs on this: rationing, bunking, five months in a bunk, a deck with no air.
2. **Outlook** -- what they believe about the situation: are we getting home, is command competent, was the
   last loss worth it, is the ship sound. Subjective, and it changes when the facts change.
3. **Holdings** -- who they hold with: bonds, grudges, allegiances. A person with strong bonds endures more
   than a person alone, and a person who blames the captain is a different problem from one who is merely
   tired.

Aggregates exist for command -- a department's state, a watch's state -- but they are **derived**, never
stored as a hidden counter. That is what keeps the model honest and reviewable.

## What moves it

Every input is something the simulation already tracks, which is why this is cheap:

- **loss** -- deaths, casualties, a shuttle lost, a deck written off, a name added to the wall;
- **workload** -- hours since anyone rested, the depth of the job queue, deferred leave;
- **deficit** -- the needs above, especially sleep; canon shows what a tired crew does to a ship;
- **faith in command** -- whether the last calls look like competence or waste;
- **fear and threat** -- the Borg aboard, being stranded, a hostile region, the unknown;
- **progress** -- distance made good, a discovery, supplies gained, contact with home;
- **warmth** -- the mess hall, the kitchen, a night on the holodeck, a promotion, a service done properly.

Note the last two. **A design where morale only falls is a misery simulator**, so the positive inputs have to
be as real as the negative ones, and they are the cheapest content in the game.

## What it drives

All of it measurable, because a state that changes nothing is decoration:

- **Performance at post** -- the acknowledgement latency, error rate and work speed the crew harness already
  measures. A tired or demoralised engineer works slower and fails more often, which is how a repair becomes a
  failure event rather than a wait.
- **Compliance** -- the **soft authority gate** in `docs/access-and-authority.md`. An order given to someone
  with low outlook and low faith in command may be delayed, done badly, refused outright, or reported up the
  chain instead. This is the mechanism that makes rank a *crew* rather than a menu.
- **Voluntary risk** -- those who want something volunteer; those who have stopped caring do not, and then the
  order has to be given, which is a different scene entirely.
- **Initiative** -- high morale: people fix what they notice. Low morale: people do exactly what they were
  told, slowly.
- **Breakdown** -- exhaustion, panic, a person who cannot go back into the coolant bay, and at the extreme
  someone who stops being reliable enough to hold a post.

## How you read it without a bar

You see morale the way you see it in life: in behaviour, in what people say, and in the ship's own numbers
deteriorating before anyone admits anything. Then one console makes it legible without reducing it to a
percentage -- the personnel screen:

- per person and per department: **fit, worn, strained, at breaking point**;
- the **top contributing factor** in words -- *short on sleep*, *lost someone in the coolant bay*, *does not
  believe the course is worth the cost*, *rations*;
- and **what would help**. That last one turns morale into a decision rather than a report.

## Where rank comes in: this is the executive officer's job

Morale is what gives the middle of the hierarchy something of its own to do. Department heads manage their
people -- rotate watches, notice who is fraying, grant rest, defend a section's workload -- and the first
officer owns the crew as a body: the roster, the welfare, the services, the notifications. Canon gives the XO
exactly that portfolio, and until now our design gave them nothing uniquely theirs.

The captain should not be managing individual morale. The captain **sets the conditions**, and the officers
carry them out -- which is the delegation design in `docs/start-states.md` with a payload attached.

## Recovery, and why ritual matters

Losses must be *metabolised*, not accumulated. The funeral is not decoration: it is the mechanic that converts
a death into shared resolve instead of a growing dread, and canon does it every time it buries someone.
Alongside it: sleep, hot food, a night on the holodeck, a promotion, a good month, a discovery, a message from
home.

And the floor is uneven on purpose. People do not bottom out together, which means a ship can be held together
by the ones who are still steady -- and that is what the trait system and the bond graph are *for*. *Steady
under fire* becomes the person you lean on, by name.

## Failure states

- **Insolence and refusal** -- an order that is not carried out, and the log says why.
- **A department that stops caring** -- work slows, maintenance slips, and the failures come later.
- **Relief of command** -- the ship-level version of the player's failure, and the reason command must be
  losable. Canon has mutiny, and it has captains who were right and alone.
- **The quiet one** -- nobody refuses, and the ship simply stops doing more than it is told.

## The tension at the centre of the game

Morale is the resource you spend to survive attrition, and *how you spend it is how you treat people*. A
captain who only ever spends the crew gets a ship that cannot take another loss. A captain who invests -- rest,
truth, recognition, ritual -- keeps a crew that will take a great deal. That is the strategic core of the whole
design, expressed as one number's absence.

## Acceptance

- Morale is per person, derived from deficit, outlook and holdings, and never stored as a ship-wide scalar.
- A demoralised crew member measurably degrades their post -- latency, error rate, or refusal -- and the log
  can say which of the three components is responsible.
- The positive inputs work: a well-fed, rested, well-led crew actually recovers, measurably.
- A captain can lose the crew's confidence without losing the ship, and both are recorded.
- And the human check: a player should be able to name the three people they are worried about, from what they
  have seen, without opening a status screen.
