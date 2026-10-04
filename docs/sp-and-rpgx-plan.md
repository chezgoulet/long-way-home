# Immersive Elite Force — the singleplayer lane, and where RPG-X actually fits

Companion to `elite-force-content-options.md` and `ef-community-catalog.md`. Compiled 2026-10-04.
Player platform: Linux. Goal: immersive first-person experience at home, nothing for sale.

---

## First, the distinction that matters for your goal

**RPG-X is not a singleplayer experience.** Its own description is explicit: it has *no predetermined
missions, campaigns, or goals*. It is a stage — ship interiors, props, consoles, emotes, class-based
roles — designed so that a group of people can act out Star Trek together. The immersion is spatial
and social. Walking a ship alone in RPG-X is possible, but you are an actor on an empty set.

**The singleplayer extensions are the immersive first-person lane** you're describing, and they are
the reason to be interested. Starbase 11 in particular was built as exploration first: a hub-based
tour of a starbase in 2267 that you move through map by map, with sixteen optional combat missions
runnable as holodeck simulations from inside the tour.

So: SP extensions = solo immersion. RPG-X = the social one, worth it if other people are involved.

---

## The singleplayer extensions

**Starbase 11** — Laz Rojas. Multi-part, TOS era, 2267. Planetside base plus an orbital facility;
nine named building interiors, a range, a park, a disco, drydocks, a dreadnought, a starliner, plus
holosims. Six zip parts, 473 MB total, direct from the author's own site (verified live 2026-10-04):

- http://lazrojas.com/elitefarce/mods/starbase11/starbase11_1.zip (86.2 MB)
- ..._2.zip (72.4 MB), ..._3.zip (83.6 MB), ..._4.zip (103.8 MB), ..._5.zip (86.1 MB), ..._6.zip (40.6 MB)

Install: unzip **all six** into your Elite Force directory — NOT into BaseEF. That creates a
`Starbase11` folder with its own launcher and documentation. Read the included readme before running.

**Where copied to locally:** the local staging directory (staged locally).

**Others worth having** (verified available on a mirror with direct file URLs; sizes approximate):

- The Argas Effect — fan-made "Season 4" of The Original Series. Act 1 complete (~84 MB); Act 2
  exists unfinished.
- Colony 7 — three chapters, described as complete. Chapter 1 (37 MB), Chapter 2 (31 MB), plus
  updates.
- Enterprise NX-01 Launchbay (19 MB) — teaser content for a project that never finished.
- Elite Force Revisited (34 MB) — a reworked/larger-scale take.
- Battlestar Bellerophon (10 MB) — texture pack.
- The Venorcis Project (5 MB).
- Freelance: Callum's Dream (105 MB) — the released chapter of the cancelled project.
- Named in community listings but not yet verified by me: Assaulted Empire, Project Independence,
  Regeneration, Infiltration, Escape Route, Alien Derelict.

Mirror with direct downloads: https://www.lonebullet.com/mods/g/star-trek-voyager-elite-force-mods-6759.htm?sort=n
(ModDB and GameFront also host most of these but are behind bot protection — fine in a real browser,
not scriptable.)

---

## Running singleplayer on Linux — the honest position

There is **no native Linux singleplayer client** for Elite Force today. Every modern open-source
engine descended from ioquake3 handles the multiplayer half only:

- ioEF, cMod, lilium-voyager, Tulip Voyager → Holomatch (multiplayer).
- VoyagerNX → Holomatch, on Nintendo Switch.
- VoyagerSP-Android → singleplayer, but Android/ARM only (idTech3 + Quake3e Vulkan, built from
  Raven's released SP source).

So today there are two routes.

**Route 1 — Wine/Proton (available now).** Run the GOG Windows build and unzip the SP mods into it.
Lutris has an entry for the game, and there are community install guides (PlayOnLinux walkthroughs
recommend old Wine builds, 32-bit prefix, sometimes Win98 mode for the installer). Expect some
fiddling; an old id Tech 3 game is usually friendly, but the guides that exist are from the Wine
1.3–1.6 era, so modern Wine/Proton is untested ground. This is the fastest path to playing Starbase
11 this week.

**Route 2 — build a native Linux singleplayer engine (a real project).** The pieces all exist:
Raven released the singleplayer game source (official SDK, `stvoy_sp_mod_sdk.zip`), and the Android
port proves that pairing that source with a lilium/Quake3e-derived engine works. lilium-voyager
already supports GNU/Linux, and the Android project ships a native SP game module built from the same
source. Porting that pattern to desktop Linux is build system and platform-glue work rather than
research. Honest estimate: a focused effort with a good chance of a working build, not a certainty —
call it two-thirds likely to produce something playable, with the failure mode being a stubborn
build/dependency problem rather than a dead end.

Route 2 is what makes *all* of the SP mods run natively, forever, without Wine. It is also the only
route that would let a dedicated singleplayer-capable server exist on local hardware.

---

## RPG-X — the social lane

RPG-X (UberGames, 2004) turns Holomatch into an interactive Star Trek roleplay environment: no frag
gameplay, class-based roles, ship interiors with working sets, character customisation, emotes,
props, and scheduled group sessions. Distribution is legitimate — the standalone release exists with
permission, and The Last Outpost distributes it.

**Getting it:**
- Official hub: https://last-outpost.net/rpgx/ (RPG-X Ultimate Edition — standalone installer,
  includes the client plus virtually every user mod dependency). Standard Edition is the smaller
  (~1 GB) variant and is on ModDB: https://www.moddb.com/mods/star-trek-rpg-x
  Note: their download route is JavaScript-gated — it works in a browser, not from a script. I could
  not verify the direct file path; I can mirror it via a browser if you want it archived here.
- Community: The Last Outpost Discord — https://discord.gg/zxV4f3Z6ch — which is where the scheduled
  roleplay happens. RPG-X without other players is a set with the lights on.

**Linux:** RPG-X has its own engine fork, `rpgxEF`, based on ioEF, and its repositories document
building on Linux (64-bit output, MySQL client needed for the game libraries). So RPG-X is
*natively* Linux-capable in a way the singleplayer isn't:
- https://github.com/UberGames/rpgxEF
- https://github.com/solarisstar/rpgxEF

---

## Co-op, if the point is playing with someone

A Holomatch co-operative mod exists for Elite Force I (the predecessor of the much larger
HaZardModding co-op project for Elite Force II): Holomatch Cooperative Mod beta 11, ~8 MB, live at
files.lonebullet.com. It turns Holomatch maps into co-op missions — a reasonable way to play with
Beatrice or on the LAN, though it is a beta from the mod's own early era.

---

## Permanent source: the Internet Archive

The most reliable inventory of official Elite Force files anywhere, no rot, no bot walls:
https://archive.org/details/star-trek-voyager-elite-force (243 files)

Confirmed present:
- Editing/Game Development Kit 1.1 and 1.2
- Editing/Holomatch Source Code 1.1 and 1.2
- Editing/Singleplayer Source Code 1.2 (`stvoy_sp_mod_sdk.zip`)
- Editing/Map Sources — Voyager, Borg, Holomatch, Virtual Voyager, Stasis, Forge, Scavenger,
  Dreadnought, Holodeck, CTF, Brig map sources
- Editing/Icarus Scripting Tools (SP mission scripting)
- Editing/Character Animation Sources + ASE model sources
- Dedicated Server/Linux 1.0 and 1.1 (multiplayer only)
- Patches 1.1/1.2 and the Mac 1.2.1 Holomatch patches

That page is the ground truth for anything we build.

---

## Staged so far locally

- `files/starbase11/` — all six parts, 473 MB, checksums recorded.

Nothing else pulled yet. Nothing has been installed, and no software has been run.
