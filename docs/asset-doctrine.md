# The asset doctrine: the base game is the palette

The owner's direction, 2026-10-07. A short document because it is a short rule, and it governs every piece of
visual content we will ever add — locations, props, panels, faces.

## The rule

**We do not need new assets. The base game supplies enough to reuse and remix into the visual diversity we
need.** Where a limit appeared to be licence-blocked, the answer is composition rather than acquisition.

## What that means in practice

- **Reuse, then remix, then compose.** A console or a panel anywhere on the ship is nearly identical to some
  console or panel elsewhere on the ship. What distinguishes them is the **LCARS content** — the information
  and controls drawn on the surface — not the furniture. So one panel asset, re-dressed and re-labelled, serves
  the whole ship, and the panel-painting path already built for the station consoles is the mechanism.
- **Characters are parts.** Mixing and matching parts of the shipped character assets produces faces and
  bodies we do not have. `docs/crew-manifest.md` and `docs/crew-roster.md` already take this posture — the
  crew are drawn from the game's own bots because those bots have models, skins and a place in the fiction.
- **Whole-cloth generation is in scope for our own content.** Writing a skill that generates character meshes
  and skins from scratch is not a stretch, and it is a legitimate way to make the anonymous crew particular.

## The boundary, which is sharp

**We may own and ship what we make for our own characters and the anonymous crew. We do not re-author named
characters at all.** This is the same line the voice work already drew, and for the same reason: Raven's source
grant reaches Raven's software, and it does not reach Paramount's characters and marks. A mesh generated to
look like a specific named crew member is a new artifact with no clean chain of title, whether it is sold or
given away. So: an original face for an original ensign is ours; a generated likeness of a lore character is
not, and lore characters use the shipped assets they already have.

**But the boundary is on what we author, never on what the simulation does to them.** The owner's framing,
2026-10-07: *"we'll leave lore characters alone outside of the consequences of what can happen to them within
our gameplay. Janeway would die, or get assimilated, which would suck."* So a named character is a crew record
like any other — they hold a post, they accumulate marks, and they can be wounded, spent, written off,
assimilated or lost, with the run carrying it. **They are not protected by authored content and they are not
re-authored.** That pairing is the doctrine: the cast is fully simulated and never restaged.

**And novel voice is code we ship, never audio.** Any new line a named character speaks is produced by
**the analyzer and synthesizer this repository ships**, acting on **the voice assets the player already
owns**, generated on the player's own machine, and it lives in their cache beside the save. The repository
carries the mechanism and never the output — the shape `docs/staff-meetings.md` settled on for voice, for the
same two reasons, licence and resources alike. **The boundary is on generation and distribution, not on
capability** (owner's ruling, 2026-10-07): nobody on this project generates an actor's voice and keeps the
result — not in the repository, not in an issue, not in a pull request, not in a document.

## And what never changes

**Nothing generated and nothing extracted enters the repository.** No game assets, no third-party images, no
generated meshes or skins committed as content — the same rule the audio work already follows. The repository
carries the mechanism and never the output; a clean clone plus a retail install remains the whole requirement.

## Why this matters to the programme

`docs/community-inheritance-audit.md` records that community *code* is adoptable with credit while community
*assets* are not adoptable without permission. This doctrine is what makes that a non-blocker: we were never
going to need their assets, because the palette we already own — fourteen decks of interiors, four paks of
character models, six holodeck programmes and the whole entity dictionary — is enough to dree the ship.
