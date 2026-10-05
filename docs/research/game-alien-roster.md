# The alien roster in Elite Force — what we can field without authoring art

Censused 2026-10-05 from three independent witnesses: the NPC classes placed in all 147 shipped map
sources, the `AI_*` implementations in the released game code, and the complete character-model list
across the four retail paks. Counts are placements across the map sources.

## Canon Voyager species, present as enemies

- **Borg** — 440 placements, `AI_Borg`, models `borgbig`, `borgthin`, `borgbolts`, `borgfoster`. Also the
  only faction with named individuals in the bot roster: `7of9`, `borgqueen`, and drone designations
  (`1of729`, `2of3`, `5of9`).
- **Species 8472** — 161 placements, `AI_Species8472`, model `species8472`. From *Scorpion*.
- **Hirogen** — 26 placements, `AI_HirogenAlpha`, models `hirogen`, `hirogen_boss`. Season four's hunters,
  which makes them the natural fit for a ship-boarding scenario: it is what they do.
- **Malon** — 42 placements, model `malon`. The waste-scow species, and the reason a scenario about
  something *dumped* on the ship is already asset-supported.
- **Klingons** — 124 placements, models `klingon`, `klingonfem`. Present via the holodeck programmes
  (`_holodeck_kln1`, `ctf_kln1`, `hm_kln1`) rather than the campaign plot, plus characters (`chang`,
  `gowron`, `khaless`, `jurot`).

## Raven's inventions — Elite Force's own, and most of its campaign

- **The Etherians** — `AI_Etherian`, 11 code files.
- **The Vohrsoth** — `AI_Vohrsoth`, model `forge_boss`. The Forge's antagonist intelligence.
- **The Scavengers** — `AI_Scavenger`, 30 code files, models `scavenger_skins`, `munroscav`, `alexascav`,
  maps `scav1`–`scav5`, `scavboss`. A whole arc and a whole species.
- **The Reavers** — `AI_Reaver`, 173 placements, model `reaver`. The second-heaviest enemy placement in
  the game after the Borg.
- **The Harvesters** — `AI_Harvester`, 122 placements, model `harvester`.
- **The machine family** — `AI_HeadBot`, `AI_ScoutBot`, `AI_WarriorBot`, `AI_HunterSeeker`; models
  `headbot`, `scoutbot`, `warbot`, `warbot_boss`, `hunterseeker`. 111 scout-bot placements alone.
- **Creatures** — `biohulk`, `parasite`, `vermin` (25 parasite placements).

## Present, but I cannot attribute them confidently

`imperial` (79 placements, models `imperial`–`imperial6`, `impfem`, `chaoticaguard`), `stasis` (149
placements, maps `stasis1`–`stasis4`), and `avatar` (43 placements). These are certainly content, and
their names are Raven's, but nothing in the code ties them to a canon species. Treat them as
Elite Force-internal factions until a scenario needs them named.

## Present as characters rather than enemies

Crew and guests, useful because they are already modelled and animated: humans (the Hazard Team —
`munro`, `alexandria`, `biessman`, `chell`, `foster`, `mackey`, `toviedo`, `pelletier`, `telsia`),
Vulcans (`tuvok`, `vorik`), a Bolian (`chell` has a Bolian head variant, `hed_borg_bolian`), a Talaxian
(`neelix`), plus `sela` (Romulan), `tolek` (Cardassian), `sevens`, the Doctor, Janeway, Torres, Kim,
Chakotay, Paris. The holodeck programmes add Camelot, Old West (`desperado`, `paladin`) and a Klingon
programme — which is how the ship can have a *pastime* with shipped assets.

## What is NOT in the game, and therefore costs us

**No Kazon. No Vidiians. No Jem'Hadar. No Ferengi. No Romulan or Cardassian enemies** — the code returns
zero files for each, and neither appears in the model list; Romulans and Cardassians exist only as one
bot each. So any scenario built on them is authored from nothing: models, animation, AI behaviour and
voice. Worth knowing before a scenario is designed around them, because the four factions we *do* have
in depth — Borg, the Forge (Scavengers, Reavers, Harvesters and machines), Species 8472, and the
Hirogen — can carry a great deal of story with zero new art.
