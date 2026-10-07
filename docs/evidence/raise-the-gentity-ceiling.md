# Evidence — raising the gentity ceiling: `GENTITYNUM_BITS` 12 → 13

Date: 2026-10-07. Branch: `feat/raise-the-gentity-ceiling`, cut from `feature/g3-reactive-crew`.
Brief: the raise-the-gentity-ceiling brief. It follows from item 1 of *Headroom found, measured, and
deliberately not implemented* in `docs/evidence/engine-content-headroom.md` (the binding ceiling, 3896 of
4096), from the headroom item in `docs/prior-art-rpg-x.md`, and from `docs/engine-extension-policy.md`.
The precedent in the patch series is `0008` (`GENTITYNUM_BITS` 11 → 12); this change keeps that patch's
shape.

Nothing here is asserted without the command that produced it. The command is given before its output.

**The headline, stated before the numbers:** the ceiling is now **8,192**, and the merged fifteen-deck
ship that spent 3,896 of 4,096 (95%) now spends **3,896 of 8,192 (48%)**. Retail `borg1` falls from 14% to
7%. The raise was proven not to change what any mode we *ship* puts on the wire, because every mode we
ship either runs both ends from this one build (single player), or runs a separate application we do not
build (cMod), or does not exist yet (our own multiplayer).

## The change

`patches/0018-entities-to-8192.patch` raises `GENTITYNUM_BITS` 12 → 13 in **both copies** of `q_shared.h`
— the engine's (`EFAndroid-SP/app/jni/efcode/qcommon/`) and the module's (`efgame/src/game/`) — so
`MAX_GENTITIES`, which is `(1<<GENTITYNUM_BITS)`, follows from 4,096 to 8,192. The two constants were
found duplicated by the headroom measurement and must agree; both move together, as in `0004` and `0008`.

The precedent (`0008`) did three things: raised both trees, fixed the SP bridge tables that a literal 1024
had left behind, and enlarged the game module's memory pool. This time there is nothing else to fix: the
bridge tables now derive their size from `MAX_GENTITIES` (`SP_MAX_ENT` and the `g_sOverride` / `g_voiceEnd`
/ `s_cand` / `s_dist` / `order` arrays, all introduced or corrected by `0008` and `0010`), so they widen
with the constant. There is no literal entity-table size left in the bridge:

```
$ grep -n "4096\|2048\|MAX_GENTITIES" ../upstream/EFAndroid-SP/app/jni/sp/sp_bridge.cpp
103:#define SP_MAX_ENT MAX_GENTITIES
113:#define CS_MAX 1024
503:// Sized to MAX_GENTITIES (1024): ...            (comment; the array itself is MAX_GENTITIES)
506:static int   g_sOverride[MAX_GENTITIES];
513:static int   g_voiceEnd[MAX_GENTITIES];
523:    for(int i=0;i<MAX_GENTITIES;i++){
740:    static int   s_cand[MAX_GENTITIES];
741:    static float s_dist[MAX_GENTITIES];
760:        static int order[MAX_GENTITIES];
```

(The `CS_MAX 1024` on line 113 is the configstring table, a different limit — see *Adjacent limits*.)

Every `GENTITYNUM_BITS` **definition** in the checkout, after the patch — there are exactly two, and they
agree:

```
$ grep -rn "define[[:space:]]*GENTITYNUM_BITS" ../upstream/
.../efcode/qcommon/q_shared.h:1163:#define GENTITYNUM_BITS  13  // raised again: the ship is at 3896 of 4096; a fifth deck adds 254..294
.../efgame/src/game/q_shared.h:878:#define GENTITYNUM_BITS  13  // raised again: the ship is at 3896 of 4096; a fifth deck adds 254..294
```

## Build, from the pinned upstream

The patch series is the deliverable, so it was re-applied to the pinned upstream from scratch and both
trees rebuilt. `bootstrap-upstream.sh` resets the checkout, applies `0001`…`0018` in order, and rebuilds
the module and the two script tools; the engine is then rebuilt over the module.

