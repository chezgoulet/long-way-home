# Elite Force — What the Community Actually Built

Compiled 2026-10-04. Companion to `elite-force-content-options.md`.

Elite Force did not get a mod scene — it got a second life. The engine was Quake III, Raven
released the source, and a small stubborn community has spent twenty-five years filling the game
with content that never shipped.

---

## The scale, measured

The community's main server host publishes its full map database as a plain text file. As of
2024-07-05 it lists **2,369 maps** in active rotation:

- 1,911 tagged Quake III ports (81%) and 456 Elite Force-native maps.
- 1,762 free-for-all and 605 capture-the-flag layouts.
- 560 downloadable map pk3 files are hosted in one archive, plus a thumbnail pack.

That is the single most useful fact about this game's mod scene: the 1024-shader limit used to stop
Quake III maps running in Elite Force, and lifting it (cMod and related engine work) turned the
entire Quake III level corpus into Elite Force content. The map library is not the product of
twenty-five years of Elite Force mappers. It is that, plus most of Quake III.

Source: https://stvef.org/maplists (map list, thumbnail pack, pk3 index)

---

## The best example of all: RPG-X

If one project justifies the whole scene, it is RPG-X.

Released in 2004 by UberGames, RPG-X is a total conversion of Elite Force's multiplayer mode into an
interactive Star Trek roleplay sandbox. It removes the frag-oriented functions of Holomatch entirely
and rebuilds it around character and story: class-based roles, ship interiors, props, emotes,
uniforms, interactive consoles, and a post-Nemesis era setting. Groups run scheduled roleplay
sessions on custom ships.

It is regularly described — including on its own ModDB page — as perhaps the most comprehensive
interactive Star Trek roleplaying experience ever released, and it is still supported today: it was
re-released as a standalone with permission, is distributed with the freeware Holomatch package, and
its community (The Last Outpost) runs an active Discord and plays weekly.

That is a twenty-two-year run for a fan project on a 2000 game. It is the best answer to "what has
the community done with it."

Links:
- https://www.moddb.com/mods/star-trek-rpg-x
- https://last-outpost.net/about/rpgx
- https://strpwiki.fandom.com/wiki/RPG-X

---

## Singleplayer: the campaign was extended by fans

The original campaign ends; the community kept writing. The notable additions:

**Starbase 11** — by Laz Rojas. A multi-part, TOS-era singleplayer experience set in 2267,
reconstructing the starbase seen in two Original Series episodes. It is deliberately exploration-
heavy: you tour the base rather than fight through it. Released in numbered parts, and still
downloadable. This is the benchmark for singleplayer mod work on Elite Force.
- http://www.lazrojas.com/elitefarce/mods/starbase11/index.html

**Star Trek: The Argas Effect** — a fan-made "Season 4" of The Original Series, made as a full
singleplayer mod with multiple acts. Developed by the group behind the SpaceStation K7 site. Act 2
exists in an unfinished form, which tells you what fan campaign production looks like in practice.
- https://www.moddb.com/mods/star-trek-the-argas-effect

**Colony 7** — singleplayer mod, described as complete, with three substantial chapters.

**Others in circulation** (list drawn from a community archive thread cataloguing singleplayer
campaign mods, dated 2019, so treat as leads rather than a verified inventory): Enterprise NX-01,
Assaulted Empire, Project Independence, Regeneration, Infiltration, Escape Route A.

**Star Trek: Freelance** — the cautionary tale and worth naming honestly. A non-commercial
story-driven project set about nine years after *Nemesis*, originally built as an Elite Force
modification, with real ambition and a released chapter ("Callum's Dream"). It was cancelled by its
author for lack of staff. The most ambitious singleplayer project the scene attempted, and it did
not finish.
- https://www.moddb.com/mods/star-trek-freelance

---

## Multiplayer gameplay mods: the game modes the developers never made

The community treated Holomatch as a platform. A partial roll of the ones that mattered:

- **IN2TAGIB** — Holomatch rebuilt as a sniper arena, with an improved interface, no-damage
  rocket-jumping, and unlagged hit registration.
- **Team Elite** — a Team Fortress-style class mod for Elite Force, with its own class systems and
  map logic. Maintained for years by a single modder.
