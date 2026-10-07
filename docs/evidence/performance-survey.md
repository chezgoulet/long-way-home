# Evidence — the performance survey: what the engine's snapshot path actually does

Date: 2026-10-07. Branch: `feat/performance-survey`, cut from `feature/g3-reactive-crew` (the trunk).
Brief: the performance-survey brief. It follows from §8 of `docs/design-north-star.md` (one ship,
one server), `docs/evidence/engine-content-headroom.md` (the sibling survey and its instrument,
`patches/0017`), `patches/0010` (the ship's snapshot selection), and
`docs/evidence/deck07-auxcore.md` (the frame numbers the deck runs report). This is the *frame
cost* gap that `docs/evidence/raise-the-gentity-ceiling.md` names as un-re-measured after the
entity array and tables widened to 8,192.

**Deliverable: a measured menu, not a patch.** Every claim carries its number and the `file:line`
that establishes it. The only code added is a measurement instrument, `developer`-gated and off by
default: `patches/0019` (the SP bridge) and one `developer`-gated log line in the module
(`module/crew/g_crew.cpp`). No behaviour changed.

**This is placed under `docs/evidence/`, not `docs/research/`, because it carries fresh
measurements** (§Observed) rather than a reading of existing ones. The code reading is in §The rule.

## Headline, before the numbers

The engine does **not** decide what enters a snapshot by distance. It decides by **visibility**,
and when more is visible than fits it keeps whatever the cut-off happened to reach — by entity
number in multiplayer, by distance in single player since `0010`.

The number the brief is about, **`MAX_SNAPSHOT_ENTITIES 256`, is the multiplayer server's cap, not
the single-player one.** The SP bridge compiles against the *module's* `cg_public.h`, where
`MAX_ENTITIES_IN_SNAPSHOT` is **1024**, so the merged ship's in-process snapshot is capped at 1024,
not 256. Measured on the loaded ship, that cap is **already spent in the ordinary case**: at the
player's spawn, and at every deck arrival tried, the PVS passes **1,100–1,200** entities and the
snapshot writes the cap, **1024, on every tick** (`trunc` counts the capped ticks: 381 of 381 over
20 s). The design's "everyone in one room" case is therefore *already* the ship's ordinary case for
the snapshot as built: the cap binds, and the only question the crowd changes is *which* entities
are kept, not how many.

**Distance-based model substitution exists in this lineage and is enabled — but it is render-side.**
It changes triangles per frame, not entities per snapshot. It cannot help the wire, which is the
constraint the design names.

The frame numbers: the ordinary ship's game module costs **2.0 ms/frame** (re-derived:
`game_frame_avg_us 1996`, max 5,097, over 381 frames); the bridge's snapshot build adds **~0.6 ms
per 50 ms tick**; a 32-body crowd raises the module's frame to **~3.5 ms**, flat from 1 to 32 bodies
in this harness; the direction layer's own decisions are **≤9 µs**. Everything else in the render
frame measured here is a **software Vulkan** frame (llvmpipe), and is not a number about hardware.

## The measurement platform, and its one caveat

The engine was rebuilt from the patched upstream (`scripts/bootstrap-upstream.sh`, nineteen patches,
then `cmake --build build-engine`); the module from `module/`. The run is the merged fifteen-deck
ship, `build/ship/out/longway_voyager.pk3` copied into the run's game dir, exactly as
`scripts/s3-check.sh` does. The renderer selected is **`CPU llvmpipe (LLVM 20.1.2)`** — a software
Vulkan device with no GPU, chosen because the run is headless under `xvfb`. **The whole-render-frame
time is therefore a software frame and says nothing about modern hardware.** The module's frame, the
bridge's snapshot build, and the entity counts are CPU numbers of the game and engine logic and are
not affected by that.

### A1 — the ordinary ship: the frame cost re-derived, and the snapshot at its cap

