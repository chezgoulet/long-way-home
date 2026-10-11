# The posts, the floor, and the four checks — the evidence

This pass closes the reason the four world-effect checks could not assert their frames
(`docs/evidence/the-frame-assertions.md`): the crew layer's out-of-world floor was ship-wide, and a
member's "own deck's posts" were never established. It also changes what the four checks photograph:
their camera now stands at the thing the claim is about. This file is the evidence — **what was
observed and the command that produced it.**

Everything below is the output of the command named beside it, on `feat/the-posts-and-floor`, cut
from `testing@06796a7`, on the playtest host (`sasquatch`), with the module rebuilt from this tree
before any engine check ran:

```
$ cmake --build ../upstream/efgame/build-linux -j"$(nproc)"
[100%] Built target efgame
```

## The finding, measured — and a correction to the brief's mechanism

The brief says the merged ship's `.crew` "carries 24 posts and 32 members, every one at the same
single coordinate", read by crewgen. **On this tree that is not what the module reads.** With
`g_crewFromShip 1` the layer does not read a `.crew` file at all — `module/crew/g_crew.cpp:1661`:

```c
	if ( g_crewFromShip->integer ) cs.cfg = crew::Config();   // the ship's roster replaces it
	else LoadConfig();
```

So the stub in `build/g3-home/baseEF/maps/voyager.crew` is never opened by the four checks, and the
false "fell out of the world" it produces has a different cause: **`cs.floorZ` defaulted to 0 when
there are no posts**, and was the ship's lowest post when there were. Measured before the change, on
`map voyager` with the crew layer on:

```
$ scripts/damage-check.sh            # before this pass
...
$ grep -c 'fell out of the world' build/g3-home/damage.out
17
$ grep -E 'post\(s\)|no posts' build/g3-home/damage.out
CREW: 0 post(s), up to 24 crew
CREW: no posts: the map has no waypoint_navgoal and the scenario authored none
```

Seventeen members — the crew embodied on deck 12 and the map's own NPCs below z = −2048 — were
judged against a floor that did not exist (0). **The brief's shape is right and its mechanism is
wrong**: recorded here rather than smoothed over.

## Task A — the floor is per deck, and says so when it has none

`module/crew/crew_core.{h,cpp}` gained one rule, `PostFloors`: the lowest post standing on each deck,
and `known == false` for a deck with no post. `module/crew/g_crew.cpp` uses it instead of the one
`floorZ`: the member's deck is the deck its post stands on (else the deck its body is on), and a
member is out of the world only below **its own deck's floor**. `ComputeFloors` runs whenever the
post table changes and says once per deck that has no posts. The unit test proves both directions and
the no-posts case on synthetic posts:

```
$ cmake -S tests/crew -B /tmp/crew >/dev/null && cmake --build /tmp/crew -j4 >/dev/null && /tmp/crew/test_crew_core
crew_core: all checks passed
```

The engine demonstration is `scripts/crew-floor-check.sh` (`g_shipTest 88`): one run on the merged
ship, standing on deck 12 where the crew are far below the ship's lowest post, then dropping one
member 3000 below its deck.

```
$ scripts/crew-floor-check.sh
==> crew floor
SHIP: floor test: standing at (-3776 -3776 -37646)
SHIP: floor test: on deck 12; before the drop no member is out of the world
SHIP: floor test: dropped lwh_crew_015 to (-2813 -3136 -40763), 3000 below its deck
^1CREW: lwh_crew_015 fell out of the world at (-2813 -3136 -40763) (deck 12, floor -37772)
PASS  no member on deck 12 was fallen while it stood (floor -37772); dropped to z -40763, lwh_crew_015 was caught
PASS  the crew layer's floor is the member's own deck's, and it fires only below that
```

**Both directions in one log**: nothing is reported while the deck-12 crew stand 25 000 units below
the ship's lowest post, and the one member dropped 3000 below *its own deck's* floor (−37772) is
caught, with the deck and the floor named in the line.