- **Federation** — a bug-fix-and-expand project that reworked and added commands for most of the
  game's modes at once: FFA, Team Holomatch, CTF, Tournament CTF, 1v1, Assimilation, Specialties,
  Disintegration, Elimination, Action Hero.
- **nanoEF** — the purest example of playfulness in the whole scene: shrunken characters, grapple
  hooks, and a game rebuilt around being small. Its author kept working on sequels into the 2020s.
- **PiNBALL** — a mode where splash damage is the entire point and you shove opponents around the
  map.
- **Gladiator** — random-weapon round survival, last player standing wins.
- **Freeze Tag** — freeze the whole enemy team at once; teammates thaw each other by proximity.
- **PwrWeapons**, **Mr Pant's Excessive Overkill** — weapon overhauls from subtle to absurd.
- **Starfleet Tournament Mod** — replaces weapons, sounds, music, and interface, and ships sixteen
  maps, without touching the code.
- **TOS Weapons Mod** — replaces the arsenal with Original Series weapons.
- **Super Mario mod** — because of course.

Inventory source (a fansite that has curated this list since the early 2000s):
https://eliteforce.gamebub.com/mods.php

---

## Visual and audio overhaul

- **Elite Force Graphic Overhaul Project** — HD retextures of Voyager, the tutorial, Borg, and
  Holomatch terrain, plus a new OpenGL renderer. The most ambitious visual project.
  https://www.moddb.com/mods/elite-force-graphic-overhaul-project
- **Star Trek Voyager Elite Force Remaster Fan Edition** — full-HD texture support, ~792 MB base
  plus patches, updated as recently as 2025-10-09.
  https://www.moddb.com/mods/star-trek-voyager-elite-force-remaster-fan-edition
- **Sparkss Elite Force Weapon Sound Overhaul** — weapon audio.

---

## Engine work — the reason all of the above still runs

- **ioEF** by Thilo Schulz — the original ioquake3 conversion that reimplemented the Holomatch
  engine on the Quake III GPL source. Every modern client descends from it.
  https://github.com/thiloschulz/ioef
- **cMod** by Noah Metzger — the maintained multiplayer client/server (v1.30, 2025-12-19).
  https://github.com/Chomenor/ioef-cmod and https://stvef.org/cmod
- **Lilium Voyager** — another ioquake3 fork for Holomatch, which contributed the OpenGL2 renderer.
  https://github.com/zturtleman/lilium-voyager
- **Elite Reinforce** — a source port of the singleplayer, focused on bug fixes and speedrunning
  movement, with technical documentation of how Elite Force's movement differs from Quake III.
  Early (v0.6) but the most interesting singleplayer-side engineering in the scene.
  https://github.com/kugelrund/Elite-Reinforce
- **VoyagerSP-Android** — the singleplayer running on Android on idTech3/Quake3e with a Vulkan
  renderer, GPL-2, actively developed into mid-2026.
  https://github.com/imjustadudegamer/VoyagerSP-Android
- **Freeware Holomatch** — the multiplayer game and its community maps and models, released free
  for the 20th anniversary with Raven and CBS permission:
  https://last-outpost.net/download/stvoyHM_setup.exe (Windows), https://last-outpost.net/download/stvoyHM_Linux.zip (Linux)

---

## Still alive, for a 2000 game

Evidence the scene is not a museum:

- cMod 1.30 shipped 2025-12-19.
- The Remaster Fan Edition patch shipped 2025-10-09.
- One community member alone runs eleven public servers (free-for-all, CTF, gladiator, low-gravity,
  no-cheat, elimination) across three hosts — https://stvef.org/servers
- A separate community maintains a modelling toolchain (Milkshape plugins, MDR exporters, original
  animation source files in XSI format) so people can still make characters today.
- The sequel's co-op mod — a different game, but instructive — turned Elite Force II's entire
  singleplayer campaign into eight-player co-operative play and released its seventh generation in
  September 2025. No equivalent exists for Elite Force I. That is a gap, not an impossibility,
  given the singleplayer source is public.

---

## What this means for adding content at home

The scene's own history says the good outcomes came from picking one thing and going deep (RPG-X:
one total conversion, kept alive for two decades) rather than one person attempting a story-driven
epic (Freelance: cancelled). The map census says content volume is a solved problem. The interesting
space is: a game mode nobody made, a map for your own house, or a server your family actually plays
on — and the tools for all three exist and are documented.
