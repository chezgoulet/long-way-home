# Evidence: the away site, opened onto the exterior (Item 1 / O11)

**What this closes.** The vantages pass (`docs/evidence/the-away-vantages.md`) downgraded the site's
"reads as outside" to **partly**, with two named causes: *"the structure encloses most facings, and a
machine block the re-dress's drop rule missed blocks the stores post's outward view."* This pass opens
the **stores post's outward view** and records that the second cause was **mis-diagnosed**.

**Host:** `sasquatch`, the engine host. The re-dress, the map build, the engine run and the frame
measures all ran here. Branch `feat/away-site-exterior`, cut from `origin/testing`.

## 1. The finding: the blocker was the site's own crate, not a source block

The vantages evidence named an "80×80, bottom exactly on the deck floor" machine block that survived
the drop rule. Casting the stores post's eye-line (`y=780, z=216`, looking east) through the **built**
map names what is actually in the way:

```
$ python3 - <<'PY'   # brushes in the generated map whose box contains the eye line, x 920..1280
...
  x_lo=1020  tex=forge/fmetal1  lo=(1020,732,168) hi=(1100,812,248) cls=worldspawn
PY
```

That brush is not copied from `ctf_space` — it is the site's **own** cargo crate, built by
`_crates()` at `(x+96 … x+176, y-48 … y+32, base … base+80)` from the stores anchor `(924,780)`. It is
80×80×80, its bottom is exactly on the deck floor, and it sits **squarely in the stores post's eastward
sightline**. The drop rule only governs **source** brushes (`copy_brush`); it can never drop a brush the
tool builds itself, so the vantages pass's "the drop rule missed it" was the wrong diagnosis — there
was no source block in the way. (Separately, the drop rule **does** have a hole: the source's
192×192×208 block at `(1024,880)-(1216,1072)` survives it. That block is real and is fixed too; it just
was not the blocker.)

## 2. The fix, in three parts (all in `tools/away/dress_away_site.py`)

- **The drop rule's hole closed.** A large brush is dropped if it is a body standing on the deck, not
  only if it floats above it: `block and (lo[2] > FLOOR+8 or (lo[2] >= FLOOR-8 and height > 64))`. The
  deck's own floor plates and ribs are thin and the walls reach below the floor, so they stay.
  **One** source brush goes (31 kept → 30).
- **The crate lowered, not moved.** The crate stays east of the rack (where a crate off a rack would
  fall) but is kept **low** — its top below the eye line — so the stores post looks out *over* it into
  the debris field. Moving it west instead was tried and rejected: it opened a spurious third post (see
  §3).
- **The arrival is not a post.** `_waypoints()` built a `waypoint_navgoal` for the arrival as well as
  the two work posts; with the crate's solids changed, the arrival search found a spot and the site grew
  a third post. The site's contract is **two** posts (the vantages evidence's own phrase), and the
  arrival is the player's spawn (`info_player_start`), not a place of work. The arrival entry is removed
  from the post loop.

## 3. What I verified by running it — the picture, before and after

The site's own gate, `scripts/away-check.sh`, exits **0**. The before/after pair is at the **stores
post, frame 2 (east), the same vantage**. **The frames are preserved outside the repository** (the repo
carries no game media) at the absolute paths

    /home/c/lwh-frames/the-away-site-exterior/before/lwh_away_depot_stores_2.tga   (the blocked view)
    /home/c/lwh-frames/the-away-site-exterior/after/lwh_away_depot_stores_2.tga    (the opening)

Each frame is **1280×1024**; the measure's band `0.10,0.55` is **y 102..563, x 0..1280 — 461 rows,
590,080 px** (full width, the middle 45 % of the height). Both sets hold all six stores-post frames. The
`before` set is regenerated with the pre-pass tool (`git show ce9428c^1:tools/away/dress_away_site.py`)
and the `after` set with the tool on this branch; the diff below reproduces from the preserved files:

```
$ python3 tools/frames/frame.py floor --band 0.10,0.55 \
      /home/c/lwh-frames/the-away-site-exterior/{before,after}/lwh_away_depot_stores_2.tga
# BEFORE  stores_2  mean 53.5 median 49 p90  85 spread  94  LEGIBLE   (the crate fills the view)
# AFTER   stores_2  mean 41.5 median 41 p90  73 spread  82  DARK -- p90 73 < 80 (nothing lit)
$ python3 tools/frames/frame.py diff --band 0.10,0.55 --min-changed 0.03 --min-delta 24 \
      /home/c/lwh-frames/the-away-site-exterior/before/lwh_away_depot_stores_2.tga \
      /home/c/lwh-frames/the-away-site-exterior/after/lwh_away_depot_stores_2.tga
  lwh_away_depot_stores_2.tga   band 0.1-0.55   changed  23.2% mean|d|  18.1 of 590080 px  DIFFERS
```

