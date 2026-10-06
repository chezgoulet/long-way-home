# The record, the log and the memory: three layers, and what may lie

The owner's design of 2026-10-06, and the piece that gives the rest of the corpus its shape. It began as a
question about whether an officer's month report can be false and ended by separating three things this
programme had been treating as one.

## Three layers, and they are different kinds of thing

| | **the record** | **the log** | **the memory** |
|---|---|---|---|
| what it is | the simulation's own state | documents people wrote and signed | what a person actually knows |
| who owns it | the machine | the crew | the individual |
| presumed | true — it is the state | **possibly false**, by commission or omission | partial, and sourced |
| readable in the fiction | by nobody | by whoever is cleared to read it | by that person, and by the log |
| lifecycle | lives as long as the save | can be edited, purged, lost | lives and dies with the person |
| can it be destroyed | no | yes | only by killing the person |

The corpus already had the third layer, and `docs/memory-and-consequence.md` gives it provenance: *saw it
myself*, *was told by X*, *heard it as rumour*, *read it in the log*. A character cannot know what they were
never told. `docs/memory-boundaries.md` gives the first layer its boundary rule and `docs/gap-the-log` gives
the second its first slice. What was missing is that they are three things, not one — and that the interesting
quantity is the **gap** between them.

## Instruments cannot lie; people can

The rule that makes the rest work, and the one to cite when an argument about honesty comes up.

**Instruments are reads of state.** The navigation counter, the inventory of what still works, the
damage-control board, the panel the player is standing at. These never lie, and
`docs/navigation-counter.md`'s own rule — *the number must never lie* — is untouched. They are the simulation
read out, so they cannot drift from it.

**The log is not an instrument.** It is a document somebody wrote and somebody signed. It can be false by
commission — a struck line, a softened number, an added claim — or by omission, which is the harder and more
common kind.

So the log is testimony, the record is fact, and **the gap between them is the game.**

## The simulation writes the log and never reads it

A law, not a preference, and it is what keeps determinism intact while the log becomes unreliable:

- **The record is authoritative.** Nothing in the ship model may make a decision by consulting the log.
- **The log is write-only from the simulation's point of view.** The simulation writes it; the crew and the
  player read it; nobody rules by it.
- Otherwise a lie becomes an input to a deterministic model, saves stop reproducing, tests stop meaning
  anything, and a hallucination becomes a mechanic — the same invariant the standings rules in
  `docs/handoff-live-crew-and-meetings.md` already carry, applied to text the *crew* authored rather than text
  a model did.

**Consequence for the meeting design.** `docs/staff-meetings.md` says a brief carries "what the ship and the
people look like right now." Under this rule it cannot: a brief built from the record is a meeting where
everyone already knows the truth. **A meeting brief is built per participant, from that person's marks and
the log, and from nothing else.** See `docs/story-and-semantics.md`, conflict 5.

## The two logs

Canon has both, and the game's own station menus already ship one of them — *Personal Log* is in the
expansion's Virtual Voyager menu list (`docs/gates.md`, G2's acceptance bar).

- **The official log.** Signed and published. The month report lives here. Scoped: a post reads its own scope,
  command reads all, the player can search (`docs/gap-the-log`).
- **The personal log.** Private, and where the truth goes when it cannot go in the report.

**The toll is the distance between them.** The canon instance of the whole mechanic is Sisko's final entry in
*In the Pale Moonlight* — *I can live with it* — and it is worth noticing that canon delivered that line as a
**personal** log entry, which is as good a validation of the two-log shape as we are likely to get.

## The month report

The report is the game's period beat, and it unifies two things the corpus already had: it is the *signed*
artifact the toll is measured against, and its headline number is the navigation counter's change since the
last entry. The counter's derivative is the story; the report is where the story is said out loud.

- **Drafted by the simulation, honestly, from the record.** The player does not author it from nothing.
- **Edited by the player.** Strike a line, soften a number, add a claim. That is the lie, and it is made with
  the player's hands rather than narrated at them.
