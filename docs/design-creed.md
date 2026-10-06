# The design creed

What this programme believes, and the incident that taught each belief. Written down because the technical
rules can be re-derived from the code, and the ethos cannot -- and because ReachLock will need it more than it
needs our patch series.

A lesson without its incident is a platitude, so each one is anchored to the moment it cost us something.

## 1. The ship is the state, not the place

The problem looked like an engine limitation: crew could not cross decks because a deck change destroys the
server's memory. It was not an engine limitation at all. **Nothing owned the ship** -- we were treating the maps
as the ship, when they are views of it. One authoritative structure, and decks become places the state is shown
in. Every hard problem after that was easy by comparison.

## 2. Failure is content

A transporter mishap, a lost shuttle, a captain relieved by their own crew, a merged crew member who is not
either of the people who went in. Canon is *made* of these, and most games treat them as punishment to be
rolled and forgotten. The rule that makes it fair: the risk is visible before you act, the consequence traces
back to what you did, and a competent path avoids it. **Rolled catastrophes feel like bugs; earned ones become
war stories.** And the one unwinnable ending -- the core breach -- must be reachable only by a chain, never by
a roll.

## 3. The world must remember

The crew who were pulled off a post, the bond that formed over a sealed hatch, the deck that was written off and
sealed, the month the estimate got worse: all of it outlives the moment that caused it. Memory is what makes
attrition mean anything, and it is cheap -- a log line, a flag on a record, a name in the wall.

## 4. Measure by what it affords, not by what looks right

The recommendation to build crew posts on deck04 looked sound -- it had the densest navigation of any deck --
and it was wrong, because its interactive objects were shooting-range props. Nobody's post was at a workstation.
The measurement that mattered was *work objects with furniture at standing distance*, and it reversed the
decision. **Count what a thing affords before you judge it by its size, its popularity, or its plausibility.**

## 5. The artifact is the evidence

Five separate times in one programme, the honest check was looking at the thing rather than trusting the tool
that made it:

- a map that compiled clean with every brush inside out, and an engine that said *map with no shaders*;
- a BSP whose lumps were empty while the compiler exited 0;
- a criterion whose evidence was missing and which the tooling therefore marked *unmeasured*, not passed;
- a specification committed to my working tree and to nowhere else, caught only because someone asked
  "is it on the machine?";
- a figure whose role and trait text overlapped, caught by opening the image.

**A green check assembled from an exit code and a guess is not verification.** Read the artifact.

## 6. Adjectives do not compile

"Moody", "busy", "lived-in", "like the show" cannot be checked, and an agent given them builds a box with a desk
in it. What transmits is a reference (copy *that* map), a measurement (standing distance, not "roomy"), a parts
list drawn from what exists, and a walk -- *you come in from the corridor, the console is on the far wall.*
**Describe the route, not the mood.**

## 7. Depth is earned by attention

A hundred and forty-one crew, and the answer was not to write a hundred and forty-one arcs. A small
hand-authored core, a large generated body with collision checks, and life supplied by play -- with the
inversion that matters: **the spotlight is inherited.** When the chief engineer dies, whoever is promoted into
that post inherits an arc, because the game put them there. Ten real characters beat a hundred and forty
silhouettes.

## 8. Subtraction is design

The list of things we decided *not* to build is the reason the rest coheres: no point defence (canon has none),
no time travel (the reset button we exist to refuse), no Borg-versus-holograms (unsupported), no flight sim
(more expensive than every system combined), and section 42 deliberately left next door rather than spent in
this room. **Every "we will not do that" is a decision that keeps the programme honest**, and it belongs in
writing where the next agent will read it.

## 9. Write it where the builders are

The design was written, pushed, and invisible: the agent building the ship was on a branch thirty-five commits
behind, in a tree with none of the documents. The fix was structural, not motivational -- a project brief at
the root, a ledger that says what is next, figures that regenerate, and a read-only checkout for anyone who
only needs to read. **Documentation that the builder cannot see does not exist.**

## 10. Say what you do not know

Confidence levels per class of claim; the honest note that a figure was cited to a page that does not carry it;
"nobody has played it yet" said to a person who had just called the work finished. The most valuable habit in
the whole programme is refusing to round uncertainty up -- and the most valuable sentence is *here is what I
could not verify.*

## And what the game is actually about

Not the ship, and not the enemy. **A group of people holding together under cost.** You are not the hero; you
hold one post in an organisation, and what you do is carry the cost forward. That is why the crew model, morale,
authority, failure and the fuel constraint all converge on the same thing -- and it is the same thing a
space-western needs: a contract, a crew, a debt, and a frontier that does not care.