```
$ GAME_DIR=build/g3-home/baseEF
$ cp build/ship/out/longway_voyager.pk3 "$GAME_DIR/"
$ find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
$ SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a scripts/run-engine.sh --home-dir build/g3-home \
      +set com_hunkMegs 768 +set developer 1 +set s_useOpenAL 0 +set s_initsound 0 \
      +set g_crew 0 +set g_crewRun 20 +set g_crewQuit 1 +map voyager
EFSP: SP_SNAPSHOT: t=20000 ents=1024 peak_ents=1024 cand=1166 peak_cand=1202 trunc=381 built=381 build_avg_us=606 build_max_us=1014
EFSP: SP_FRAME:    t=20000 frames=1210 avg_us=15743 max_us=45000
```

```
$ python3 -c "import json;b=json.load(open('build/g3-home/baseEF/crew/voyager.baseline.json'));\
print(b['game_frames'],b['game_frame_avg_us'],b['game_frame_max_us'])"
381 1996 5097
```

Read plainly: the game module's own frame is **1,996 µs average (2.0 ms)**, worst 5,097 µs, over
381 sim frames of a 20-second run with the crew layer off. That is the number the deck runs report
as "game frame 2.1 ms average" (`docs/evidence/deck07-auxcore.md`); re-derived here it is 2.0 ms,
and this is what it was measured on: the merged ship, all fifteen decks' entities and scripts live,
`g_crew 0`. The bridge builds **one snapshot per 50 ms tick** (`SP_SV_FRAMEMSEC 50`,
`sp_bridge.cpp:141`); `build_avg_us=606` is that build's own cost, and `cand=1166` is how many
entities passed its visibility test. `ents=1024 … trunc=381 built=381` means **every one of the 381
ticks hit the cap**: the ship, standing still at spawn, already has more visible than the snapshot
holds.

### A2 — the population concentrated: 32 co-located bodies

The closest a headless harness reaches to a full holodeck: the crew layer's declared roster
(`crew::MAX_CREW` is **32**, `module/crew/crew_core.h:22`) with all members and all posts at one
compartment, and the player teleported there (`g_shipTest 4` → deck 4 arrival, deck 4 being a real
compartment on the merged ship). The config used is `maps/voyager.crew`, the file the module reads
(`g_crew.cpp:1276`):

```
$ cat > build/g3-home/baseEF/maps/voyager.crew <<'EOF'
crew max 32
bound reach_ms 2000
budget save_bytes_per_npc 256
member c01 type Renner at -3936 -2778 -13151
...32 members, 32 posts, all at the same point...
EOF
$ SDL_AUDIODRIVER=dummy timeout 60 xvfb-run -a scripts/run-engine.sh --home-dir build/g3-home \
      +set com_hunkMegs 768 +set developer 1 +set s_useOpenAL 0 +set s_initsound 0 \
      +set g_ship 1 +set g_shipTest 4 +set g_shipTestPos "-3936 -2778 -13151" \
      +set g_crew 1 +set g_crewRun 6 +map voyager
CREW: read maps/voyager.crew (32 post(s), 32 declared crew)
SHIP: standing at (-3936 -2778 -13151)
CREW: perf t=7000 game_frames=20 game_avg_us=3464 layer_frames=20 layer_avg_us=5 members=32
EFSP: SP_SNAPSHOT: t=7000 ents=1024 peak_ents=1024 cand=1275 peak_cand=1281 trunc=121 built=121 build_avg_us=559 build_max_us=658
```

The curve, same spot, same 20-frame second, declared members 1…32 (the module's instrument, one
line per second; `members` is the count the layer is driving):

| bodies | module `game_avg_us` | layer `layer_avg_us` | snapshot `cand` | snapshot `ents` |
|---|---|---|---|---|
| 0 (layer off) | 2,110 | — | 1,185 | 1,024 (capped) |
| 1 | 3,425 | 1 | 1,188 | 1,024 (capped) |
| 2 | 3,455 | 3 | 1,191 | 1,024 (capped) |
| 4 | 3,462 | 3 | 1,196 | 1,024 (capped) |
| 8 | 3,481 | 4 | 1,206 | 1,024 (capped) |
| 12 | 3,489 | 5 | 1,215 | 1,024 (capped) |
| 16 | 3,517 | 5 | 1,230 | 1,024 (capped) |
| 24 | 3,494 | 9 | 1,254 | 1,024 (capped) |
| 32 | 3,464 | 5 | 1,275 | 1,024 (capped) |

Read plainly, three things:

1. **The snapshot's size does not grow with the crowd.** `ents` is 1,024 — the cap — at every body
   count. What the crowd does is add to the **candidate** set (`cand` 1,185 → 1,275) and thereby
   displace the far end of the kept set. In this ship, adding people to a room costs *coverage of
   elsewhere*, not snapshot growth, because the snapshot was already full.
2. **The module's frame is ~2.0 ms with the layer idle and ~3.5 ms with the layer driving declared
   crew, and is flat from 1 to 32 bodies in this harness.** The step is the cost of the crew layer
   being active over spawned NPCs; the per-body slope over 1…32 is small (~5 µs/body, and within
   run-to-run noise). **This harness therefore does not demonstrate the per-NPC scaling of a real
   crowd** — see §What could not be measured. The design's complement (≈150) is **4.7× the
   harness ceiling of 32**.
3. **The layer's decisions are ≤9 µs/frame** (`layer_avg_us`), which matches G3's own measured
   figure (`docs/evidence/g3-reactive-crew-measured.md`). Driving the crew is not the frame cost;
   the bodies' own game AI is.

### A3 — what the bridge's snapshot costs the engine

`BuildSnapshot` (`sp_bridge.cpp:739`) walks every entity (`ge->num_entities`, 3,896 on the ship),
keeps the visible ones, and writes up to the cap. Its own clock, measured across A1/A2, is
**~0.55–0.85 ms per tick**, essentially independent of the crowd (the candidate scan grows by
~90 entities, which the cap then discards). Separately, the snapshot is a **struct** in the SP path,
not a packet: `sizeof(entityState_t)` is **232 bytes** (compiled from the module's headers), so the
bridge copies up to **1024 × 232 ≈ 232 KiB per tick** into its ring (`sp_bridge.cpp:786–800`), and
the cgame's `cgi_GetSnapshot` copies a whole `snapshot_t` (**238,180 bytes**) from the bridge into a
cgame slot on every new snapshot (`efgame/src/cgame/cg_snapshot.cpp:220`, called from
`CG_ReadNextSnapshot`). Neither is the wire; both are CPU and memory bandwidth. That is the split
the brief asks for: **the module's own frame ~2.0 ms (idle) / ~3.5 ms (crewed), the bridge's
snapshot build ~0.6 ms/tick, and the rest of the measured render frame software rendering.**

## The rule: how the engine decides what enters a snapshot

There are two snapshot builders in this tree, and they do not share a cap.

### Multiplayer — the engine's own server (`server/sv_snapshot.c`)

`SV_BuildClientSnapshot` (`sv_snapshot.c:509`) calls `SV_AddEntitiesVisibleFromPoint`
(`sv_snapshot.c:357`) from the client's eye. An entity is added when, in order:

- it is `r.linked` (`:388`);
- it is not `SVF_NOCLIENT` (`:398`);
- `SVF_SINGLECLIENT` / `SVF_NOTSINGLECLIENT` / `SVF_CLIENTMASK` admit this client (`:403`–`:420`);
- `SVF_BROADCAST` entities are added unconditionally (`:430`), bypassing every test below;
- its leaf area is `CM_AreasConnected` to the client's area — **the areaportal / closed-door test**
  (`:437`–`:443`);
- its cluster is set in `CM_ClusterPVS` of the client's cluster (`:445`–`:474`), with portal entities
  recursing from their own origin (`:480`–`:490`).

The list is then `qsort`ed and copied (`:566`, `:597`–`:610`). **There is no distance test and no
per-snapshot configuration cvar.** The rule is tunable only by entity flags the *game* sets
(`SVF_NOCLIENT`, `SVF_BROADCAST`, `SVF_PORTAL`) and by the compiled cap. When the cap is reached,
`SV_AddEntToSnapshot` returns early and **silently discards** the entity (`sv_snapshot.c:344`):
because the main loop runs in ascending entity number, and portals append out of order, what is
dropped is whatever the walk reached last — **not** the furthest or the least important.

### Single player — the bridge (`sp_bridge.cpp`, patch `0010`)

`BuildSnapshot` (`sp_bridge.cpp:739`) fills a `snapshot_t` handed to the cgame **in-process** (no
packet, no delta compression). Patch `0010` replaced "the first N in number order" with:

- include the local player (entity 0) always;
- skip entities not `inuse` or flagged `SVF_NOCLIENT`;
- **linked** entities whose centre is not in the player's PVS are dropped —
  `W_InPVSIgnorePortals` (`sp_bridge.cpp:326`, used at `:771`), which is cluster visibility
  **ignoring door / areaportal state** (the retail `SV_inPVSIgnorePortals`);
- if more than the cap remain, keep the **nearest** (`std::nth_element`, `:776`–`:784`), then restore
  ascending entity order.

There is **no distance threshold**; the "threshold" is the cap. Two consequences fall straight out
of the source and are visible in A1–A2:

- **Unlinked entities are never culled** — the PVS test is guarded by `e->linked`
  (`sp_bridge.cpp:771`), so an `inuse`, non-`NOCLIENT`, unlinked entity always enters the candidate
  set. On the ship, 3,896 entities exist and 2,043 are linked, so the unlinked remainder is a real
  population in the scan.
- **Where it falls over is when the PVS passes more than the cap.** Which is *already true on this
  ship* (A1): the cap binds, and the nearest-keep decides who is dropped. Adding a body to a room
  therefore removes a body elsewhere from the snapshot.

### `MAX_SNAPSHOT_ENTITIES 256` — the number the brief names

`MAX_SNAPSHOT_ENTITIES` is **256** (`qcommon/qcommon.h:142`), and it is the **multiplayer server's**
array (`snapshotEntityNumbers_t.snapshotEntities[MAX_SNAPSHOT_ENTITIES]`, `sv_snapshot.c:305`) and
the size of the server's snapshot-heap arithmetic
(`svs.numSnapshotEntities = sv_maxclients × PACKET_BACKUP × MAX_SNAPSHOT_ENTITIES`,
`sv_init.c:274`; `PACKET_BACKUP` 32, `qcommon.h:136`). **When it is exceeded: silent truncation at
`:344`** — the entity is skipped, no log, no error, no dropped client. Downstream, the client's own
copy is capped too: in the network client `cl_cgame.c:358` logs `"CL_GetSnapshot: truncated %i
entities to %i"` against the *engine's* `MAX_ENTITIES_IN_SNAPSHOT` (**256**,
`EFAndroid-SP/.../cgame/cg_public.h:32`). So multiplayer has 256 twice, server and client, and a
crowd past it is degraded silently.

