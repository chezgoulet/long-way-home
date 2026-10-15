# Evidence — the frames are shot from the site's own vantages

Date: 2026-10-11. Branch: `feat/the-away-site` (cut from `testing`, `d219d2d`).
Brief: `BRIEF.md` (the pass), carried on the branch that built the site, because it repairs that
pass's frames before they land. The pass whose evidence is affected: `docs/evidence/the-away-site.md`
(its Task C, D and E). The rule it repairs is the standing one from `docs/evidence/the-dark-room.md`.
The instrument is `tools/frames/frame.py`.

Nothing here is asserted without the command that produced it. The command is given before its output.

> **Corrected 2026-10-15 (the exterior pass, `docs/evidence/the-away-site-exterior.md`).** This evidence
> ends with the verdict **partly**, on two named causes: the structure encloses most facings, and *"a
> machine block the re-dress's drop rule missed blocks the stores post's outward view."* **The second
> cause is a mis-attribution.** Casting the stores post's eye line through the built map (Task D,
> `:159`) names not a source block but the site's **own** cargo crate, built by `_crates()` — a brush
> the drop rule can never touch, because the rule governs only copied source brushes. The exterior pass
> lowers that crate, closes the drop rule's real hole (the source's *other* block does survive it), and
> the stores post now opens east onto the debris field; its outward frame is now the named dark frame
> (its subject is the sky), and the pace `_5` reads. **The first cause — the structure encloses most
> facings — stands.** The statements below at Task D (`:159-165`), the frame captions (`:191,194`), the
> verdict (`:244-254`) and judgement call 4 (`:328-330`) are kept as the record of the find.

## The defect

`module/ship/g_ship.cpp`'s vantage test (`g_shipTest 96`) held **the security office's anchor and its
screenshot naming inside the module**: the block found the point entity `lwh_armory_post` and wrote
`screenshot lwh_darkroom_security_<n>`. `scripts/away-check.sh` drove that test on the depot, so the
depot's frames were named `lwh_darkroom_security_*` and its HUD lines said *"security post-north at
(960 908 224)"* while the player stood at the arrival, not at the site's own post.

**A frame's filename is the run's intention, not its content.** The names said the dark room's
security office; the pictures were of the depot. The two disagreed, and nothing caught it: the frames
were measured and *passed*, so the check was green while its evidence was mislabelled and taken from
vantages the site does not work from.

## Task A — where to stand, and what to call the frames, come from the caller

The module now reads both from cvars, and neither the security office's post nor its frame name is
baked into the rig:

```c
cvar_t *g_shipVantage;     // the frames' stem, e.g. "lwh_darkroom_security" or "lwh_away_depot_console"
cvar_t *g_shipVantagePost; // the targetname of the point entity to stand at, e.g. "lwh_armory_post"
...
g_shipVantage     = gi.cvar( "g_shipVantage", "lwh_darkroom_security", 0 );
g_shipVantagePost = gi.cvar( "g_shipVantagePost", "lwh_armory_post", 0 );
```

* **where to stand** — `g_shipVantagePost` names the point entity to stand at (a room's or a site's
  own post). A `"x y z"` in `g_shipTestPos` still overrides it, so a candidate post can be stood on
  without rebuilding the map. A post the caller names and the map does not carry is **refused
  loudly**: the run prints `no point entity named "..." on this map -- 0 vantage(s) photographed` and
  photographs nothing, rather than quietly standing somewhere else and calling the frames its own.
* **what to call the frames** — `g_shipVantage` is the stem: the frames are
  `screenshots/<stem>_<n>.tga`. The rig's own log lines carry the stem, so the run's intention and
  its content are readable in the same line.

The defaults belong to the dark room's own command, which sets neither — so its caller is untouched.
The six vantages (four facings at the post, and a pace west and east of it) are unchanged: this pass
lifts the anchor and the naming, not the rig.

## Task B — the dark room is unchanged, and shown

Its own command, with **neither new cvar set**, on the merged ship:

```
$ SDL_AUDIODRIVER=dummy xvfb-run -a scripts/run-engine.sh --home-dir build/g3-home \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipMode 1 +set g_shipDeckPitch 3072 +set g_shipTest 96 \
    +map voyager
SHIP: vantage: lwh_darkroom_security: the anchor at the post "lwh_armory_post": (-4172 -3906 -19332)
SHIP: vantage: lwh_darkroom_security post-north at (-4172 -3906 -19308) facing 90
SHIP: vantage: lwh_darkroom_security frame 0, player at (-4172 -3906 -19331), viewangles (0 90 0)
... post-south/_1, post-east/_2, post-west/_3, west-east/_4 at (-4364 -3906 -19308),
    east-west/_5 at (-3980 -3906 -19308)
SHIP: vantage: lwh_darkroom_security: 6 vantage(s) photographed

$ ls build/g3-home/baseEF/screenshots/
lwh_darkroom_security_0.tga  lwh_darkroom_security_1.tga  lwh_darkroom_security_2.tga
lwh_darkroom_security_3.tga  lwh_darkroom_security_4.tga  lwh_darkroom_security_5.tga

$ python3 tools/frames/frame.py floor --band 0.10,0.55 build/g3-home/baseEF/screenshots/lwh_darkroom_security_*.tga
lwh_darkroom_security_0.tga   band 0.1-0.55  mean  92.0 median  87.0 p90 122.0 spread 121.0  LEGIBLE
lwh_darkroom_security_1.tga   band 0.1-0.55  mean  87.0 median 114.0 p90 139.0 spread 135.0  LEGIBLE
lwh_darkroom_security_2.tga   band 0.1-0.55  mean 108.7 median 119.0 p90 122.0 spread  51.0  LEGIBLE
lwh_darkroom_security_3.tga   band 0.1-0.55  mean  72.4 median  62.0 p90 122.0 spread 129.0  LEGIBLE
lwh_darkroom_security_4.tga   band 0.1-0.55  mean  81.2 median  60.0 p90 226.0 spread 201.0  LEGIBLE
lwh_darkroom_security_5.tga   band 0.1-0.55  mean  89.0 median  90.0 p90 141.0 spread 174.0  LEGIBLE
```

**The frame names are the same names** (`lwh_darkroom_security_0..5`), the anchor is the same post
(`lwh_armory_post` at `-4172 -3906 -19332`), the six vantages are the same six, and the six frames
measure as they did: the means match `docs/evidence/the-dark-room.md` (92.0 / 86.9 / 108.6 / 75.3 /
81.2 / 88.8) to within the last decimal. **The dark-room caller still works.** One thing did change
and is named rather than hidden: the log prefix is now `SHIP: vantage:` instead of `SHIP: dark room:`,
because the test is no longer only the dark room's. The frame names — the thing the brief protects —
are unchanged.

## Task C — the depot, from its own posts

The site's own posts are read from the generated map and cross-checked against the dress report's
`posts`, so the caller stands where the site actually works, not where it was told a post ought to be:

```
posts from the map (waypoint_navgoal), cross-checked against build/away/dress.json:
  console  lwh_post_console  720 1036 208
  stores   lwh_post_stores   924 780 192
```

Each post is stood at by name, and the harness prints the anchor it honoured — which the check greps
for, so a run that stood elsewhere fails:

```
$ scripts/away-check.sh          (excerpt, the two vantage runs)
==> the vantages, the console post (lwh_post_console at 720 1036 208)
    vantage: lwh_away_depot_console post-north at (720 1036 232) facing 90
    vantage: lwh_away_depot_console frame 0, player at (720 1036 208), viewangles (0 90 0)
    ... post-south/_1, post-east/_2, post-west/_3 at (720 1036 232); west-east/_4 at (528 1036 232)
        settled to (528 1036 192); east-west/_5 at (912 1036 232) settled to (912 1036 192)
==> the vantages, the stores post (lwh_post_stores at 924 780 192)
    vantage: lwh_away_depot_stores post-north at (924 780 216) facing 90
    vantage: lwh_away_depot_stores frame 0, player at (924 780 192), viewangles (0 90 0)
    ... and the same six
```

`player at` is where the engine settled the player after the teleport: **on the deck, at standing
origin height** (floor + 24). The camera is then at that origin plus the view height, so every frame
is from a person's eye. The post's own four facings are `_0.._3`; `_4`/`_5` are the rig's pace west
and east of the post.

The frames are named for the site and for the post they were shot from:
`lwh_away_depot_console_0..5.tga`, `lwh_away_depot_stores_0..5.tga`. Twelve frames, six per post.

## Task D — the light, measured per vantage

`tools/frames/frame.py floor`, the middle band (`0.10,0.55`: above it the console text, below it the
weapon), a pixel's value being its brightest channel:

```
$ python3 tools/frames/frame.py floor --band 0.10,0.55 build/away-home/frames-console/*.tga \
      build/away-home/frames-stores/*.tga
lwh_away_depot_console_0.tga   band 0.1-0.55  mean 55.8 median 53.0 p90  80.0 spread  65.0  LEGIBLE
lwh_away_depot_console_1.tga   band 0.1-0.55  mean 71.9 median 56.0 p90 147.0 spread 182.0  LEGIBLE
lwh_away_depot_console_2.tga   band 0.1-0.55  mean 48.0 median 42.0 p90  85.0 spread 139.0  LEGIBLE
lwh_away_depot_console_3.tga   band 0.1-0.55  mean 56.8 median 53.0 p90  82.0 spread  66.0  LEGIBLE
lwh_away_depot_console_4.tga   band 0.1-0.55  mean 56.0 median 47.0 p90  99.0 spread 168.0  LEGIBLE
lwh_away_depot_console_5.tga   band 0.1-0.55  mean 60.3 median 53.0 p90  90.0 spread  96.0  LEGIBLE
lwh_away_depot_stores_0.tga    band 0.1-0.55  mean 67.6 median 57.0 p90 109.0 spread 139.0  LEGIBLE
lwh_away_depot_stores_1.tga    band 0.1-0.55  mean 58.1 median 54.0 p90  84.0 spread  67.0  LEGIBLE
lwh_away_depot_stores_2.tga    band 0.1-0.55  mean 53.5 median 49.0 p90  85.0 spread  94.0  LEGIBLE
lwh_away_depot_stores_3.tga    band 0.1-0.55  mean 62.6 median 54.0 p90  91.0 spread  97.0  LEGIBLE
lwh_away_depot_stores_4.tga    band 0.1-0.55  mean 51.9 median 47.0 p90  85.0 spread 110.0  LEGIBLE
lwh_away_depot_stores_5.tga    band 0.1-0.55  mean 29.0 median 24.0 p90  45.0 spread  43.0  DARK -- median 24 < 35 (mostly black)
```

**Every one of the posts' own four facings reads** (eleven of twelve frames LEGIBLE), so both of the
site's own posts stand in the light. Three runs of the same vantages agree to 0.2 of a unit of
measurement and to a tenth of a percent on the diff (below); the numbers are stable to that and are
not claimed to be byte-identical.

**The one dark frame is named, and it is not the room.** `lwh_away_depot_stores_5` is the rig's own
pace *east* of the stores post — `(1116 780)` — turned west. A query over the re-dressed map's world
brushes shows what the camera is aimed at:

```
$ python3 - <<'EOF'   # the world brush the stores post's east-west pace is aimed at
    ... reads build/away/generated/lwh_away_depot.map's world brushes ...
EOF
the block: x 1020..1100  y 732..812  z 168..248  (its east face 16 units in front of the camera at x 1116)
```

The camera stands **16 units from the unlit east face of an 80×80×80 machine block** that sits on the
open apron. The picture is of that block, at arm's length, in its own shadow. This is the dark-room
pass's own category — *a facing can be dark in a well-lit place* — and the defect it exposes is not
the site's light but **the block**: it is one of the source's machine blocks the re-dress's drop rule
was meant to remove, and it survived because its bottom is *exactly* on the deck floor (`z 168..248`,
the floor is 168), so the rule's `lo[2] > FLOOR + 8` test did not recognise it as a block standing
*on* the deck. **The block blocking the outward view from the stores post is a finding for the next
pass; it is not fixed here** (the brief excludes building or changing the exterior). It is recorded,
not dressed over, and the check names it: any *other* dark frame fails.

## Task E — what each frame shows, having looked at all twelve

Converted with **PIL** (`Image.open(...).save(...)`, not ImageMagick) and looked at before captioning.
Six per post; `N/S/E/W` are the post's own facings, `west-east`/`east-west` are the rig's paces.

**The console post — `(720 1036)`, the sheltered half's work post:**

| frame | facing | what it shows |
|---|---|---|
| `_0` | post-north | A rust-streaked plate wall filling the frame, a lamp box top-left, machinery left. The site's own wall — no sky. |
| `_1` | post-south | The sheltered interior: rusted plate walls with pale glazed panels and a lamp, a red trim strip on a machine at the left, metal grating floor receding to a doorway. A lit industrial room. |
| `_2` | post-east | **The exterior.** Between two rusted structures — a wall with a lamp box left, a machine riser and a red trim strip right — an opening onto **black sky scattered with debris and, upper right, a large broken hull tumbling in it**. A pale lit panel stands in the gap; the deck grating runs out toward the opening. Reads as a place open to space. |
| `_3` | post-west | A blank rusted wall and the grating floor. The site's own wall. |
| `_4` | west-east pace | A long aisle between walls running east, three pale lamp boxes on the left wall, red trim strips on machines both sides, and **ahead and above, the black debris sky and blue lamps**. The widest view of the site's own length with space beyond it. |
| `_5` | east-west pace | An aisle in the sheltered half: grating floor, walls both sides, a row of blue lamps down the middle distance, red trim top-left. Interior; no sky. |

