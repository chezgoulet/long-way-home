# G1 evidence — the engine loads the real game data

Date: 2026-10-04. Gate: **G1, milestone M1** — met. The engine builds, runs, and reads the retail
content on the playtest host.

## Where the game was

The GOG installer was added to Steam as a non-Steam game, so Steam installed it inside a Proton
prefix rather than where the installer sat:

```
~/.local/share/Steam/steamapps/compatdata/4009300482/pfx/drive_c/
        Program Files/GOG Galaxy/Games/Star Trek Elite Force/
```

Contents: `stvoy.exe`, `efgamex86.dll` (999,500 B), `efuix86.dll`, `binkw32.dll`, and `BaseEF/` with
`pak0.pk3` (566 MB), `pak1`–`pak3.pk3`, `playermaps.pk3`, `efq3.key` and `expefq3.key` — 659 MB in
total. The expansion key being present suggests the expansion data is installed, which matters for the
Virtual Voyager bar at G2.

The data is **linked, never copied**: `build/baseEF` is a symlink to the installation. Nothing of it
enters the repository.

## Two things that had to be learned rather than assumed

**Case matters, and the assumptions were mine.** GOG ships the directory as `BaseEF`; the engine looks
for `baseEF`. A candidate list that hard-coded (`baseEF`, `BaseEf`) reported a complete installation as
missing on a case-sensitive filesystem. Found by running against the real install; fixed with a
case-insensitive search.

**The engine resolves its data path from the binary's location, not the working directory.** Running
the server by absolute path from the linked build directory still looked in
`.../release-linux-x86_64/baseEF`. It needs the path stated:

```
lilium-voyager-server_x86_64 +set fs_basepath <dir containing baseEF> +set fs_homepath <same>
```

That is an integration detail our own launcher will have to honour, not a defect.

## The run

```
Lilium Voyager HM 1.40_GIT_0f7dcd8-2026-05-20 linux-x86_64
SSE instruction set enabled
----- FS_Startup -----
/home/c/big/git/long-way-home/build/baseEF/playermaps.pk3 (60 files)
/home/c/big/git/long-way-home/build/baseEF/pak3.pk3 (1685 files)
/home/c/big/git/long-way-home/build/baseEF/pak2.pk3 (794 files)
/home/c/big/git/long-way-home/build/baseEF/pak1.pk3 (67 files)
/home/c/big/git/long-way-home/build/baseEF/pak0.pk3 (19069 files)
----------------------
21675 files in pk3 files
execing default.cfg
Hunk_Clear: reset the hunk ok
--- Common Initialization Complete ---
```

A native Linux Elite Force engine, on the playtest host, reading 21,675 files of retail content.

Both renderers linked on this host (OpenGL1 and OpenGL2), unlike the build host where the GL2 targets
failed on SDL's transitive dependencies — the difference is the local library set, not the code.

## What remains for G1

- **M2** — the engine loading our `libefgame.so` and `libefui.so` instead of the retail
  `efgamex86.dll`, and rendering the menu.
- **M3** — play a mission, write a save, reload it. The acceptance test, and the first thing that needs
  a person at the machine.