- **The record keeps the diff.** So the player can always see what they actually did, and the crew can only
  ever see the published version. Nothing enters the record that we cannot answer for; the record simply
  remembers that you deleted it.
- The diff is a **player-facing honesty instrument, not an in-fiction leak.** No board of inquiry can subpoena
  it, because there is no in-fiction mechanism to do so.

Cadence and authorship: each department head signs their own section, the captain signs the whole. That maps
onto the log's existing `scope` field and means the contradiction check below runs per scope.

## Veracity and the toll

If the reports keep coming across false relative to what the crew remember, trust and morale fall in those
under the officer who signed. Three things have to be pinned down or the mechanic collapses into a lie
detector.

**Who detects it — the marks.** A crew member notices a false report when it contradicts a mark they hold,
and **provenance decides what they can notice at all**:

| source | what it is worth against a signed report |
|---|---|
| saw it myself | proof |
| was told by X | depends on whether they believe the teller |
| heard it as rumour | unreliable, and attributable to the rumour rather than to the truth |
| read it in the log | worthless — the log is the thing being accused |

**Noticing is not being believed.** A junior contradicting a signed report is a junior contradicting a signed
report. The corpus's own soft authority gate supplies the grammar
(`docs/access-and-authority.md`): the mark goes in **privately**, trust falls, and the *public* challenge costs
the challenger. So a lie is safe precisely when its witnesses are gone, bought or compromised — which is why
*I can live with it* works, because the only person who knew was in on it. The war stories come from the times
the officer was wrong about who saw.

**Direction — the toll is paid downward.** The owner's phrasing is that trust falls *in those under the
character*, and it should be held to that, because it makes the mechanic sharp: lying up to a senior officer,
or into a report nobody below reads, costs nothing from below. What cannot be done is being false to the
people who report to you without the room noticing.

**Modulation.** Divergent allegiance and a bad history **amplify** the toll; alignment and a good history
**suppress** it. Which has a darker consequence worth keeping: your loyalists are your blind spot, and the
first person to see through you is the one who already did not trust you.

## Purge

The logs can be purged. It is a real option, with a real trigger and a real price.

**What it protects against.** Readers — everyone who needs the document. A sliced computer core hands your log
to whoever cut it (`docs/borg-incursion.md`: a hack is a leak). An inquiry, an audit, a leak, a faction that
acquires the record: all read documents. **Purge when the threat reads.**

**What it never protects against.** Assimilation. See below.

**What it costs.**

- **It orphans marks.** A crew member whose memory of an event is sourced *read it in the log* does not
  forget — they are left holding a citation that no longer exists. A purge converts a ship where anyone could
  check into a ship where only witnesses count, which is precisely the mechanism above: a lie is safe when its
  witnesses are gone, bought or compromised. A purge can eliminate one class of witness, the documentary one,
  and never the human one. That is what a cover-up is: making the ship's memory match the crew's memory by
  deleting everything else.
- **It costs the ending.** The log is the last frame of the game (`docs/endings.md`). A player who purges to
  survive is trading the ending for the run, and a run that purges often has nothing left to hand back.

**And it is a trap, not a tool.** Purge to hide a fudge and the trap is set: the log is gone, so the crew can
no longer check the report against anything — and then a crew member is assimilated, and the Collective holds
the fudge and quotes it back in that person's voice. The cover-up makes the reveal worse, the reveal is less
attributable than it would have been, and the purge is what caused it. It falls out of the rules rather than
out of an author's decision, which is the only kind of trap that belongs here.

## What assimilation takes: memory and access

Owner's ruling, 2026-10-06: when a person is assimilated, the Collective gains **the contents of that person's
personal log** — purged or not — **and their access levels to every system they could operate.**

This is the corpus's existing rule one level up. `docs/borg-incursion.md` already says that taking the
computer core means inheriting what the ship knows, *including where everybody is*, and calls it the thing
that makes the ship's memory feel dangerous. The same rule at the scale of a person.