**The region that was blocked differs by 23.2 %** — the crate where the sky now is. The after frame
reads DARK by the room measure because **its subject is the black debris field**; the deck under it is
lit (median 41, spread 82), which is the category the away-site evidence already names for the console
post's outward frame: *a legibility threshold written for a room cannot be passed by a picture of the
sky.* Looking at the frame confirms it: the lowered crate sits low in the foreground with red trim, and
above it the **debris field** — black sky with tumbling wreckage — fills the opening where the crate
used to be.

The second effect is the source block's removal: the pace `stores_5` (facing west, 16 units from the
block) went from **DARK (p90 45)** to **LEGIBLE (p90 101)**, a 63.3 % change — it now looks down the
lit deck instead of at the block's unlit face. (`/home/c/lwh-frames/the-away-site-exterior/before/lwh_away_depot_stores_5.tga`
and `…/after/lwh_away_depot_stores_5.tga`, 1280×1024, band `0.10,0.55` = y 102..563.)

The "opening shows a world" property, at the stores post, **moved the right way**:

```
$ python3 tools/frames/frame.py diff --band 0.10,0.55 --min-changed 0.3 --min-delta 24 \
      /home/c/lwh-frames/the-away-site-exterior/after/lwh_away_depot_stores_3.tga \
      /home/c/lwh-frames/the-away-site-exterior/after/lwh_away_depot_stores_2.tga
  lwh_away_depot_stores_2.tga   band 0.1-0.55   changed  60.3% mean|d|  39.1 of 590080 px  DIFFERS
# the same comparison BEFORE the pass: 54.0 % -- the outward view now differs *more* from the wall
```

## 4. The check, updated to the new state

- The dark frame is now **named as the stores post's outward view** (`ALLOWED_DARK =
  lwh_away_depot_stores_2.tga`), because its subject is the sky; the stance is documented in the check.
- The "every post's own facings must read" rule now exempts that one named frame (the post's other
  facings — where the station and the structure are — must still read, so a post with no legible stand
  still fails).
- A new assertion: **the stores post opens onto the world** — its outward frame differs from the wall
  behind it (60.3 %, above the 30 % bar).

## 5. Two stale things corrected in passing, named

- **The crew threshold.** `scripts/away-check.sh` required **≥ 10** crew embodied on the site; the ship's
  roster now puts **9** on deck 1 at 08:00, because the **nine-departments pass** (2026-10-12, 3.3)
  moved it from the 13 the check was written against. This is **not** an away-site change — it is the
  roster's — and the check had not run since (it needs game data, so CI does not run it). Corrected to
  `≥ 9` with the finding named in the check.
- **The vantages evidence's diagnosis.** `docs/evidence/the-away-vantages.md` attributes the blocked
  view to "a machine block the re-dress's drop rule missed"; §1 shows the blocker was the site's own
  crate. The vantages evidence is annotated in place, keeping the finding as the record.

## 6. What I could not verify, and why

- **Whether the site reads as *somewhere* (the owner's eye, not mine).** The frames are the evidence he
  judges; the seam between the copied deck and the new shell is the risk, and it is his call.
- **`scripts/check.sh`** (the GDK corpus pass) is the playtest host's — it needs the GDK script corpus
  and compiler; `scripts/test.sh` is run here and is the one CI runs.

## 7. What I deliberately left unbuilt

- **The structure still encloses most facings**, the vantages pass's *first* named cause. This pass opens
  the **stores post's outward view** (the second cause); opening the *console* half further, and the
  wider shell, is a judgement about the site's shape that belongs with the owner's walkthrough, and the
  other zone of the site is intact. Named, not silently kept.
- **The other five archetypes** (derelict interior first) — Item 2's queue.
- **The away-mission loop** — scenario content, registered and named (the run's queue), not started.

## 8. Judgement calls, named as calls

1. **I treated the blocker as the site's own crate, not a source block**, because casting the eye line
   through the built map names the crate and nothing else. The vantages pass's "machine block the drop
   rule missed" is corrected as a mis-attribution.
2. **I fixed the drop rule anyway**, because the hole is real (a source block survives it) and the
   correction is cheap; but I say plainly it is not what blocked the view.
3. **I lowered the crate rather than moving it**, after moving it west produced a spurious third post —
   the minimal change that opens the sightline and keeps the site's two-post contract.
4. **I removed the arrival from the posts**, treating it as a spawn rather than a work post; the site's
   two posts are the console and the stores, which is the contract the check and the evidence both name.
5. **I set the crew floor to the observed 9, named**, rather than rounding to a round number, and
   recorded that the change is the roster's, made by another pass.
