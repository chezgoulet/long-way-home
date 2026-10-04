# Track B evidence — the map toolchain runs natively, and navigation bakes itself

Date: 2026-10-04. Track B (authoring pipeline), first gate item: *headless map and navigation build,
proved by rebuilding a shipped map.*

## A shipped map compiles, in the game's own format

Raven's own `_brig.map`, unmodified, straight from the published map sources:

```
q3map2 -fs_basepath <install-linked dir> -fs_game baseEF -game quake3 _brig.map
  ... 1080 stripped surfaces ...
  Writing /tmp/nrc/work/_brig.bsp
  Wrote 0.5 MB (549192 bytes)
```

- **`IBSP` version 46 — byte-identical version field to the retail `_brig.bsp`.** What we produce is
  in the format the game loads, not an approximation of it.
- **Zero shader misses.** q3map2 resolved the map's shaders out of the retail paks, which means the
  editor and compiler can work against the real asset set rather than a reconstruction.

Toolchain, all native on Linux: **q3map2 v2.5.17n** (ydnar's maintained branch) and **mbspc 2.2**
(the nav/AAS compiler), both from NetRadiant-custom's Linux release.

## The game pack, assembled rather than vendored

`tools/gamepack/make-gamepack.sh` builds `eliteforce.game/` from the owner's GDK and installation:

```
entity classes : 214      (the authoritative SP entity dictionary, from the GDK)
shader scripts : 35       (derived from the retail paks' own scripts/*.shader)
```

It writes the four files NetRadiant expects — `main/eliteforce_entities.def`,
`main/default_shaderlist.txt`, `default_build_menu.xml`, `game.xlink` — with the compile presets
already carrying `-fs_game baseEF -fs_basepath`. Neither Raven's dictionary nor the game data enters
the repository: both are supplied at build time.

## Navigation bakes itself — this is the important find

The retail installation ships a `.nav` file beside every map (`maps/_brig.nav`), and the SP navigator
loads it. The question that mattered for authoring is whether a new space needs a separate nav tool.
It does not:

```
g_main.cpp:290   navCalculatePaths = ( navigator.Load( mapname, checkSum ) == qfalse );
g_main.cpp:319       NAV_CalculatePaths( mapname, checkSum );
g_navigator.cpp:479  gi.FS_FOpenFile( va( "maps/%s.nav", filename ), &file, FS_WRITE );
```

**When `maps/<map>.nav` is missing or its checksum does not match, the game computes the paths and
writes the file itself on load.** So authoring an inhabited space is: geometry, plus `waypoint` family
entities in the map, plus one load to bake the navigation. There is no third tool to run, and no
format to reverse.

That also closes the loop with the validator: it already counts navigation coverage per space, so the
gate and the baker agree on what "navigable" means.

Related detail worth keeping: the released source's `.nav` reader had an LP64 bug — `GetLong` read 8
bytes where the format is a 4-byte FourCC (`'JNV2'`), desynchronising on 64-bit builds. Upstream
patched it; our x86_64 build inherits the fix.

## What is not yet proven

**No compiled map has been loaded in the engine.** Compiling and loading are different claims, and
only the second one closes this gate item. That needs a session on the playtest host: drop the
compiled `.bsp` into the data's `maps/`, load it, and confirm it spawns — at which point the `.nav`
should appear beside it as evidence that navigation baked too.