**The single-player bridge does not use 256.** The bridge's `#include "../cgame/cg_public.h"`
resolves through the bridge's own include path to the **module's** header, where
`MAX_ENTITIES_IN_SNAPSHOT` is **1024** (`efgame/src/cgame/cg_public.h:17`; the build dependency for
the bridge object names this header, and A1–A2 measure the cap at 1024). This is a live hazard worth
recording: two headers of the same name carry 256 and 1024, and which one the bridge sees depends on
its include order. The two `.so`s must be built from one header or `snapshot_t` sizes disagree; the
module's header says so in its own comment.

### How far the ship is from 256 in the ordinary case

Against the **SP cap of 1024**, the ordinary loaded ship is **at** it: candidates 1,166–1,202,
written 1024, every tick (A1). Against the **multiplayer cap of 256**, the SP candidate count is
**~4.6× over** in the ordinary case. The honest caveat is in §What could not be measured: the SP
test is cluster PVS **ignoring areaportals**, while the multiplayer server additionally requires
`CM_AreasConnected` (`sv_snapshot.c:437`), which closed doors and sealed areas can cut; the
multiplayer count could be lower than the SP candidate count, and it was not measured. What *is*
measured is that the ship's whole-visibility population is several times any 256-entity snapshot.

## Distance-based model substitution: present, enabled, and render-side only

The id Tech 3 lineage's model LOD is here, in the renderer:

- `model_t.numLods` is loaded from the model's own LOD chain — `R_LoadMD3(..., mod->numLods, ...)`
  and `mod->numLods++` (`renderervk/tr_model.c:70`, `:79`), and `R_LoadMDR` copies the MDR's
  `numLODs` into `mod->numLods` (`tr_model.c:660`, `:664`);
