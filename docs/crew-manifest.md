# The crew manifest: rank, post, department, watch, and bills

Who does what, who reports to whom, and what a shift actually is. Built on canon where canon speaks, and
marked as ours where it does not.

## Three axes, kept apart

Most designs collapse these into one ladder, and then a hologram cannot be chief medical officer, a civilian
cannot be morale officer, and a Maquis field commission cannot exist. Keeping them orthogonal is what makes
canon's odd cases work:

- **Rank** -- *authority*: what you may order, and to whom (`docs/access-and-authority.md`).
- **Post** -- *function and access*: what you operate, and what the console will let you touch. A post needs
  a qualification; a rank does not qualify anyone for anything.
- **Department** -- *scope*: the section you belong to, which bounds both of the above.

Rank is held by a person; a post is held by a person *at a place*; a department owns spaces and systems. The
crew record carries all three, and that is why the Doctor is a **post without a rank** -- he is chief medical
officer and the ship must obey him in sickbay, and he holds no Starfleet commission at all. Neelix is a
civilian with a post. Seven is a consultant. A field-commissioned Maquis officer holds a *provisional* rank,
which is a rank with a flag on it and a crew that remembers why.

## The ladder, and what each rung is for

| rank | what it is for in the game | typical post |
|---|---|---|
| Captain | decides; ship-wide access; accountable | commanding officer |
| Commander | sequences, owns the crew as a body; ship-wide access | first officer |
| Lieutenant Commander | owns a department and its people | chief engineer, CMO, chief of security, science officer, ops manager |
| Lieutenant | leads a division or a watch | division head, senior watch officer |
| Lieutenant (jg) | leads a shift or a team | shift lead, specialist |
| Ensign | holds a post; gets sent into the breach | junior officer of a division |
| Chief Petty Officer | **knows the machinery**; trains the ensigns who outrank them | senior technical lead |
| Petty Officer | the technical backbone | systems operator, team second |
| Crewman | the hands | general duties, assistants |

The chief petty officer rung matters more than it looks: it is where the ship's real competence lives, and it
is the natural source of the drama where the person who knows the job is outranked by the person who ordered
it. Canon does this constantly.

## Departments, and a size that adds to 141

Canon gives the complement and gives no breakdown, so **these numbers are ours** -- a proposal, to be tuned
against the ship's actual needs.

| department | crew | holds |
|---|---|---|
| Command | 9 | CO, XO; the watch-officer pool; the chief of the boat; two admin |
| Operations | 16 | ops, conn, comms, transporters, sensor watch, replicator systems |
| Engineering | 36 | warp core, EPS, hull and structure, environmental, damage control, fabricator, computer |
| Security and tactical | 18 | armed response, the brig, the armoury, tactical systems |
| Science | 15 | astrometrics, stellar cartography, labs, sensors, data and archives |
| Medical | 12 | sickbay, medics, the surgical bay, the Doctor |
| Flight and shuttlebay | 10 | pilots, shuttle maintenance, flight-deck crew |
| Support and logistics | 19 | cargo and quartermaster, galley and mess, airponics, quarters, recreation |
| Civilians and attached | 6 | morale officer, botanical aide, consultant, guest berths |

Engineering is the largest department because it is the one the ship cannot survive losing, and Support is
second because a crew that is not fed, rested and clean stops working -- which is the morale model in
`docs/morale.md` with a payroll attached.

## Watch, and what a "full shift" is

Three watches, eight hours each: **Alpha** (0800-1600), **Beta** (1600-2400), **Gamma** (0000-0800). Canon
uses exactly this alpha/beta/gamma convention and calls the night one the night shift.

A third of the crew is on duty at any moment, but **not a third of the posts are staffed**:

- **Watch-standers** -- departments that run around the clock: the bridge, engineering, operations, security,
  medical. Their watches are *manned* watches: a full watch bill, three deep.
- **Day specialists** -- science labs, admin, most maintenance and fabrication, quartermaster, airponics.
  Effectively Alpha-heavy, with a thin Beta presence and a stand-by by call in Gamma.

