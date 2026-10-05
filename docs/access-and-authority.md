# Access and authority: two different locks

The owner's model, 2026-10-05: **rank and role grant technical access to the ship, and the constraints lift as
you go up.** The captain can therefore operate the entire ship alone if everyone else is dead -- a capability
that is always held and almost never used, because in practice the captain simply tells people what to do. The
first officer holds the same kind of ship-wide access, and every post below is scoped down the chain.

## Two axes, kept apart

This only works if two different things are not confused:

- **Access** is *technical*: will this console accept input from this person? Hard, binary, rank-derived, and
  testable. The captain's console accepts anything; a junior's accepts its own post.
- **Authority** is *social*: will the crew carry the order out? Soft, probabilistic, and governed by morale,
  respect, and how the last order went -- which is why command can be lost (see `docs/start-states.md`).

So: **rank controls access absolutely; rank governs compliance probabilistically.** A captain alone with the
crew dead has full access and no compliance problem. A junior at the captain's console has no access at all.
A captain giving an order the crew believe is suicidal has access to the console and a mutiny on their hands.
Both locks, both real, and they should be modelled separately because they fail differently.

## The scope ladder

| scope | who | what it means in practice |
|---|---|---|
| **ship-wide** | captain, first officer | every control on every deck, plus allocation, authorisation and override |
| **department** | department heads | all systems in their section: engineering over power, warp, EPS; medical over sickbay and the EMH; security over doors, force fields and the brig |
| **post** | everyone else | the systems their station actually operates, and nothing else |
| **read-only** | crew in transit, visitors, passengers | status without command |
| **none** | intruders, and anyone the crew have cut off | the console does not answer |

## Canon already has this, and it is specific

Warp core ejection requires **the chief engineer's or senior staff's authorization code** -- not a rank in the
abstract, a *credential* held by particular people. Red alert can be raised by more than the captain. The ship
is canonically operable with as few as a hundred crew. Every one of those is the same mechanic: some controls
are behind a name, and the name can be delegated.

Which gives access four sources, in order of how often they matter:

1. **The post you hold.**
2. **Credentials you have earned** -- cross-training, a qualification, a code given to you for one task.
3. **Delegation** -- a department head grants a junior their section's access for a shift, and can take it back.
4. **Emergency override** -- slow, loud, logged, and usually needs two people to agree.

## When everyone is dead

The full-access fallback exists for exactly this, and the paths down to it should be graceful:

- **Field promotion first.** If the senior officer is gone, the roster promotes by the same rules that run
  during play, and the surviving crew inherit the access that comes with the post. This is the normal path.
- **Override second.** A skeleton -- or one person -- can force the ship to accept them, at a cost: it is slow,
  it is logged, and it is the kind of thing a board of inquiry asks about later.
- **The EMH third.** Canon gives the hologram authority in emergencies, which makes the Doctor a legitimate
  holder of the keys when nobody else is left. A ship run by its doctor and its computer is a very Star Trek
  emergency, and it is free content.

## What the console shows, which is how the hierarchy teaches itself

A locked control is **visible and named**: not hidden, and not silently inert. It says what it needs --
*engineering authorisation required*, *medical override*, *bridge command* -- which does three things at once.
It teaches the player the chain of command without a manual, it answers "who do I wake up" mechanically, and
it makes the moment access is granted feel like something.

## The cost of using the top

The captain can do everything and can only be in one place. Operating a console means leaving the bridge, which
means the bridge is unstaffed. So the admin capability is a *rescue valve*, not the normal mode -- and using it
should feel like an emergency even when it works, because the ship's command is now standing in the coolant
bay instead of watching the sensors.

## Three edge cases worth modelling, because they are the interesting ones

- **A compromised system does not obey rank.** A sliced console (see `docs/borg-incursion.md`) refuses
  everyone, including the captain, until someone clears it. Hardware beats hierarchy.
- **A dead officer's credentials still work.** The computer does not know they are dead. Using them is
  possible, sometimes necessary, and ethically its own scene.
- **The crew can revoke.** If command is losable, access is too: a relieved captain's credentials stop working,
  and the ship records who turned them off.

## Acceptance

- Access and authority are separate quantities, and a locked control names who can open it.
- The same action is refused at one post and permitted at another, and the refusal is legible on the console.
- Delegation and revocation both work, and both are recorded in the log.
- The captain can operate the entire ship alone -- and the simulation makes the cost of doing so obvious.
- A sliced system refuses everyone, and a dead officer's credentials are still usable and still a decision.
