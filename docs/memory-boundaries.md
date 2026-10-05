# Memory boundaries: operator memory, character memory, and why they never share a store

Prompted by the obvious question: there are many agentic-memory stacks now, and ours would like the crew to
remember things. **Could one be confused for the other?** Yes -- and the confusion is what makes this worth
writing down, because the two look identical from the outside and carry opposite trust semantics.

## Two memories that look the same and are not

| | **operator memory** (the agent's, the House's) | **character memory** (the crew's) |
|---|---|---|
| what it holds | facts about the user, the project, conventions | what a person in the fiction has seen, been told, or done |
| presumed | **true**; it drives decisions | **partial**; a person can have been lied to |
| scope | one store, curated, injected into every session | per character, per run, and *seeded* -- nobody knows what they were not told |
| who may write | the operator, deliberately | the simulation, as a consequence of events |
| who may read | the agent | that character, and the log |
| lifecycle | long-lived, pruned and reviewed | lives and dies with the run, and with the person |

A vector store cannot tell these apart. It stores text, embeds it, and returns it by similarity. **The trust
semantics live entirely in the boundary we draw around the store**, not in the technology.

## The four ways they get confused, in order of how much they cost

1. **Cross-contamination: fiction writes into operations.** A player says something in character; a
   conversational NPC with a shared memory layer records "fact"; an agent reading that store later treats
   in-fiction claims as things that really happened about the project or the user. This is prompt injection
   wearing a costume.
2. **Trust inversion: operations leak into fiction.** The worse one. If an NPC's dialogue is driven by a store
   that also holds operator memory -- names, relationships, addresses, private context -- then a *player* can
   extract real House facts out of a video game by asking the right question. That is a privacy breach, not a
   design error, and it is exactly the failure the House's own limits exist to prevent.
3. **Injection: player text reaches the agent's memory.** If anything player-authored (a chat line, a name, a
   ship's log entry, a wiki page) can end up in an operator memory store, the game is an attack path into the
   agent that runs the House.
4. **Retrieval bleed and non-determinism.** One index over a mixed corpus answers a real question with
   fiction, and a stochastic recall layer cannot be saved, replayed or tested -- which breaks the whole
   evidence discipline: state we cannot reproduce is state we cannot verify.

## The rules that follow

1. **Separate stores, separate credentials, one-way flows.** Game-scoped data never reaches an operator store;
   operator memory is never in the retrieval scope of anything the player can reach. The House already has the
   mechanism -- the memory tools take a vault name -- so this is a discipline, and it can be asserted in a
   test.
2. **The fiction owns up to what it knows.** Character memory gets a **provenance**: *told by X at T*, *saw it
   myself*, *heard a rumour*, *read it in the log*. A person who was not told must not know. This is not
   flavour; it is the difference between a crew that feels real and a crew that is omniscient.
3. **Crew memory is records, not retrieval.** Bonds are edges, memories are flags with a source, morale is
   derived. All of it is explicit, inspectable, saveable and testable -- and it belongs in the save, not in a
   vector index.
4. **Embedding recall is for flavour only**, and only where the game genuinely generates dialogue at runtime:
   a **separate vault per run**, scoped to the characters present, with no path to any operator store. If the
   memory can be structured, structure it; retrieval is a fallback, not the model.
5. **The player's own knowledge is not a memory store either.** What the captain knows should be the log and
   the chart -- auditable, searchable, on screen -- not a hidden semantic space. Our design already says the
   log is memory; this is the same rule applied to the player.

## The check to write

- a test asserting that no game-side code path can write to an operator store;
- a test asserting an NPC's retrieval scope contains game data only, with no operator entries;
- a test that a character who was not present, and was not told, cannot recall an event -- provenance enforced;
- and a replay check: the same save and the same inputs produce the same crew memories.