```
$ ./scripts/bootstrap-upstream.sh
==> upstream: https://github.com/imjustadudegamer/VoyagerSP-Android.git @ 0d8942e86a8469859da086f590875bcb66b4f4df
==> applying 18 patch(es)
    0001-build-under-a-modern-toolchain.patch
    ...
    0018-entities-to-8192.patch
...
==> checking exports
  libefgame.so : _Z10GetGameAPIP13game_import_t@@EFGAME_1.0 _Z6vmMainiiiiiiiii@@EFGAME_1.0 _Z8dllEntryPFiizE@@EFGAME_1.0
  libefui.so   : _Z8GetUIAPIv@@EFUI_1.0
  ok: the intended entry points and nothing else

$ cmake --build build-engine -j12
...
[100%] Linking CXX executable longwayhome
[100%] Built target longwayhome
```

The module exports only its three entry points, so the 18th patch did not disturb the module boundary
(engine-extension-policy rule 1).

## The instrument, re-run: the new ceiling and the ship's usage

The instrument is `patches/0017`, unchanged: it logs (only under `developer`) the loaded level's own
configstring registration and entity count, read from the bridge's table and the game's entity counter
after the game has spawned its entities. Reproduce any run below with `+set developer 1` and grep
`SP_SpawnServer: content:`.

### A1 — a loaded retail campaign map (`borg1`)

```
$ find /tmp/opencode/lwh-home/baseEF -name '*.pid' -delete
$ SDL_AUDIODRIVER=dummy timeout 90 xvfb-run -a scripts/run-engine.sh --home-dir /tmp/opencode/lwh-home \
      +set developer 1 +set s_useOpenAL 0 +set s_initsound 0 +map borg1
EFSP: SP_SpawnServer: CM_LoadMap(maps/borg1.bsp)
EFSP: SP_SpawnServer: 158 inline models, entity string 115246 bytes
EFSP: SP_SpawnServer: ge->Init done. num_entities=564 linked=371
EFSP: SP_SpawnServer: content: 104 configstrings of 4096 (bridge table CS_MAX 1024): 46 models of 256, 53 sounds of 256; gentities 564 of 8192
EFSP: Munro connected
```

### A2 — the merged fifteen-deck ship (`voyager`)

```
$ cp build/ship/out/longway_voyager.pk3 /tmp/opencode/ship-home/baseEF/
$ find /tmp/opencode/ship-home/baseEF -name '*.pid' -delete
$ SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a scripts/run-engine.sh --home-dir /tmp/opencode/ship-home \
      +set com_hunkMegs 768 +set developer 1 +set s_useOpenAL 0 +set s_initsound 0 +map voyager
EFSP: SP_SpawnServer: CM_LoadMap(maps/voyager.bsp)
EFSP: SP_SpawnServer: 531 inline models, entity string 577264 bytes
EFSP: SP_SpawnServer: ge->Init done. num_entities=3896 linked=2043
EFSP: SP_SpawnServer: content: 213 configstrings of 4096 (bridge table CS_MAX 1024): 141 models of 256, 67 sounds of 256; gentities 3896 of 8192
EFSP: Munro connected
```

Read plainly: the ship's own entity count is unchanged (3,896 — the raise adds capacity, not entities) and
the ceiling it is measured against is now **8,192**. The same number the headroom document reported as 95%
is now 48%.

| limit | retail `borg1` | the merged ship | ceiling before | ceiling now |
|---|---|---|---|---|
| configstrings | 104 | 213 | 4096 | 4096 (untouched) |
| models | 46 | 141 | 256 | 256 (untouched) |
| sounds | 53 | 67 | 256 | 256 (untouched) |
| gentities | 564 | **3896** | 4096 (95%) | **8192 (48%)** |

### A3 — the module's memory pool still holds the wider array

`0008` had to enlarge the game module's fixed bump pool when the entity array grew; the array grows again
here (4,096 → 8,192 gentities at 1,320 bytes each). The pool was not changed, and a `g_debugalloc` run
shows why it need not be:

```
$ ... +set g_debugalloc 1 +map voyager
EFSP: G_Alloc of 10813440 bytes (14352384 left)     # the entity array: 8192 * 1320
```