**A deck with no posts** is reported once, not floored at zero. On the merged ship every one of the
fifteen decks carries navigation goals, so the rule does not fire there; it is exercised on a map
with no posts at all (`map lwhroom`, from the `testroom-watch` scenario's own map):

```
$ scripts/run-engine.sh --home-dir build/g3-home +set g_ship 0 +set g_crew 1 +set g_crewFromShip 1 +map lwhroom
...
CREW: direction layer active on lwhroom
^3CREW: deck 1 has no posts; a member there is below no floor
CREW: 0 post(s), up to 24 crew
```

The line is emitted at load, once per deck, from `ComputeFloors`; a member never reaches the
comparison (`known == false`), so nothing under it is judged.

After the change, the same damage run:

```
$ grep -c 'fell out of the world' build/g3-home/damage.out
0
```

## Task B — the merged ship's posts are real places

**The choice: the crew layer accepts the classes the maps actually carry.** The maps name their
navigation goals `waypoint_navgoal_<n>` (123 `waypoint_navgoal_2`, and 271 in total on the merged
ship across the fifteen decks); the layer looked only for a class named exactly `navgoal`
(`g_crew.cpp:1209`, `:1249`), so on a merged map the two never met. `IsNavgoalClass` now accepts the
prefix, and `FindNavgoal` and the fallback both use it. **Why this and not a build step:** it is game
logic, so it lives in `module/`; it needs no change to `stitch.py` or the pak; and it works for any
map the client loads, the published decks included.

Under `g_crewFromShip` the merged ship's post set is then **derived per deck** from those goals
(`SeedShipPosts`), taken round-robin across decks so a cap never starves one, and stopped short of
`MAX_POSTS` so the ship's roster still has room for the places it brings. Nothing is hand-authored:
the stub is not read, and a `.crew` edited by hand is not what the ship uses.

```
$ scripts/run-engine.sh ... +set g_crewFromShip 1 +set g_shipDeckPitch 3072 +set g_shipTest 34 +map voyager
CREW: 216 post(s) from the map's own navigation goals, on 15 deck(s)
CREW: 216 post(s), up to 24 crew
```

The crew on deck 12 at distinct posts, each a real place on the deck (`g_crewDebug 1`, run above):

```
$ grep -E 'lwh_crew_(015|058|061|064) \(ent' build/g3-home/posts5.out
CREW: lwh_crew_015 (ent 80) -> place_lwh_crew_015 at (-3776 -3776 -37670)
CREW: lwh_crew_058 (ent 3211) -> place_lwh_crew_058 at (-2816 -3264 -37664)
CREW: lwh_crew_061 (ent 3217) -> place_lwh_crew_061 at (-3200 -3904 -37670)
CREW: lwh_crew_064 (ent 3222) -> place_lwh_crew_064 at (-3712 -2880 -37763)
```

Four people, four places, on the deck they are on — not one point. (These are the ship's own places,
`place_lwh_crew_NNN`, standing on the deck's navigation; the seeded goals are the deck's floors and
the places a `.crew` can name by `navgoal`.)

**The published decks are unchanged.** `scripts/s5-check.sh` passes (it is the one that runs
`g_crewFromShip` on a published deck):

```
$ scripts/s5-check.sh
...
PASS  through 4 meals and 4 spells between them, the crew embodied were the crew the ship had aboard
INFO  of 24 embodied, 24 reached their place within the meal hour and 0 gave up
PASS  most of the crew embodied walked to their place on the deck
```

The **G3 measurement** (the charter's ten-minute run, `scripts/g3-measure.sh`, default 600 s) also
passes on this tree:

```
$ scripts/g3-measure.sh
CREW: coverage min 100% last 100% over 121 sample(s)
PASS          zero navigation failures        0 stuck, 0 out of world
PASS          save/load restores posts       6 crew compared field by field
G3 measured run: PASS
```

## Task C — the four checks, before and after

The camera now stands at the claim's subject, via one helper (`StandWhereVisible` /
`StandWhereVisibleAt`): the player's own spot if it can see the thing, else a ring of vantages around
it, taking the first with a clear line and a floor, and holding the aim across both frames. The
before-frame is captured at the same vantage where the claim needs a difference. Measured with the
tool already in the tree (`tools/frames/frame.py`).

### The worked counter-example — a whole-frame difference is a green stamp

The brief's measured trap, re-measured here. The **old** damage pair (the corridor wall the earlier
pass brought out to `build/frame-assertions/`), at the tool's own default:

```
$ python3 tools/frames/frame.py diff build/frame-assertions/damage_before.tga build/frame-assertions/damage_after.tga
damage_after.tga   whole frame   changed   6.2% mean|d|   6.1  DIFFERS
$ python3 tools/frames/frame.py diff --band 0.10,0.55 --min-changed 0.05 ...
damage_after.tga   band 0.1-0.55 changed   0.3% mean|d|   0.4  SAME
$ python3 tools/frames/frame.py diff --region subject:615,525,665,585 --min-changed 0.005 ...
damage_after.tga   subject       changed   0.0% mean|d|   0.0  SAME
```

**A whole-frame difference at the default passes on the wall** — 6.2%, because the console text and
the weapon's animated screen moved — while the part of the frame the claim owns changed 0.0%. The
change nobody claimed is the green stamp. The **new** pair shows the same trap and the way out:

```
$ python3 tools/frames/frame.py diff build/g3-home/baseEF/screenshots/lwh_damage_before.tga .../lwh_damage.tga
lwh_damage.tga   whole frame   changed   6.2% mean|d|   6.9  DIFFERS     (still the HUD/console)
$ ... --band 0.10,0.55 --min-changed 0.05 ...
lwh_damage.tga   band 0.1-0.55 changed   2.0% mean|d|   2.0  SAME
$ ... --region subject:615,525,665,585 --min-changed 0.005 ...
lwh_damage.tga   subject       changed   0.7% mean|d|   0.8  DIFFERS    (the spark)
```