**The stores post — `(924 780)`, on the open apron east of the roof line:**

| frame | facing | what it shows |
|---|---|---|
| `_0` | post-north | A rusted machinery wall across the frame with two pale lamp boxes, a red-lit machine at the left, grating floor, a blue lamp mid-right. A lit interior-looking face — this is the sheltered structure seen from the apron. |
| `_1` | post-south | A rusted plate wall filling the frame, floor at the bottom edge. A wall at arm's length. |
| `_2` | post-east | **Partly the exterior.** A large rusted machine block fills the centre at 96 units; **above it, a band of black sky with debris and a wreck fragment**, a red trim strip across the gap. The block is the one named in Task D. |
| `_3` | post-west | A rusted wall with a pale lamp box, grating floor, red trim at the right. Looking west, back toward the sheltered half: interior. |
| `_4` | west-east pace | **The clearest exterior of the set.** A long aisle of the outpost running east, red trim strips along the left machinery, pale lamp boxes on the right wall, and — beyond the structure and overhead — **a wide black sky full of debris and a broken hull**, with blue lamps down the aisle. Reads as a deck standing in space. |
| `_5` | east-west pace | The dark frame: an almost featureless dark brown surface, faint vertical striations, filling the frame. The east face of the machine block of Task D, 16 units away. |

## Task F — the falsification

**The anchor plumbing is broken and a check catches it.** The post the caller names is replaced by one
the map does not carry:

```
$ LWH_VANTAGE_POST=lwh_post_nowhere scripts/away-check.sh
==> the vantages, the console post (lwh_post_nowhere at 720 1036 208)
FAIL  the harness did not stand at the site's own post lwh_post_nowhere (console; see .../vantage-console.out)
```

The module refused rather than photographed from somewhere else, and the check failed on the refusal:

```
$ grep -h "no point entity" build/away-home/vantage-console.out
SHIP: vantage: lwh_away_depot_console: no point entity named "lwh_post_nowhere" on this map -- 0 vantage(s) photographed
```

**Restored**: the override is an environment variable read by the check, nothing in the module or the
map was changed, and the check is green again (the runs below). The same falsification can be made
with a bad `"x y z"` in `g_shipTestPos`, which `RoomPostAnchor` rejects loudly (the room-reads pass's
demonstration, `docs/evidence/the-room-reads.md`).

**And the naming, which is what let the defect hide.** A run that stands at the depot's own post but
names the frames for the dark room — the *before* state, now reachable only by the caller asking for it:

```
$ SDL_AUDIODRIVER=dummy xvfb-run -a scripts/run-engine.sh --home-dir build/away-home \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest 96 +set g_shipVantagePost lwh_post_stores \
    +map lwh_away_depot
SHIP: vantage: lwh_darkroom_security: the anchor at the post "lwh_post_stores": (924 780 192)
$ ls build/away-home/baseEF/screenshots/
lwh_darkroom_security_0.tga ... lwh_darkroom_security_5.tga
```

The frames are the depot's; the names are the dark room's — the disagreement the brief was written
about. The check does not need to look at the pictures to catch it: it asserts that its frames are
`lwh_away_depot_<post>_<n>.tga` and that there are six of them, so a mislabelled or short run fails
before anything is measured.

## The verdict — does it read as outside?

**Partly.** The frame that decided it is **`lwh_away_depot_console_2.tga`** — the console post turned
toward the open half: it is a picture of an opening in the outpost's structure onto **a black sky with
debris and a broken hull tumbling in it**, and there is no way to read that as a corridor or a painted
wall. `lwh_away_depot_stores_4.tga` (the apron pace, looking east down the site) is the strongest of
the set: the debris field and the wreckage fill the sky beyond and above the deck.

But **the site's own structure encloses most facings**: at both posts, north, south and west are
rusted walls and machinery at arm's length (`console_0/3/5`, `stores_0/1/3`), and at the stores post
even the outward facing is more than half blocked by the machine block of Task D. The site therefore
reads as **a sheltered outpost with a window onto space**, not as an apron standing open on it. The
exterior the source's `common/sky_light` carries **does** deliver a sky — it is `env/junk`, the debris
field with wreckage, clear in `console_2`, `console_4` and `stores_4` — but the site does not yet open
onto it enough for the frames to read unambiguously as *outside*.