The smallest free headroom seen during the whole run was **13,770,048 bytes** with `POOLSIZE` at
24 MiB — i.e. peak usage ~11 MiB of 24 MiB — and no `G_Alloc: failed` was printed. `POOLSIZE` stays at
`(24 * 1024 * 1024)`. (The array was 5.4 MB at 4,096; it is now 10.3 MB. `0008`'s 24 MiB had the room.)

## The protocol question — established, not assumed

`GENTITYNUM_BITS` rides the wire: the network field table and the snapshot encoder both take their width
from it. This is a protocol-width change with the same class of consequence as the `MAX_MODELS` question
the headroom document left alone. The question is whether **any mode we ship** pairs a build that uses
this width with a build that does not.

### Every wire use derives its width from the constant

```
$ grep -rn "GENTITYNUM_BITS" ../upstream/EFAndroid-SP/app/jni/efcode/qcommon/msg.c \
      ../upstream/EFAndroid-SP/app/jni/efcode/server/sv_snapshot.c \
      ../upstream/EFAndroid-SP/app/jni/efcode/client/cl_parse.c
msg.c:1049:{ NETF(otherEntityNum), GENTITYNUM_BITS },
msg.c:1050:{ NETF(otherEntityNum2), GENTITYNUM_BITS },
msg.c:1051:{ NETF(groundEntityNum), GENTITYNUM_BITS },
msg.c:1087:{ NETF(otherEntityNum), GENTITYNUM_BITS },
msg.c:1099:{ NETF(otherEntityNum2), GENTITYNUM_BITS },
msg.c:1166:  MSG_WriteBits( msg, from->number, GENTITYNUM_BITS );
msg.c:1207:  MSG_WriteBits( msg, to->number, GENTITYNUM_BITS );
msg.c:1229:  MSG_WriteBits( msg, to->number, GENTITYNUM_BITS );
msg.c:1535:{ PSF(groundEntityNum), GENTITYNUM_BITS },
msg.c:1590:{ PSF(groundEntityNum), GENTITYNUM_BITS },
msg.c:1616:{ PSF(jumppad_ent), GENTITYNUM_BITS },
sv_snapshot.c:112:  MSG_WriteBits( msg, (MAX_GENTITIES-1), GENTITYNUM_BITS );
sv_snapshot.c:271:  MSG_WriteBits( msg, (MAX_GENTITIES-1), GENTITYNUM_BITS );
cl_parse.c:116:   newnum = MSG_ReadBits( msg, GENTITYNUM_BITS );
cl_parse.c:521:   newnum = MSG_ReadBits( msg, GENTITYNUM_BITS );
```

There is no literal entity-number width on the wire: reader and writer both take `GENTITYNUM_BITS`. So a
client built with the same header as its server is consistent by construction, and a client built with a
different header is not.

The **save** format, by contrast, is width-independent: the entity-number fields it transcribes are plain
`int`s, not bitfields —

```
$ grep -n "otherEntityNum\|groundEntityNum\|jumppad_ent\|int.*number;" ../upstream/EFAndroid-SP/app/jni/efcode/qcommon/q_shared.h
1382:  int  number;          // entity index
1398:  int  otherEntityNum;  // shotgun sources, etc
1399:  int  otherEntityNum2;
1401:  int  groundEntityNum; // ENTITYNUM_NONE = in air
```

— and `gentitySize` is unchanged at `1320`, so `g_savetranscode.cpp`'s native-size assert still fires
nothing. This is why the save test in section C round-trips.

### Mode 1 — retail single player. Both ends are this one build. **Verified.**

Our engine and our module both come from this repository's patched trees, compiled from the same header.
The SP path hosts the module in-process and hands `snapshot_t` structures across `CG_GETSNAPSHOT` rather
than serialising a packet, so entity numbers do not cross a machine at all; where any encoder is used, the
server and client are the same binary. Evidence: the two runs in A1/A2 — retail data, our engine, our
module, `CM_LoadMap` and `Munro connected`, with no truncation or mismatch. A 13-bit stream read by a
13-bit reader cannot disagree with itself.

### Mode 2 — retail multiplayer, cMod as shipped. **Untouched; confirmed, and booted.**

Mode 2 is cMod: a separate prebuilt application. Our patch series touches two headers in the SP engine and
the SP module; it does not touch cMod, which is consumed as a binary and not built here.

```
$ grep -E "^(\+\+\+|---) " patches/0018-entities-to-8192.patch
--- a/EFAndroid-SP/app/jni/efcode/qcommon/q_shared.h
+++ b/EFAndroid-SP/app/jni/efcode/qcommon/q_shared.h
--- a/efgame/src/game/q_shared.h
+++ b/efgame/src/game/q_shared.h

$ grep -rln "cmod\|cMod\|CMOD" scripts/
scripts/run-cmod.sh                      # the only script that names it; it launches, never builds
$ file build/cmod/cMod-stvoyHM build/cmod/cMod-dedicated
build/cmod/cMod-stvoyHM:   ELF 64-bit ... (built 2025-12-18/19, from the maintainers' release)
build/cmod/cMod-dedicated: ELF 64-bit ...
$ ldd build/cmod/cMod-stvoyHM | grep -iE "efgame|longway" || echo "no reference to our engine or module"
no reference to our engine or module
```

And it still runs, with its own protocol, unaffected by this tree:

```
$ SDL_AUDIODRIVER=dummy timeout 25 xvfb-run -a scripts/run-cmod.sh --dedicated +set net_port 27999 +map hm_borg1
cMod HM v1.30_GIT_4341b89-2025-12-18 linux-x86_64 Dec 19 2025
Loading vm file vm/qagame.qvm...
VM file qagame compiled to 2152485 bytes of code
InitGame: ...\mapname\hm_borg1\protocol\24\com_protocol\26\com_gamename\EliteForce...
AAS initialized.
```

cMod carries `protocol 24 / com_protocol 26` and the retail `qagame.qvm`; it never loads our engine or
module, so a change to our `GENTITYNUM_BITS` cannot reach it.

### Mode 3 — our own multiplayer. **Does not exist as a build.** 

Mode 3's multiplayer is Track D, "the programme as chartered" (`docs/client-modes.md`): it is not built,
so there is no mode-3 artifact that this change could affect. When it is built it is "ours on both ends"
by construction and inherits this same guarantee — a server and a client compiled from the same header.
This is stated as a fact about what is shipped, not a claim about a mode that exists.

### The case that would break it — not reachable in any shipped mode

The failure the raise must not cause is an **unmodified Elite Force client talking to our engine, or our
engine talking to an unmodified client**. No shipped mode creates that pairing:

- mode 1 has no second machine: the client and server are one process from one header;
- mode 2 does not use our engine at all — it runs cMod, which we do not build;
- mode 3 is not built.

The engine binary nonetheless contains the full id Tech server/client code (every client does), and it
carries a Holomatch route that the SP path bypasses. That route is **not** a mode we ship (`docs/gates.md`: mode 2
is cMod, not our engine), and running our engine against an unmodified client would be a protocol fork
regardless of this patch — exactly the boundary `docs/client-modes.md` records. It is named here so the
day anyone considers pointing an unmodified client at our engine, the fork is known. **No stop condition
was met.**

## C — a save round-trips

Session 1 loads `borg1` and the engine writes its level-entry autosave in the retail-format chunk writer;
session 2 loads that save back.

```
$ find /tmp/opencode/gentity-home/baseEF -name '*.pid' -delete
$ ... --home-dir /tmp/opencode/gentity-home +set developer 1 ... +map borg1
EFSP: save: wrote saves/auto.sav (map borg1, t=1000, eAUTO)
EFSP: SP_WriteEntryAutosave: wrote eAUTO level-entry autosave for borg1
EFSP: Munro connected

$ find /tmp/opencode/gentity-home/baseEF -name '*.pid' -delete
$ ... --home-dir /tmp/opencode/gentity-home +set developer 1 ... +load auto
EFSP: load: saves/auto.sav -> map borg1 t=1000 (deferred reload)
EFSP: SP_LoadGame: ge=0x7311be9543a0 apiversion=6 gentitySize=1320
EFSP: SP_FinishTransition: eAUTO -> reset g_levelTime 1000 -> 1000 (retail SV_SpawnServer parity)
EFSP: SP_FinishTransition: eAUTO autosave load -> ReadLevel(qtrue), entities respawn fresh
EFSP: SP_FinishTransition: done numEnt=564 ps.origin=(104 -936 5)
```

The save written by this 13-bit build reads back, the level returns to its spawn (`ps.origin` matches the
fresh spawn), and `gentitySize=1320` is unchanged. (`0008`'s note stands: entity numbers are persisted as
ints, so the save format itself did not change width here; see *What could not be verified* for the
cross-build case.)

## D — `scripts/build-ship.sh` still produces a valid BSP

The compiler does not read the engine constant (the BSP format is fixed), but the artifact is checked
anyway, and from the **packed pk3**, not the build directory:

```
$ ./scripts/build-ship.sh
15 decks -> build/ship/voyager.map
  brush models 530 (engine limit 256): {...}
  triggers turned from brush models into boxes: 648
/tmp/tmp.XXXX/maps/voyager.bsp   OK   (1/17 lumps empty)
wrote build/ship/out/longway_voyager.pk3        # exit 0

$ unzip -o -j build/ship/out/longway_voyager.pk3 '*.bsp' -d /tmp/opencode/gt-pk3
$ python3 tools/mapgen/check-bsp.py /tmp/opencode/gt-pk3/voyager.bsp
/tmp/opencode/gt-pk3/voyager.bsp   OK   (1/17 lumps empty)
```

The freshly built BSP then loads through the raised engine (A2's run above used this pk3), so the artifact
is verified, not merely the exit code.

## E — `scripts/test.sh` and `scripts/check.sh` exit 0

```
$ ./scripts/test.sh
==> patches: numbered without gaps, and each one parses
    18 patches
all checks passed                                       # exit 0

$ ./scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
parsed 318 entity classes from SP_entities.def, HM_entities-def.txt, lwh_entities.def
...
ALL PASS
==> compiler corpus pass (every shipped script: compile, then read back)
    2024 files: 2016 compiled and read back, 8 rejected by the compiler, 0 read-back failure(s)
      the 8 rejections are exactly the known, documented set   # exit 0
```

(The two entity `.def` files are the GDK's; they are copied into the upstream checkout as
`bootstrap-upstream.sh` instructs, and are never committed. Without them the dictionary check sees only
our one adopted class and the fixture fails — that is an environment precondition, not a result of this
change.)

## The checks, as PASS lines

```
PASS  GENTITYNUM_BITS is 13 in both q_shared.h copies and nowhere else; MAX_GENTITIES follows to 8192
      (grep "define GENTITYNUM_BITS" ../upstream/: two definitions, both 13)
PASS  the patch series applies from the pinned upstream and both trees build clean
      (bootstrap-upstream.sh: "applying 18 patch(es)", exports "ok: the intended entry points and nothing else")
PASS  the loaded retail campaign map loads and plays on the raised engine
      (… +map borg1: CM_LoadMap(maps/borg1.bsp), num_entities=564; "Munro connected")
PASS  the merged fifteen-deck ship loads and spawns the player on the raised engine
      (… +map voyager: CM_LoadMap(maps/voyager.bsp), 531 inline models, num_entities=3896; "Munro connected")
PASS  the instrument reports the new ceiling and the unchanged usage, read from the level's own state
      (content line: "gentities 564 of 8192" / "gentities 3896 of 8192")
PASS  the module memory pool holds the wider array without a failed allocation
      (g_debugalloc: "G_Alloc of 10813440 bytes (14352384 left)", smallest headroom 13770048 of 25165824)
PASS  every entity-number wire use derives its width from the constant
      (grep msg.c / sv_snapshot.c / cl_parse.c: reader, writer and field table all GENTITYNUM_BITS)
PASS  mode 2 is a separate prebuilt application this tree does not build, and it still boots
      (ldd: no reference to our engine or module; run-cmod.sh --dedicated: cMod v1.30, qagame.qvm, hm_borg1, AAS initialized)
PASS  a save written by this build round-trips through a load
      (… +load auto: "load: saves/auto.sav -> map borg1", "done numEnt=564 ps.origin=(104 -936 5)")
PASS  scripts/build-ship.sh produces a valid BSP, verified from the packed artifact
      (check-bsp.py OK on the .bsp extracted from longway_voyager.pk3; then loaded by the engine)
PASS  scripts/test.sh exits 0
PASS  scripts/check.sh exits 0 (2016 of 2024 scripts compile and read back, the 8 known rejections)
```

## What could not be verified

- **No two-machine multiplayer test was run.** Mode 2's client half, and any mode-3 pairing, need two
  machines and a session (G7's bar). What was verified for mode 2 is narrower and is stated as such: cMod
  is a separate prebuilt binary this tree does not build, it does not load our engine or module, and its
  dedicated server still boots and loads `qagame.qvm`/`hm_borg1`.
- **Cross-build save compatibility.** A save written by the *previous* (12-bit) build was not loaded under
  this one, and vice versa. The save's entity-number fields are plain `int`s and `gentitySize` is
  unchanged, so the save format itself is width-independent — but the wire is not, and the safe statement
  is the one tested: a save written by *this* build round-trips. `0008`'s caution about saves from before
  a raise is inherited, not re-tested.
- **Deck 6's actual entity increase.** The +254…294-per-re-dress figure is the four kept re-dresses; deck 6
  is still the 81-entity placeholder, so the projection that motivated the raise is from the measured
  pattern, not from deck 6 built.
- **The full-ship frame cost.** No new frame measurement was taken; `scripts/s3-check.sh`'s frame
  measurement was not re-run, so the cost of the wider array and tables in the frame is not re-measured
  here.
- **A played mission.** "borg1 loads, spawns the player, and a save returns to it" is what the transcripts
  show; that a mission is completable is G1's own open item.

## Headroom found, measured, and deliberately not implemented

The standing instruction — "if there's any other way to get more performance out of the engine as we go
along, do it" — asks for further headroom **with its measured number**. Nothing below was implemented.

1. **The vis-cluster cap is a q3map2 constant, not an engine one.** `MAX_MAP_VISCLUSTERS` is `0x4000`
  (`q3map2.h`), and the merged ship is at **15,355 of 16,384 (94%)**, read from BSP lump 16 of the packed
  artifact:

  ```
  $ python3 -c "..."   # lump 16 header from the extracted voyager.bsp
  visdata bytes 29481608 portalclusters 15355 clusterbytes 1920   (MAX_MAP_VISCLUSTERS 16384)
  ```

  This raise does not touch that arithmetic — it is the entity ceiling, a different axis. The binding
  content limit for the *next* full-detail deck is still this one, and the fix is a q3map2 rebuild (its
  source is in the pin), which the brief puts outside scope.

2. **The SP configstring table is still the wrong 1024.** `sp_bridge.cpp:113` hard-codes `CS_MAX 1024`
  while `MAX_CONFIGSTRINGS` is 4,096. This raise does not touch it (it is the configstring axis, and the
  ceiling there is `MAX_MODELS + MAX_SOUNDS`), but it remains the thing to widen if models or sounds ever
  move; the bridge would silently drop the new configstrings otherwise. Named, not changed.

3. **`MAX_MODELS` / `MAX_SOUNDS` remain at 256** (141 and 67 in the merged ship). They ride the network as
  8 bits (`NETF(modelindex)`), so raising them is a protocol change; there is no measured pressure yet.
  Unchanged, as the headroom document left them.

4. **The snapshot's per-frame entity capacity.** `patches/0010` already chooses snapshot entities by
  visibility and distance rather than the first `MAX_ENTITIES_IN_SNAPSHOT`. The wider ceiling does not by
  itself make a snapshot carry more entities; a denser deck still spends per-frame cost. Not re-measured.

## Judgement calls, named as calls

1. **The change is two lines in the patch series, not a rebuilt fork of anything else.** It is a
   compile-time constant, so by construction it cannot be cvar-gated; the protection for retail is that
   mode 1 compiles both ends from this header and mode 2 is a different application. Stated as what
   protects retail rather than as a gate, per `docs/engine-extension-policy.md` rule 3 and the brief.
2. **`POOLSIZE` was left at 24 MiB.** `0008` raised it alongside the 11→12 raise; the measurement here
   (A3) shows ~11 MiB peak, so raising it again would be a change without a number behind it. If a denser
   deck later allocates more strings, the `g_debugalloc` line is the check.
3. **No version or format bump was added for the save.** The save's entity fields are `int`s and
   `gentitySize` is unchanged (section C), so nothing in the format moved; `0008` did not bump one either.
   The honest caveat is in *What could not be verified*.
4. **The dated document `docs/evidence/engine-content-headroom.md` is left as written** — it is the record
   of what was true when the raise was *not* implemented, and item 1 of its headroom section is precisely
   what this branch acted on. The live ledger (`docs/gates.md`) and the patch accounting
   (`CONTRIBUTING.md`) are updated to match the new ceiling.
