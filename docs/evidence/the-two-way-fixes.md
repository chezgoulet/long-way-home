# Evidence: the two-way fixes (Item 0)

**What this is.** Five things the Item-1 pass left in a state a reviewer had to repair by hand. They are
one branch (`fix/the-two-way-fixes`) and one PR. **The one that was larger than it looked: (c)** — making
a cited frame measure reproducible needed the pre-pass tool recovered from git, a map rebuild, an engine
capture and a frame store outside the repository; it is not a doc edit.

**Host:** `sasquatch`, the engine host. Cut from `origin/testing` at `ce9428c`.

---

## (a) The dark exemption is now two-way

`scripts/away-check.sh` named `lwh_away_depot_stores_2.tga` in `ALLOWED_DARK` and failed only when a
*different* frame was dark. That guard is **one-way**: it reports the exemption working and can never
report the exemption ceasing to be needed — if a later pass seals the opening and the frame becomes a lit
wall, the check still passes and the lost outward view goes unremarked.

The rule is now `tools/frames/dark_probe.py`, run by the check:

- the **named** frame **must read dark**; if it does not, the opening its subject stands for is gone, and
  the probe **fails**, naming it;
- **no other** frame may read dark; a dark frame that is not the named one **fails**, naming it.

**Both directions demonstrated** (the guard is evidence only once it has failed on the input it exists to
catch):

```
$ python3 tools/frames/dark_probe.py --band 0.10,0.55 --named lwh_away_depot_stores_2.tga \
      build/away-home/frames-*/*.tga ; echo "exit=$?"
PASS  the named frame reads dark as expected: lwh_away_depot_stores_2.tga (the opening it stands for is still open)
PASS  no frame other than the named one reads dark (12 frame(s) measured)
exit=0
```

```
# the same set with that facing replaced by a WALL frame (stores_2 <- stores_3)
$ python3 tools/frames/dark_probe.py --band 0.10,0.55 --named lwh_away_depot_stores_2.tga \
      /tmp/opencode/wallset/*.tga ; echo "exit=$?"
FAIL  the named frame lwh_away_depot_stores_2.tga is NOT dark: the opening it stands for has been lost (a lit surface where the sky was)
exit=1
```

---

## (b) The archetype survey is landed, with its instrument named

The stopped Item-2 pass measured four candidate sources and cited them to "`tools/stitch`", **not a path
on this tree**. The survey is now a file — `tools/away/survey_sources.py`, wrapping the real parser
`tools/shipmap/stitch.py` — and the rows are landed in `docs/evidence/the-archetype-survey.md`, with each
source's path, the exact command, and the reading. **It counts brushes, not faces** (a face count reads
~6× larger), which the cross-check confirmed: entities and lights reproduce exactly, brush counts are the
brush counts.

```
$ tools/away/survey_sources.py build/gdk/maps/*/scav2.map build/gdk/maps/*/scav4.map \
      build/gdk/maps/*/dn3.map build/gdk/maps/*/borg2.map
map            brushes patches entities lights   size (x y z, units)              footprint  height
scav2             7028      61     1201    475  x  -1176..1612   y  -2376..2068   z   -168..476      2.7x4.3   0.6
scav4             3263     331      878    197  x   -996..1728   y    272..4310   z   -648..808      2.7x3.9   1.4
dn3               2394     172      645    296  x   -778..2096   y  -1318..3524   z  -1560..956      2.8x4.7   2.5
borg2             4098     240      825    332  x  -1348..3016   y  -2720..1464   z   -776..1648     4.3x4.1   2.4
```

`docs/outside-the-ship.md`'s **derelict interior** row now names `scav2` and points at the survey, so the
next pass starts from the row and does not re-derive it.

---

## (c) A cited frame measure is now reproducible by a reader

The last evidence cited frames by relative paths (`frames-stores-before/`, `frames-stores-after/`) that no
longer existed on the host, so one of its figures could not be reproduced from the document. **Corrected**
in `docs/evidence/the-away-site-exterior.md`, and the frames are **preserved outside the repository**:

```
/home/c/lwh-frames/the-away-site-exterior/before/lwh_away_depot_stores_2.tga   (the blocked view)
/home/c/lwh-frames/the-away-site-exterior/after/lwh_away_depot_stores_2.tga    (the opening)
```