So the 0300 watch is a fraction of a fraction: a watch officer on the bridge, a conn, an ops, a tactical, the
night engineer on rounds, one medic, one patrol, one transporter room. **That thinness is the design's cheapest
source of tension**, and it is why waking someone off watch costs sleep, morale and tomorrow's performance.

### What a full Alpha watch looks like

| station | Alpha | Beta | Gamma |
|---|---|---|---|
| bridge watch officer (officer of the deck) | lieutenant or above | lieutenant (jg) or above | ensign or above |
| conn | officer | officer | officer, often junior |
| operations | officer plus one | officer | one |
| tactical | officer plus one | officer | one |
| science or comms | officer | one | by call |
| duty engineer | CPO plus party | CPO plus party | one CPO plus a roving hand |
| damage control party | 4 on call | 3 on call | 2 on call |
| security | patrol plus brig watch | same | one patrol, brig watch |
| medical | medic plus the Doctor on standby | medic | one medic, the EMH for anything worse |

## The bills: the answer to "who does what"

A crew member has **one post and a set of bill assignments**. The bills are what reconfigure a hundred and
forty-one people into whatever the situation needs -- and they are where the manifest becomes gameplay.

- **Watch bill** -- the daily roster: who stands which watch, at which station, on which day.
- **Battle stations** -- canon calls Voyager's general quarters **code-blue stations**, and almost nobody's
  battle station is their normal post. The kitchen goes to casualty clearing; the labs go to damage control;
  the Doctor's staff triage; the shuttlebay arms for boarding. The watch bill and the battle bill are
  different documents about the same people, and the gap between them is where the interesting problems are.
- **Emergency bills** -- fire, decompression, abandon ship, intruder alert. Each names who reports where, in
  what order, and who takes charge if the first name is dead.
- **Damage control** -- the emergency repair organisation, which pulls from every department and reports to
  the engineer of the watch, not to the senior officer present.

**The holes are the game.** A crew of 141 on a ship with these many stations has people who are the only
qualified name on two critical bills, and the manifest should make that visible: *this person is the only
transporter chief aboard* -- canon says exactly that, because the first transporter chief was killed in the
crossing.

## The Hazard Team: the unit the manifest is missing

Elite Force's own invention, and the game's lore hands us a roster that is already most of what Tier A needs. It
is not a department -- it is a **standing unit that cuts across departments**, and that is precisely why it
belongs in this document.

### What canon says

- The team was formed under Captain Janeway after the crew took losses to boarders, and it is divided into
  **A-squad and B-squad**. In the game's own lore its achievements are defeating the **Vohrsoth** and getting
  Voyager home through the **transwarp hub** -- so the unit's story arc *is* the journey's endgame.
- **Lieutenant Les Foster** commands it, and he and **Tuvok** train it in tactical procedure and lead it
  personally into action. So its reporting line is through security and tactical, while its tasking comes from
  the captain and the first officer.
- **A-squad**, with roles and personalities already written:
  - **Lt. Les Foster** -- leader, trainer, goes in first.
  - **Ens. Alexander Munro** -- second in command; headstrong, apparently undisciplined, repeatedly proves
    himself a better leader than anyone expected. *In the first game the player could be a female version of
    the same character, Alexandria Munro* -- a game canon choice we should keep, marked as our branch.
  - **Crewman Telsia Murphy** -- scout and sniper; **a longtime friend of Munro** (canon supplies the bond,
    we do not have to invent it).
  - **Crewman Austin Chang** -- demolitionist; joined when the team was formed.
  - **Crewman Kendrick Biessman** -- heavy weapons; sarcastic, boisterous, cocky, loud, and very good at it.
  - **Crewman Chell** -- technician; joined to get close to alien technology, averse to fighting, complains
    constantly, and is indispensable anyway.
  - **Crewman Juliet Jurot** -- field medic; fiercely logical with Betazoid empathy, and paradoxical with it.
