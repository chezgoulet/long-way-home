# Star Trek: Voyager — Elite Force: Adding Content at Home

Notes compiled 2026-10-04. GOG copy (base game + Expansion Pack). Home use, nothing for sale.

---

## The short answer

Yes — and considerably more than a typical 26-year-old game allows, because of two things:

1. **Raven Software officially released the game source code** for both the singleplayer and the
   multiplayer halves of Elite Force. Not a decompile, not a leak — an official source release under
   a written licence that explicitly permits making modifications.
2. **The multiplayer half is now freeware**, running on an open-source engine that is actively
   maintained and still getting releases as of December 2025.

That means "add content" here covers new maps, new weapons, new NPCs, new game modes, and new
singleplayer entities — not just texture swaps on the existing levels.

---

## Tier 1 — Modernise what you already own (low effort, high payoff)

The Holomatch (multiplayer) component ships with a 2000-era client: dead master server, no
widescreen, shader limits, no server browser. It has been replaced.

**cMod** — an unofficial Holomatch client/server built on ioEF (an ioquake3 conversion).
Current version **1.30, released 2025-12-18**. Windows / macOS / Linux builds.
What it fixes: widescreen aspect correction and configurable HUD centring; in-game server browser;
auto-corrected map brightness; a filesystem that removes pk3 conflicts and loading limits; settings
that servers can no longer change behind your back; support for maps that the original EF cannot
load at all because of the 1024-shader limit; HTTP file downloads.

Install note for GOG owners: you do not have to touch your GOG folder. Extract the zip into any
empty directory and run it from there — it locates the GOG game data automatically.

**HD content packs** (these stack on top and are used with the normal client or cMod):

- *Elite Force Graphic Overhaul Project* — HD retextures of Voyager, the tutorial, some Borg, and
  Holomatch terrain; HUD, GUI, levelshots.
- *Star Trek Voyager Elite Force Remaster Fan Edition* — full-HD texture support, ~792 MB base
  plus patches, still being updated (patch dated 2025-10-09). No gameplay changes.
- *Sparkss Elite Force Weapon Sound Overhaul* — better weapon audio.

Realistically: one evening, and the game stops looking and sounding like 2000.

---

## Tier 2 — More places to play: new maps, including bots

The mapping toolchain and Raven's own source maps are both public:

- **Official Game Development Kit (GDK) v1.1** — efRadiant (map editor), q3map2 (map compiler),
  bspc (bot navigation compiler), shaderlist, and four sample maps.
- **Raven's own Holomatch `.map` sources** — the actual source files for the shipped multiplayer
  levels, released publicly. Useful both for learning and for remixing.
- Because cMod lifts the old shader limit, maps ported from Quake III Arena can be made to work
  in Elite Force — a much larger pool of level geometry than EF ever had.

Bots matter here: `bspc` generates the navigation data the bots use, so a new map can be fully
bot-populated. A custom map is playable solo, against bots, or with other people.

Effort: a simple arena is a weekend. A map worth showing people is a few weeks of evenings.
The compiler side of this can be run on a Linux box (q3map2 is GPL and builds natively);
only the visual editor really wants Windows, and `.map` files are plain text.

---

## Tier 3 — Genuinely new gameplay (source code work)

The released sources give us, in C:

- **Multiplayer game source** (Holomatch Source Code v1.1 / v1.2) — weapons, game modes, scoring,
  server logic.
- **Singleplayer game source** (expansion-version based) — includes `game/` (game logic),
  `cgame/` (rendering and client effects), and data files under `BaseEF/ext_data/`:
  `weapons.dat`, `NPCs.cfg`, `items.dat`, `addon.npc`, `boltOns.cfg`.

Some of this is config-level, not code-level: NPC characteristics and weapon definitions live in
those data files, so tuning or extending them is closer to editing values than writing a mod.

What this makes possible: new weapons, new multiplayer modes (objective modes, wave/assimilation
defence, Voyager-vs-Borg scenarios), new NPCs and adversaries, new singleplayer entities for
custom levels.

Effort: a new game mode is days-to-weeks. This is real development, not tweaking.

---

## Tier 4 — Play it together, without everyone buying it

In August 2020, for the game's 20th anniversary, the **Holomatch multiplayer component was released
as freeware** — Raven and CBS gave permission to distribute the code and assets. There is a free
Windows installer and a free Linux build (both ~1.3–1.4 GB, verified live 2026-10-04):

- Windows: https://last-outpost.net/download/stvoyHM_setup.exe
- Linux: https://last-outpost.net/download/stvoyHM_Linux.zip

Hosted by **The Last Outpost / holomat.ch**, a Star Trek community that also maintains RPG-X.

This means a dedicated server can be stood up on your own hardware and other people can join
*without owning the game*. Combined with cMod's Linux server build and the bot support above, a
private server that always has something happening on it is entirely feasible — LAN play with
family, or reachable over your own VPN when away.

---

## Bonus: RPG-X

RPG-X is a long-running Elite Force modification that turns the multiplayer mode into a Star Trek
roleplay environment — interior sets, props, animations, emotes, and character customisation. It
began as a mod and later got a standalone re-release, made with permission. If "more content" means
"more to do on the Holodeck," this is the single largest body of it.

---

## Ports and curiosities

- **Elite Reinforce** — a source port of the singleplayer, focused on bugfixes and speedrunning
  movement. Early (v0.6) and specialist.
- **VoyagerSP-Android** — an Android singleplayer port on idTech3/Quake3e with a Vulkan renderer,
  GPL-2, actively developed into mid-2026. Interesting, but it needs the retail game data.

---

## Licensing, in plain terms

The game source ships under Raven's *STEF Game Source License* (dated 2002-11-21). The relevant
parts, read from the licence document itself:

- You may **create your own modifications** ("New Creations") for use with the full version of the game.
- You may **distribute those modifications free of charge for non-commercial purposes** to other end users.
- You may **not** sell, rent, lease, or commercially exploit them.
- You may not reverse engineer or modify the shipped game itself, or disable any anti-piracy measures.
- The engine itself (ioquake3 lineage) is GPL.

So the licence permits more than you were planning to do: making things and giving them away free is
explicitly allowed. Nothing on this page is a legal risk, and nothing here requires sharing anything
you'd rather keep to yourself.

---

## Where the active community is

- **stvef.org** (Elite Force Resources) — cMod downloads, Pathfinder server browser, model packs,
  map lists, a live list of community servers, master-server info, common-error fixes.
- **holomat.ch / last-outpost.net** — The Last Outpost: freeware Holomatch, RPG-X, and a Discord.
- **Nexus Mods** — Star Trek: Voyager Elite Force section (Raven's released map sources are here).
- **ModDB** — Star Trek: Elite Force section: GDK, source code releases, mods, addons, patches.

---

## Verified as of 2026-10-04

- cMod 1.30 — win x86_64 (3 MB), linux x86_64 (2 MB), mac ub2 (6 MB), released 2025-12-19.
- Freeware Holomatch — Windows installer 1,302,812,379 bytes; Linux zip 1,404,434,559 bytes;
  both HTTP 200.
- Remaster Fan Edition — base ~792 MB, patch dated 2025-10-09.
- GDK v1.1 and both source releases — present and downloadable on ModDB.