Every frame cited is **1280×1024**; the band `0.10,0.55` is **y 102..563, x 0..1280 — 461 rows,
590,080 px**. The `before` set is regenerated with `git show ce9428c^1:tools/away/dress_away_site.py`
(the pre-pass tool) and the `after` set with this branch's tool. **The citation reproduces**, from the
preserved files:

```
$ python3 tools/frames/frame.py diff --band 0.10,0.55 --min-changed 0.03 --min-delta 24 \
      /home/c/lwh-frames/the-away-site-exterior/before/lwh_away_depot_stores_2.tga \
      /home/c/lwh-frames/the-away-site-exterior/after/lwh_away_depot_stores_2.tga
  lwh_away_depot_stores_2.tga   band 0.1-0.55   changed  23.2% mean|d|  18.1 of 590080 px  DIFFERS
```

**Which frame set belongs outside the repo, and why:** the repo carries no game media, so the frames live
under `/home/c/lwh-frames/` on the host, and the evidence carries their absolute paths. This pass's derelict
frames (Item 1) go to `/home/c/lwh-frames/the-derelict/` on the same principle.

---

## (d) The crew floor is recorded as awaiting the owner's ruling

`scripts/away-check.sh` requires **≥ 9** embodied crew; it was written requiring **≥ 10**, and the
nine-departments pass (3.3) moved the deck-1 roster from 13 to 9. **The number is not changed again either
way.** The question is recorded in `docs/gates.md` (*Residuals*), with **both readings named and opposite**
— *(a)* a stale check corrected against a deliberate roster change, or *(b)* a design requirement quietly
weakened to make a check pass — so it does not read as settled. **The owner rules.**

---

## (e) The same-shaped citations corrected

Scanning every `docs/**` for a cited `tools/`/`scripts/` path that does not resolve on this ref:

- **`tools/meetings/legibility.py` — 7 dangling citations across four documents** (`the-room-reads.md`,
  `the-room-posts.md` ×2, `the-dark-room.md` ×3, `audit-whole-project.md`). The tool was **deleted** when
  the measure was generalised into `tools/frames/frame.py` (`docs/frame-assertions.md`: *"the old
  `tools/meetings/legibility.py` is deleted: one file, one rule"*). Each citation is corrected to the
  surviving path, with a dated note at the head of each document. **Two mentions are left** — in
  `docs/frame-assertions.md:38` and `docs/evidence/the-frame-assertions.md:22` — because they already say
  the tool *is deleted* and are correct as written.
- **`scripts/orders.py` → `tools/orders/orders.py`** in `docs/evidence/the-maintenance-kind.md:168` — the
  file is `tools/orders/orders.py`; `scripts/orders.py` never existed.
- **`patches/0005`** and its siblings are **not** corrected: the repo's convention cites a patch by its
  number (`patches/0005`), and the file carries a slug (`patches/0005-direction-layer-hooks.patch`). This is
  a settled shorthand, not a dangling path — recorded here so a later sweep does not "fix" 24 of them.

---

## What I verified by running it

```
$ bash scripts/away-check.sh ; echo "exit=$?"
... PASS lines ...
exit=0        # the two-way probe passes on the real frame set
$ bash scripts/test.sh ; echo "exit=$?"
all checks passed
exit=0
$ bash scripts/docs-check.sh ; echo "exit=$?"
exit=0
$ bash scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts ; echo "exit=$?"
PASS  the 2 unresolved references are exactly the recorded boundary
exit=0
```

## What I could not verify, and why

- **Nothing here is the owner's to judge.** The two-way probe's *thresholds* are the room measure's; the
  crew-floor question is his (d); the frames are his to look at.
- `scripts/away-check.sh` needs the game data (present, ran); CI does not run it.

## What I deliberately left unbuilt, and why

- **The dark exemption is not removed for the derelict** — that is Item 1's design decision, recorded there.
- **The structure's enclosed facings, the away loop, the other four archetypes** — not this item.

## Judgement calls, named as calls

1. **Two-way means failing, not warning, on the named frame ceasing to be dark** — because the assertion the
   pass makes is that the opening *exists*, and a warning would let its loss pass.
2. **(c) was larger than it looked**, and I say so: it is the one of the five that needed an engine run.
3. **I left the two correct "is deleted" mentions alone**, rather than rewriting every occurrence of the dead
   name — a document that says a tool is deleted is not a dangling citation.
4. **I did not touch the `patches/NNNN` shorthand**, and recorded why, so a later sweep does not rewrite the
   convention.
