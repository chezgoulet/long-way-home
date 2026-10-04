# G1 evidence — the engine lineage builds and runs on native Linux

Date: 2026-10-04. Gate: **G1 = M1** (engine builds and runs; binary starts, reports version).
Status: **partly met, and better than expected.** Milestones M1/M2/M3 are not yet passed — the engine
still has to load our modules and, with game data, play a mission — but the foundation question is
answered.

## What was verified

The engine lineage our single-player client builds on is `lilium-voyager` (plus, in the Android port,
a Vulkan renderer and the `sp/` bridge). Its README claims GNU/Linux support; nobody here had tested
that claim, and the G1 estimate depended on it.

Built natively on the build host with gcc 15, using a rootless SDL2 (no system packages installed):

- The engine **compiles**: platform detection, all client translation units, and both renderer trees.
- `lilium-voyager-server_x86_64` **links and runs**. It prints:

```
Lilium Voyager HM 1.40_GIT_0f7dcd8-2026-05-20 linux-x86_64 Oct  4 2026
SSE instruction set enabled
----- FS_Startup -----
We are looking in the current search path:
/home/robot/.local/share/lilium-voyager/baseEF
./baseEF
0 files in pk3 files
"pak0.pk3" is missing. Please copy it from your legitimate EliteForce CDROM. ...
```

- `lilium-voyager-renderer-opengl1_x86_64.so` links.

Two useful facts fall out of that output: a native engine binary from this lineage starts on this
machine, and it tells us exactly where it wants game data — `baseEF/pak0.pk3` under the working
directory or `~/.local/share/lilium-voyager/`.

## What did not build, and why it does not matter as much as it looks

`lilium-voyager-renderer-opengl2_x86_64.so` and the client's final link fail on SDL's transitive
dependencies (`wl_proxy_*`, `drm*`) — the flags land in a variable position the Makefile does not use
for those targets. That is a quirk of lilium's hand-written Makefile, not of the code: the objects all
compile. **Our desktop target will write its own link line**, so this is a detail we control rather
than an unknown. Worth noting too that the single-player port's engine uses Vulkan, not OpenGL, so the
GL2 renderer is not on the critical path.

## Consequence for the plan

M1 is a build-and-link exercise over code that already compiles — materially cheaper than the charter
assumed. The remaining engine work is: a native CMake target for the port's own engine tree
(`efcode/`, which is lilium plus the Vulkan renderer and the `sp/` bridge), a desktop logging shim in
place of `__android_log_print`, and the module load path so the engine picks up `libefgame.so` and
`libefui.so` instead of the retail `efgamex86.dll`.
