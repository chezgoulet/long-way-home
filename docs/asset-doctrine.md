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

**We may own and ship what we make for our own characters and the anonymous crew. We may not ship likenesses
of named characters.** This is the same line the voice work already drew, and for the same reason: Raven's
source grant reaches Raven's software, and it does not reach Paramount's characters and marks. A mesh
generated to look like a specific named crew member is a new artifact with no clean chain of title, whether it
is sold or given away. So: an original face for an original ensign is ours; a generated likeness of a lore
character is not, and lore characters use the shipped assets they already have.

## And what never changes

**Nothing generated and nothing extracted enters the repository.** No game assets, no third-party images, no
generated meshes or skins committed as content — the same rule the audio work already follows. The repository
carries the mechanism and never the output; a clean clone plus a retail install remains the whole requirement.

## Why this matters to the programme

`docs/community-inheritance-audit.md` records that community *code* is adoptable with credit while community
*assets* are not adoptable without permission. This doctrine is what makes that a non-blocker: we were never
going to need their assets, because the palette we already own — fourteen decks of interiors, four paks of
character models, six holodeck programmes and the whole entity dictionary — is enough to dree the ship.