- `R_ComputeLOD` (`renderervk/tr_mesh.c:166`) computes a projected-radius LOD from the entity's
  distance and selects a mesh LOD (`tr_animation.c:227`), scaled by **`r_lodscale`** (default **5**,
  `tr_init.c:1762`) and offset by **`r_lodbias`** (default **-2**, "Ultra", i.e. LOD transitions are
  delayed further into the distance, `tr_init.c:1640`).

**It is enabled by default**, and `r_lodbias` is user-tunable from "Ultra" (−2) to "Low" (2). **It
would not help a crowded room's snapshot**: LOD decides how many triangles a *model already in the
scene* is drawn with; the entity is still one `entityState_t` in the snapshot, still one
`modelindex` on the wire, regardless of which LOD the client renders. It reduces client **render**
cost, not the number of entities transmitted. (EF also has a separate effects LOD —
`FXF_NO_LOD`, `efgame/src/cgame/fx_primitive.cpp:1887` — for particle counts, likewise render-side.)
There is no server-side, snapshot-level model substitution in this tree.

## Every cvar governing snapshot rate, culling, or entity limits

Values are the source defaults. "What a change buys" is stated, not done.

| cvar / constant | value | file:line | what a change would buy |
|---|---|---|---|
| `MAX_SNAPSHOT_ENTITIES` | 256 | `qcommon/qcommon.h:142` | per-snapshot entity capacity in **multiplayer**; raise to carry a crowded room. Costs server heap (`sv_init.c:274`) and forces the client's `MAX_ENTITIES_IN_SNAPSHOT` to move too; raises packet-overflow risk (below) |
| `MAX_ENTITIES_IN_SNAPSHOT` (module, SP cap) | 1024 | `efgame/src/cgame/cg_public.h:17` | the SP bridge's cap, measured at 1024 (A1–A2). Lowering it shrinks the 238 KiB struct and the per-tick copy, at the cost of visible entities; it is already bound on the ship |
| `MAX_ENTITIES_IN_SNAPSHOT` (engine, network client) | 256 | `EFAndroid-SP/.../cgame/cg_public.h:32` | the client-side truncation; must move with the server constant |
| `sv_fps` | 20 | `server/sv_init.c:668` | server tick / snapshot grid; higher = more snapshots per second and more bandwidth, not more per snapshot |
| `snaps` (client userinfo) | unset → 50 ms | `server/sv_client.c:1516`–`:1538` | per-client snapshot interval, clamped to `sv_fps`; lower interval = more packets |
| `rate` (client userinfo) | 3000 (LAN forced 99999) | `server/sv_client.c:1493`–`:1506` | client byte-rate choke; the bandwidth ceiling, not the entity ceiling |
| `sv_maxRate` / `sv_minPing` / `sv_maxPing` | 0 / 0 / 0 | `server/sv_init.c:644`–`:647` | server-side rate and ping policy |
| `sv_lanForceRate` | 1 | `server/sv_init.c:693` | LAN clients are not rate-limited (`sv_client.c:1492`) |
| `sv_padPackets` | 0 | `server/sv_init.c:690` | debug padding; would only worsen bandwidth |
| `sv_timeout` | 200 (s) | `server/sv_init.c:672` | how long a silent client is held |
| `sv_maxclients` / `MAX_CLIENTS` | 8 default / **64** | `sv_init.c:641` / `q_shared.h:1160` | the design's ≈150 players needs `MAX_CLIENTS` raised and its dependents (client array, reliable buffer, snapshot heap); Track D's problem |
| `r_lodbias` | -2 (Ultra) | `renderervk/tr_init.c:1640` | render LOD; **render-side only** (above) |
| `r_lodscale` | 5 (cheat) | `renderervk/tr_init.c:1762` | render LOD scale; render-side only |
| `r_nocull` | 0 | `renderervk/tr_init.c:1768` | disables frustum culling (debug); must stay 0 |
| `r_drawentities` | 1 | `renderervk/tr_init.c:1766` | draws entities; debug |
| `SP_SV_FRAMEMSEC` | 50 | `sp_bridge.cpp:141` | the SP sim/snapshot grid, 20 Hz; hard-coded, not a cvar |
| `MAX_MSGLEN` | 16384 | `qcommon/qcommon.h:197` | the packet ceiling; `SV_SendClientSnapshot` sets `allowoverflow`, and **an overflowing snapshot is cleared and dropped** (`sv_snapshot.c:730`–`:733`), so past 16 KiB the client simply gets no update |