- **B-squad**: Ens. Elizabeth Laird, and Crewmen Jeffrey Nelson, Perfecto Oviedo, Kenn Lathrop, Thomas Odell,
  Mitch Csatlos and Michael Jaworski.

Every one of those has a model in the retail paks, which is the reuse rule satisfied: we did not have to
invent a single one of them.

### Why it belongs in the manifest, and what it does for the design

1. **It is the away-team bill made permanent.** Everyone else's battle station is a place aboard; the Hazard
   Team's is *wherever the ship is not*. That is the standing assignment that overrides the watch for its
   members, and it is why the manifest needs a unit layer above departments.
2. **It explains the player's double life.** The player holds a shipboard post *and* goes into the breach,
   which is otherwise an odd thing for a starship to ask of one person. In single player the Hazard Team is
   the answer, and in the starting states it is where an ensign begins.
3. **Its casualties gut three departments at once.** A squad contains a technician, a medic, a demolitionist
   and a security specialist, so losing one action costs engineering, medical and security simultaneously.
   That is the *holes are the game* principle with real teeth, and it is the sharpest attrition lever in the
   whole roster.
4. **Two squads are a relief resource.** Missions take one squad and leave the other; a bad mission leaves the
   ship with only B-squad, which is a worse team and a demoralised one. Canon gives us the resource for free.
5. **Its training is on the holodeck.** The firing range and tactical drills are canon and the map already
   ships, which closes the training item in `docs/gap-analysis.md` with a canon use rather than an invented one.
6. **It gives the middle of the hierarchy a second reporting line**, and a good one: Foster takes tactical
   direction from Tuvok, tasking from the bridge, and answers for his people to the first officer who owns the
   crew. That is exactly the kind of overlap that produces arguments worth having on screen.

### Consequence for Tier A

`docs/crew-roster.md` recommended drawing the authored core from the game's own named characters. Here they
are, with roles, traits and a friendship included: **fourteen names, two squads, seven defined personalities**.
The work left is not invention -- it is filling in the drives, fears and allegiances the games left open, and
deciding who the player is.

## Who reports to whom

Administratively: **CO -> XO -> department heads -> division and shift leads -> specialists and crew.**

Operationally, while a watch is running, it is not the same chain at all:

- The **officer of the deck** -- the watch officer on the bridge -- has authority over the ship's operation for
  that watch, *including over officers senior to them who are not the captain*. Waking the chief engineer at
  0300 means the OOD is briefly giving orders to someone three ranks above them. That is the naval
  convention, it is canon-adjacent, and it is one of the best scenes the manifest can produce.
- **Under damage control, the chain changes again** -- a repair party reports to the engineer of the watch,
  regardless of rank.
- **In a boarding, security owns the corridors** and everyone else is a body to be moved.
- And with the command crew dead -- the multiplayer premise -- the administrative chain has a hole at the
  top, while the operational chain still works, which is exactly why the ship can be run by whoever is left.

## Special cases the manifest must not break

- **The Doctor**: a post with no rank. His authority is situational and explicit -- sickbay, medical
  decisions, quarantine -- and it does not extend to the conn.
- **Field commissions**: provisional ranks, flagged as such, which the crew can resent and which command
  decisions inherit.
- **Civilians with posts**: morale officer, botanical aide, consultants. They hold access by post and no
  authority by rank, which is why the player may have to *ask* rather than order.
- **Cross-trained crew**: a post can be held by someone qualified for it, not merely assigned to it. That is
  where credentials (`docs/access-and-authority.md`) meet the roster, and it is how a thin watch survives.

## Acceptance

- Every crew record carries rank, post and department as separate fields, and the manifest can be printed by
  any of the three.
- A full watch can be listed for any day and watch, with the stations manned and the day-specialists off.
- Each person has a battle station and an emergency bill assignment, and the ship can name the holes: posts
  with one qualified holder, and stations that no living crew member can fill.
- The officer of the deck's authority is honoured by the simulation against senior officers, with the
  exception of the captain.
- And the human check: a player can ask "who is on right now?" and get an answer that a real crew would
  recognise.