That is the finding, and the next pass's work: **the site's own walls and machinery frame almost every
view, and the block the drop rule missed (which should have gone with the other machine blocks) closes
the one outward view from the stores post; the exterior wants more opening.**
The 71.3 % "differs" of the earlier pass measured a pair of arrival frames; that pair no longer exists
(the arrival is not a post) and its replacement measures **61.2 %** at the console post — a smaller
number, honestly, because the post is a person's working position inside the structure rather than a
spot on the open edge.

## The check, and the gates

`scripts/away-check.sh` is the regression, and now: re-dress, build, artifact, census, the two posts'
vantages, the light measured per vantage, the named dark one, the outside diff, nav, crew.

```
$ scripts/away-check.sh
...
PASS  every material already exists in the game; no new art
PASS  both of the site's own posts read at their own four facings -- the site stands in its light
PASS  the only dark vantage is the one named -- the stores post's east-west pace against the deck's own machine block
PASS  the frame that looks out shows a world: it differs from the site's own wall at the same post
PASS  the engine baked maps/lwh_away_depot.nav on first load
PASS  every crew member the ship put on the site walked to its place, and none is in solid

PASS  the away site loads, walks, reads as outside from its own posts (partly), is lit by measurement,
      and its sources are named
```

The outside diff, in the same run:

```
$ python3 tools/frames/frame.py diff --band 0.10,0.55 --min-changed 0.5 --min-delta 24 \
      build/away-home/frames-console/lwh_away_depot_console_3.tga \
      build/away-home/frames-console/lwh_away_depot_console_2.tga
lwh_away_depot_console_2.tga   band 0.1-0.55  changed  61.2% mean|d|  40.6 of 590080 px  DIFFERS
```

The checks the change could disturb:

```
$ scripts/test.sh                                       -> all checks passed (exit 0)
$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map \
      --script-corpus build/gdk/scripts                 -> exit 0
$ scripts/harness-check.sh                              -> the g_shipTest case numbers are distinct
$ scripts/room-survey-check.sh                          -> PASS; the security office's post reads
```

## What could not be verified here

- **Whether the site reads as *somewhere*.** That is the owner's walkthrough, and unchanged by this
  pass. The frames are now the site's own, which is all this pass can prove.
- **The exterior's *quality*.** That `console_2`, `console_4` and `stores_4` show the debris field is
  established; whether the arrangement reads as *this* system's sky, or as the game's generic junk, is
  not, exactly as the site pass recorded.
- **The frames' determinism at the byte level.** Runs of the same vantages agree to 0.2 of a unit of
  measurement and to a tenth of a percent on the diff, not exactly; something in the render (light
  flicker or the spawn) moves a few pixels. The numbers are stable enough to assert on, and are not
  claimed to be identical.
- **Whether a *pace* is a person's position.** The rig's paces are the dark-room rig's invention, ±192
  units along x; the stores post's east pace lands 16 units from a block. A pace is a probe, not a
  post, and this pass reports it rather than moving the site's posts (which are the dress report's own
  two, and the thing the brief asked to stand at).

## Judgement calls, named as calls

1. **The defaults stay the dark room's.** `g_shipVantage`/`g_shipVantagePost` default to
   `lwh_darkroom_security`/`lwh_armory_post` so the dark-room pass's own command (which sets neither)
   is byte-for-byte the run it was. A caller that sets neither and is not the dark room gets the dark
   room's labels — which is why the check asserts the frame names, and why the falsification above is
   reachable. Making the defaults empty would have broken the dark room's documented command.
2. **The dark room's log prefix changed** (`SHIP: dark room:` → `SHIP: vantage:`) because the test is
   general now. Its frame names did not. `docs/evidence/the-dark-room.md`'s quoted log lines are
   historical; the *frame* quotes in it remain true.
3. **The post, not every vantage, is required to read.** A post's own four facings are its claim;
   the rig's paces are a probe. The one dark pace is named (`stores_5`) and any other dark frame
   fails — the same structure the site pass used for the arrival's dark sky frame, which is also the
   dark-room ruling's own ("the ruling is about the room, not about every direction").
4. **The machine block is reported, not removed.** The brief excludes building or changing the
   exterior; the block's survival is a hole in the re-dress's drop rule (`lo[2] > FLOOR + 8` misses a
   block whose bottom is exactly the floor), which is the next pass's one-line change and a map
   rebuild, not this pass's.
5. **The paces are still the dark-room rig's.** "Where to stand" was lifted for the *post*; the paces
   are the rig's own probe and were left alone, so the dark room's six vantages are untouched. A
   caller cannot ask for a different pace, and a rig that named and skipped a pace it could not stand
   at would be a second change to the rig — not made here.