## The levers, each with its number and what it buys

Nothing below was implemented. Each is the owner's decision.

1. **The multiplayer cap is 256 and the snapshot drops silently past it.** `sv_snapshot.c:344`.
   *Buys:* raise `MAX_SNAPSHOT_ENTITIES` (`qcommon.h:142`) — a crowded MP room could carry more
   bodies. *Costs:* the server's snapshot heap is `sv_maxclients × 32 × N × sizeof(entityState_t)`
   (`sv_init.c:274`; `entityState_t` is 232 bytes), the client parse buffer
   `MAX_PARSE_ENTITIES = PACKET_BACKUP × N` (`client/client.h:89`), the engine cgame constant
   must move with it, and larger snapshots approach `MAX_MSGLEN` 16,384 where the packet is dropped
   (`sv_snapshot.c:730`). *Not the wire format:* end-of-entity is written as `MAX_GENTITIES-1` at
   `GENTITYNUM_BITS` (`sv_snapshot.c:112`), so the field width does not change.
2. **Multiplayer keeps by entity number; single player keeps by distance.** `sv_snapshot.c:344` and
   `:384` (ascending walk, silent drop) against `sp_bridge.cpp:776` (`nth_element`, nearest kept).
   *Buys:* a nearest-keep in `SV_BuildClientSnapshot`, the way `0010` did in SP, would make a full
   MP snapshot drop the far entities instead of the high-numbered ones. *Costs:* engine logic — a
   new patch and something to own, which the brief says not to pull.
3. **The SP snapshot's cost is its size, not its entity count.** The bridge copies up to
   1024 × 232 B ≈ **232 KiB per 50 ms tick** (`sp_bridge.cpp:786`), and `cgi_GetSnapshot` copies a
   **238,180-byte** `snapshot_t` per new snapshot into the cgame (`cg_snapshot.cpp:220`).
   *Buys:* passing a pointer instead of copying, or lowering the cap for non-ship maps, would cut
   roughly 10 MB/s of memcpy — CPU and memory bandwidth, not the wire.
4. **The SP selection never culls unlinked entities.** `sp_bridge.cpp:771`. *Buys:* skipping (or
   distance-capping) unlinked, non-`NOCLIENT` entities would shrink the candidate set. *But:* the
   snapshot is already at its cap (A1), so on this ship it would change *which* entities are kept,
   not how many — low value here, possibly real on maps with many unlinked props.
5. **The ship's snapshot population is dominated by visibility, and visibility is the merged map's
   vis data.** Candidates 1,166–1,202 at every point tried (A1, A2 probes). *Buys:* the ship's
   areaportals and door state are the lever in multiplayer — `CM_AreasConnected` (`sv_snapshot.c:437`)
   is what a closed door cuts; the SP path deliberately ignores it (`W_InPVSIgnorePortals`,
   `sp_bridge.cpp:326`). A map that seals more tightly has fewer candidates. *Costs:* content and
   compiler work (the vis-cluster ceiling is already 94–95%, `docs/evidence/engine-content-headroom.md`).
6. **The design's ≈150 bodies need `MAX_CLIENTS`, not a snapshot tweak.** `MAX_CLIENTS` 64,
   `q_shared.h:1160`; `sv_maxclients` default 8, `sv_init.c:641`. *Buys:* the complement in played
   characters. *Costs:* everything sized by `MAX_CLIENTS` and the snapshot heap; the owner's Track D.
7. **The snapshot rate is a bandwidth dial, not a crowd fix.** `sv_fps` 20 / `snaps` 50 ms
   (`sv_init.c:668`, `sv_client.c:1516`). *Buys:* halving the rate halves packets; it does not make
   one crowded snapshot fit.
8. **Model LOD is not a lever for this problem.** `r_lodbias` / `r_lodscale`
   (`tr_init.c:1640`, `:1762`). *Buys:* render cost at distance; nothing on the wire or in the
   snapshot.