**The threshold is `--min-changed 0.005`, on a 50×60 region the camera aims at.** The HUD and the
console text cannot reach that region, so they cannot satisfy it; the spark does (0.7% = 21 changed
pixels), and the same region on the old wall pair is 0.0%, so the assertion fails where it must.

### Each check

| check | what the frame now is | the assertion added | before | after |
|---|---|---|---|---|
| `damage` | at the life-support station, before and after the damage | difference, region `subject:615,525,665,585`, `--min-changed 0.005` | old region **0.0%** (fails) | **0.7%** (passes) |
| `emergency` | at the nearest authored strip to the room's watch post, red and stood down | difference, region `strip:480,510,820,640`, `--min-changed 0.05` | old band **0.0%** (fails) | **25.3%** (passes) |
| `fire` | at the deck's first fire | **behavioural** | — | subject region 0.2–0.9%, which is the crew walking |
| `breach` | at the authored field | **behavioural** | — | subject region 0.0–0.1%; the compartment is near-black |

```
$ scripts/damage-check.sh
SHIP: damage test: standing at (-3531 -3184 -37705), looking at lwh_station_0 at (-3776 -3776 -37646)
SHIP: damage test: life support at 60% health
CREW: life support is damaged; sparks at (-3776 -3776 -37670) on deck 12
lwh_damage.tga   subject   changed   0.7% mean|d|   0.8 of 3000 px  DIFFERS
PASS  a damaged system sparks where it is worked, in the world

$ scripts/emergency-check.sh
lwh_emergency.tga   strip   changed  25.3% mean|d|  30.6 of 44200 px  DIFFERS
PASS  the emergency lighting state comes on at battle stations and stands down on all four decks; the gate holds

$ scripts/fire-check.sh
PASS  a burning deck is embodied as smoke and flame across the deck
      screenshot: .../lwh_fire.tga (behavioural: the frame does not carry the smoke; see docs/frame-assertions.md)

$ scripts/breach-check.sh
PASS  a breach pushes and hurts where it is authored; the field stops it and holds the air; the gate holds
```

### What still cannot be seen

**Two frames now contain their subject; two still cannot carry their claim**, and the honest answer
for those two is the behavioural classification kept, not an assertion fitted to them:

- **`fire`**: the camera stands where the deck's first fire appears, and the smoke renders against a
  dark deck. The subject region changes 0.2–0.9%, and the difference is the crew walking through the
  region, not the fire — a difference, but not the change the claim means.
- **`breach`**: the camera stands at the authored field, and the breach compartment renders
  near-black. The field-off and field-on frames differ by 0.0–0.1% of the subject region. The claim
  is "a field you can see"; in this room there is nothing to see, and the log line is the proof.

`docs/frame-assertions.md` rows 7, 8, 10 and 11 carry the re-classification and the measurements.

## What could not be verified

- **Whether the new frames read as damage being worked, as fire, as a held field** — that is the
  owner's eye, and the frames come to him. The tool decides "changed", not "right".
- **The old emergency, fire and breach frames were overwritten before I kept them.** The "before"
  figure for `emergency` (0.0% of the scene band) is quoted from
  `docs/evidence/the-frame-assertions.md`, measured by the earlier pass on the same host; I could not
  re-measure it. The old `damage` pair survived in `build/frame-assertions/` and **is** re-measured
  above.
- **The frames' colour and composition on a real display** — all of this is headless `xvfb` at
  1280×1024, the engine's own screenshots.

## Judgement calls, named as calls

1. **Task B is a layer change, not a build step.** The maps' own class name is accepted in
   `module/`, and the merged ship's posts are derived per deck from it. Rejected: having `stitch.py`
   emit `navgoal` entities, which would change the map every time and put logic in the build.
2. **The "no posts" line is said once per deck at load, not per member.** The brief asks for *once,
   loudly*; a per-member line would repeat for every body. The detector still cannot fire for that
   deck.
3. **The seed is capped below `MAX_POSTS`,** round-robin across decks (216 on the merged ship), so a
   deck with few goals is not starved and the ship's own `place_` posts still fit.
4. **The damage threshold is a small region at a low change fraction.** A spark is a small thing; a
   50×60 region the camera aims at is the region the claim owns, and 0.5% is above the scene's
   stillness and beyond the HUD. It can fail for the reason that matters (the spark absent → 0.0%).
5. **The emergency assertion is the strip's own region, not the scene band.** The strip is what
   changed (25.3%); the band would have scored the room and been satisfiable by other light.
6. **`fire` and `breach` stay behavioural.** Their frames do not carry the claim after the camera was
   aimed, and the pass's own rule is to say so rather than fit an assertion.
7. **The brief's mechanism was wrong and is corrected here.** The `.crew` stub is not read under
   `g_crewFromShip`; the false "fell out of the world" was a missing floor, not a stub at one point.
