# Evidence: mode 2 staged and serving (cMod v1.30)

The inherited multiplayer client is installed beside our own build and serves a retail map, which is
what mode 2 needs before anyone plays it. Nothing of cMod is copied into our source tree and no patch
is applied to it: this is the "inherit, don't own" decision made concrete, so their next release drops
in without a rebase on our side.

## What is staged

| item | value |
|---|---|
| release | `ioEF-cMod v1.30` (published 2025-12-19) |
| asset | `ioEF-cMod_v1.30_linux_x86_64.zip`, 3,087,306 bytes |
| sha256 (first 24) | `19426ca04167f13e6927117d` |
| location | `build/cmod/` -- beside our engine, never inside the game installation |
| binaries | `cMod-stvoyHM` (client), `cMod-dedicated` (server), `cmod_renderer_opengl1.so`, `cmod_renderer_opengl2.so` |
| payload | `baseEF/pakcmod-release-2025-12-16.pk3` |

All are native x86-64 ELF, dynamically linked, **zero missing libraries** on the playtest host.

## What was proven

The dedicated server, pointed at a directory containing `baseEF`:

```
Indexed 19451 files in 5 pk3s, 702 other files, and 2268 shaders.
Server: hm_borg1
gamename: baseEF
InitGame: ... mapname\hm_borg1\protocol\24\com_protocol\26\com_gamenamename\EliteForce
          version\cMod HM v1.30_GIT_4341b89-2025-12-18 linux-x86_64 \sv_maxclients\32
AAS initialized.
```

So: retail data located, a Holomatch map loaded with bot navigation, 32 client slots, cMod's own
protocol. That is the server half of a LAN session working end to end.

## The one configuration fact that matters

cMod's README says the GOG installation is found automatically; that is a Windows-path feature and
does not fire on Linux. Left to itself the client indexes **3 files in 1 pk3** -- its own package
only, with no game data at all, then appears to hang. It needs the data path stated:

```
+set fs_basepath <dir containing baseEF>    +set fs_homepath <writable dir>
```

`scripts/run-cmod.sh` does this, and writes into `build/cmodhome` so nothing touches the installation
or a home directory. `--dedicated` runs the server form instead.

## Still to confirm with a human

The client's own first-run experience -- menus, the server browser, connecting to a local server, and
input. That is the part a headless run cannot see, and it is the reason the previous attempt at a
Holomatch session showed "connecting to localhost" with no avatar control. Note this is now cMod's
client rather than our engine's Holomatch path, so that fault is theirs to have already fixed rather
than ours to inherit.