## Judgement calls, named as calls

1. **The instrument is two `developer`-gated log lines, one in the engine and one in the module.**
   `patches/0019` adds the SP bridge's snapshot and render-frame lines; `module/crew/g_crew.cpp`
   adds a per-second module-frame line. Both print only with `developer` set (the default is unset),
   so retail behaviour, saves, and the layer with it unset are unchanged. The alternative — reading
   only `0010`/`0017` and quoting them — would not have produced the cap-is-already-spent finding
   or the candidate counts, which are the survey's point.
2. **`docs/evidence/`, not `docs/research/`.** It carries fresh runs (A1–A3), not a reading; the
   house form asks for the command before the output, which is above.
3. **The crowd is 32, the layer's ceiling, not 150.** `MAX_CREW` is 32
   (`module/crew/crew_core.h:22`). I did not raise it to manufacture a 150-body run: that would be a
   behaviour change beyond a measurement instrument, and the module's roster is not the same thing
   as a server full of players. The design's 150 is named as unmeasured (§below), not extrapolated
   into a claim.
4. **The module's flat 1→32 curve is reported as measured, not explained away.** The step from
   layer-idle (~2.0 ms) to layer-active (~3.5 ms) is real and reproducible; the per-body slope is
   small and within noise. I do not have a frame-level breakdown of *which* game subsystem the step
   is (NPC think vs. the layer's own per-member work), so I state the curve and stop.
5. **The SP cap being 1024, not 256, is the headline correction to the brief.** It comes from the
   bridge compiling against the module's `cg_public.h` (`efgame/src/cgame/cg_public.h:17`) — verified
   by the object's own dependency file and by the measured cap — not from the engine's 256 header.
   The two same-named headers carrying 256 and 1024 is recorded as a hazard, not fixed.
6. **The multi-deck probes are read as "every point tried", not "every point".** Five deck arrivals
   were probed; all saturated. I did not exhaust the ship's rooms.

## What could not be measured — named

- **A real 150-body room.** The harness's crew ceiling is 32; the design's complement is ≈150. The
  per-NPC scaling beyond 32 is **unmeasured**.
- **A real client, two machines, a full 64-player server.** Not reachable from a headless harness.
  The multiplayer snapshot's entity count and byte size on the wire were **not measured**; §The rule
  describes them from source, and §A's entity counts are the single-player, cluster-PVS numbers.
  Specifically, the **multiplayer candidate count** (which also tests `CM_AreasConnected`) is
  **unmeasured**, so the SP candidate count is an upper bound, not the MP figure.
- **Multiplayer snapshot bytes.** `MAX_MSGLEN` and the drop-on-overflow behaviour are read from
  source (`sv_snapshot.c:730`); no overflowing snapshot was produced or observed.
- **The engine's render cost on real hardware.** The render device here is software Vulkan
  (llvmpipe); the ~15 ms render frame is a software number and says nothing about a GPU. Only the
  module frame (~2.0/3.5 ms) and the snapshot build (~0.6 ms/tick) are CPU logic numbers.
- **The per-entity and per-candidate cost of the SP snapshot scan** as distinct from its total:
  `build_avg_us` is measured; a breakdown (PVS test vs. struct copy) is not.
- **Server CPU with a full NPC crew plus a full player complement**, the design's own open question
  (`docs/design-north-star.md` §8). Untested.

## Reproducing

- Build: `scripts/bootstrap-upstream.sh` (nineteen patches) then `cmake --build build-engine`;
  module: `cmake --build ../upstream/efgame/build-linux`.
- Ordinary run and crowd run: A1 and A2 above (the `+set developer 1` is what turns the instrument
  on; without it the runs behave exactly as before).
- `sizeof(entityState_t)` / `sizeof(snapshot_t)`:
  `g++ -std=c++17 -w -fcommon -include ../upstream/efgame/android/ef_android_compat.h
  -I ../upstream/efgame/android -I ../upstream/efgame/src/game -I ../upstream/efgame/src/cgame
  -I ../upstream/efgame/src/qcommon <probe.cpp>` → `232` and `238180`.
- The gates: `scripts/test.sh` and `scripts/check.sh` (below).