- **The log is taken from the mind, not from the disk.** Assimilation adds a mind to the Collective; it does
  not read a file. So the purge is irrelevant here, and the reason is not a special case — it is what
  assimilation *is*. Everything else that reads the log is reading a document; the Borg are reading a person.
- **Access comes with it.** `docs/access-and-authority.md` already carries the twin of this as an edge case:
  *a dead officer's credentials still work, because the computer does not know they are dead.* The worse
  version is now true. **The computer does not know they have been assimilated**, so the ship is lost *through
  its own authority structure* rather than through a pressure timer.
- **Canon's proof is Locutus.** Assimilating Picard gave the Collective his knowledge of Starfleet's
  dispositions, and that was the entire attack. Credential and knowledge theft is the Borg's actual method,
  not a garnish on it.
- **The clean intercept is now enormous.** `docs/borg-incursion.md`'s *no dwell, no write* means nothing is
  written below the dwell threshold. So a clean intercept is no longer the difference between minor and major
  damage — it is the difference between losing a firefight and **losing the keys to the ship**. Security stops
  being the department that shoots and becomes the department that keeps the ship.
- **Command staff are worth the most**, because access is ship-wide at the top of the ladder and the captain
  knows everything the record holds. The Borg should go for the bridge, and the bridge knows it.
- **The Collective's voice.** Because the Collective holds private words in the memory of people the crew
  trusted, it can **speak in their voice, using their own words**. This costs almost nothing to implement — the
  meeting design already puts a named speaker in front of a line — and it is the coldest available use of a
  log. It is also where the toll, the log and the Borg converge into one scene.

## The only counter-play against the Borg is ignorance

Consequence of the above, and it is uncomfortable on purpose. You cannot secure a secret by writing it and
deleting it if the person who must keep it can be taken. **The only secret the Collective cannot get is one
nobody knows.**

So compartmentalisation is not one option among two — against the Borg it is the only one. Tell people less,
so that when they are taken they have less to give. Canon does exactly this with need-to-know and prefix
codes, and the price is the toll above: **an uninformed crew is a resentful crew.** One decision buys Borg
resistance and spends crew trust. The counter-play to the Borg is therefore a question about how much you
withhold from the people who serve you, which is the most Star Trek question there is and the least
comfortable.

And the social consequence comes free. If an NPC's personal log is absorbed, the Collective holds whatever
that person privately thought of the captain — and canon's Borg use what they know. So the resentful ensign
becomes a liability, everybody aboard knows it, and the ensign knows they know. The counter-play is the one
the whole game is about: **tell them the truth yourself, first, before the Borg do.**

## Boundary

`docs/memory-boundaries.md` says game-scoped data never reaches an operator store, and operator memory is
never in the retrieval scope of anything the player can reach — because the failure mode is a privacy breach,
not a design error.

**"The Collective reads your personal log" is fiction-level.** It is computed inside the module, from the
crew records this design already saves. It must never be implemented as a real memory-store read, and no
character memory may ever be wired to an agent vault. That document was written to prevent exactly this
mistake; this feature is a new way for it to happen.

## Acceptance

- The record, the log and the memory are three stores, and no rule requires the log to equal the record.
- A month report can be edited by the player, the edit is recorded as a diff, and the crew can only read the
  published version.
- A crew member notices a false report only where it contradicts a mark they hold, with the provenance table
  above deciding what counts; noticing does not by itself mean being believed.
- A lie told into a report nobody below reads produces no trust loss from below.
- A purge orphan the marks sourced *read it in the log* rather than erasing them, and the purged run shows a
  hole where the log would have been.
- An assimilated crew member transfers their marks *and* their access to the Collective, whether or not their
  log was purged; the Collective can speak their private words in their voice.
- A meeting brief is built per participant from marks and log, and opening the room calls no model and
  consults no record.
- No game-side code path can reach an operator memory store, and no character memory appears in any retrieval
  scope the player can reach.
