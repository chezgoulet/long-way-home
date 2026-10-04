# G1 evidence — the single-player engine starts on the playtest host

Date: 2026-10-04. Gate: **G1, milestone M2**. Everything up to rendering is proven; the last step
needs a logged-in desktop session.

## The run

Unmodified binary, real game data, virtual display:

```
Lilium Voyager SP 1.40 linux-x86_64 Oct  4 2026
SSE instruction set enabled
----- FS_Startup -----
/home/c/big/git/long-way-home/build/baseEF/playermaps.pk3 (60 files)
.../pak3.pk3 (1685 files)  .../pak2.pk3 (794 files)  .../pak1.pk3 (67 files)  .../pak0.pk3 (19069 files)
21675 files in pk3 files
execing default.cfg
Hunk_Clear: reset the hunk ok
----- Client Initialization -----
----- Initializing Renderer ----
QKEY building random string
QKEY generated
----- Client Initialization Complete -----
SP: map/transition/use/save/load commands registered
----- R_Init -----
VKimp_Init( )
Available physical devices:
 0: Discrete AMD Radeon RX 590 Series (RADV POLARIS10), 0x67df
 1: CPU llvmpipe (LLVM 20.1.2, 256 bits), 0x0000
...selected physical device: 0
vulkan: No DRI3 support detected - required for presentation
```

Read what that says, item by item:

- **`Lilium Voyager SP`** — not the `HM` (Holomatch) build: this is our engine, with the SP bridge.
- **21,675 files in pk3 files** — the retail content, all four paks plus playermaps.
- **`Client Initialization Complete`** — the client came up.
- **`SP: map/transition/use/save/load commands registered`** — the SP integration layer is live inside
  the engine and has registered its console surface. This is the M2 wiring working.
- **`R_Init` → Vulkan enumerates the real GPU** and selects the discrete Radeon RX 590 (RADV Polaris10).
  The renderer initialises far enough to choose a device on this hardware.

## Where it stops, and why that is the display rather than the engine

Under the virtual display: `vulkan: No DRI3 support detected - required for presentation`. Vulkan
needs DRI3 to present, and Xvfb has none. On the console displays (`:0`, `:1`) it stalls even
earlier, because the seat is sitting at a **login screen** (`who` reports `(login screen)`) — there is
no logged-in session to present into.

Both are environmental, not defects in the port. Neither can be worked around from an SSH session.

## What this leaves for M2

One step: run it from a logged-in desktop session and see the LCARS menu. The SP module
(`libefgame.so`) loads on a map load rather than at startup, so the sequence to watch for is the
menu, then `spmap <map>` bringing the module in.

```
cd /home/c/big/git/long-way-home
./scripts/run-engine.sh
```

The script now tees the engine's output to `build/run-<timestamp>.log`, so a session leaves evidence
that can be read afterwards instead of a recollection of what appeared on screen.

## Not present in this build

Cutscene playback: the playtest host's FFmpeg packages are behind Ubuntu Pro, so Bink support is
compiled out and `platform/bink_stubs.c` skips `.bik` files. Cinematics are milestone M5.
