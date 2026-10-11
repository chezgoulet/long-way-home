# The 126 script references — the evidence

The brief (`BRIEF.md`, this pass) takes the ledger's oldest open residual, in `docs/gates.md`'s
*Residuals* list, and asks for it to be taken apart:

> *"**126 script references the GDK corpus lacks** — the validator found them; most likely the
> expansion's scripts. Worth resolving before G2 content work."*

The finding is one line, carried since 2026-10-04, and it is the mirror of
`docs/evidence/g0-script-compiler.md` (scripts going *in*): references going *out*. This file is the
measurement. **The count has moved, the class the ledger guessed is wrong, and the references the
GDK genuinely lacks are not scripts at all.** Everything below is the output of the command beside
it. This is a static host: no engine ran, no game data was touched.

## 1. The measured list

The number was never reproducible from the repository — it came from a one-off scenario declaring
every published map source and the shipped script corpus, and no script carried it. Rebuilt with
`scripts/fetch-gdk.sh` content (11 map-source archives, 106 `.map` files; the script corpus), the
validator reports, in `usescript` references (its definition — see §6):

| corpus | declaration | map-reference pairs | distinct references |
|---|---|---|---|
| **2,024** files — `fetch-gdk.sh` before this pass | `scripts/**/*.txt` | **240** | 228 |
| 2,024 files | `scripts/**/*.[tT][xX][tT]` | 182 | 172 |
| **2,408** files — the GDK's own script tree | `scripts/**/*.txt` | **63** | 60 |
| 2,408 files | `scripts/**/*.[tT][xX][tT]` | **3** | **2** |

```
$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
…
==> compiler corpus pass (every shipped script: compile, then read back)
    2024 files: 2016 compiled and read back, 8 rejected by the compiler, 0 read-back failure(s)
```

The ledger's **126** reconciles as **2 × 63**: the run that produced it globbed the map sources with
two overlapping patterns, and its own evidence records the doubling — *"4 × E001 unknown entity
class — the two known gaps … reported twice because that manifest globbed maps with two overlapping
patterns"* (`docs/evidence/g0-validator.md`). The validator de-duplicated path expansion in that
same pass, which is why 126 cannot be produced today; 63 is the same run with the doubling removed,
against the same corpus. **This is a reconstruction and is named as a judgement call** (§7.1).

**Reconciled and corrected in place.** `docs/gates.md` carried `126`; it now carries the boundary in
§4. `docs/evidence/g0-validator.md` — the document that owns the finding — keeps the finding and
gains the resolution.

## 2. The method

The census is `tools/validator/reference-census.py`, run by `scripts/references-check.sh`. It builds
the scenario (the map sources and the corpus, `usescript` references), runs the validator over it
twice — once with a case-sensitive `*.txt` declaration and once case-insensitively — and separates
what the validator calls missing into the classes in §3. The validator is the authority for the
counts; a second, direct index of the corpus proves every reference falls in one class and that the
two agree. Planted reference in §5.

```
$ scripts/references-check.sh
map root: …/build/gdk/maps
corpus:   …/build/gdk/scripts (2408 script files)
references in the map sources: 1145 distinct (usescript), in 1251 map-reference pairs
  exact path in the corpus:   1131
  basename elsewhere only:    12
  missed only by the `.txt` glob: 58
  genuinely absent (the boundary): 2
  validator: `scripts/**/*.txt` -> 60 unresolved; `scripts/**/*.[tT][xX][tT]` -> 2 unresolved
PASS  the 2 unresolved references are exactly the recorded boundary
```

## 3. The classification — every reference, by class

Of **1,145 distinct reference keys** in the published map sources:

### exact — 1,131

The reference's path is in the corpus. Nothing to do. (`bs_run_and_shoot` and the basename class
below are the exceptions; everything else is here.)

### basename elsewhere only — 12

The reference's *path* is not in the corpus, but a script of that name is. The validator's
`or os.path.basename(key) in script_names_by_base` clause (`tools/validator/validate.py`) resolves
them. That is leniency, not proof: the game loads `real_scripts/<the path the map names>`, so a
basename match somewhere else may be a different script. All twelve are Holodeck/Virtual Voyager
content where the map's path and the GDK's source path disagree:

| reference | map | corpus file |
|---|---|---|
| `minigame/beam_out` | `Tour/_holodeck_minigame_tex.map` | `_holodeck_minigame/beam_out.txt` |
| `minigame/c1_to_spot1` | same | `_holodeck_minigame/c1_to_spot1.txt` |
| `minigame/cam_start` | same | `_holodeck_minigame/cam_start.txt` |
| `minigame/clear_runto` | same | `_holodeck_minigame/clear_runto.txt` |
| `minigame/lift_platform` | same | `_holodeck_minigame/lift_platform.txt` |
| `minigame/overhere` | same | `_holodeck_minigame/overhere.txt` |
| `minigame/rotate` | same | `_holodeck_minigame/rotate.txt` |
| `minigame/rotate_main_stairs` | same | `_holodeck_minigame/rotate_main_stairs.txt` |
| `minigame/runto_door1` | same | `_holodeck_minigame/runto_door1.txt` |
| `minigame/runto_door2` | same | `_holodeck_minigame/runto_door2.txt` |
| `null` | `deck05.map` | `deck04/null.txt` |
| `runshoot` | `voy16.map` | `_holodeck_highnoon/runshoot.txt` |

### present in the corpus, missed only by the case-sensitive glob — 58

The GDK ships the script tree as both `foo.txt` and `Foo.TXT` (2,408 files, 212 of them `.TXT`).
`glob.glob` is case-sensitive on Linux, so a manifest declaring `scripts/**/*.txt` — the convention
`docs/scenario-manifest.md` teaches — silently drops every `.TXT` file, and `E002` then names the
reference rather than the glob that missed it. Every one of these is *in* the corpus:

```
borg1/2for1 borg1/alcovevengeance borg1/defendmachine borg1/die_munro_die borg1/killsplat
borg1/nodedestruct borg1/nodedestruct2 borg1/noderepair borg1/pipemachine borg1/protect
borg1/protect2 borg1/protect2a borg1/protecta borg1/sffleeleave borg1/splatshift
borg1/strutdestruction borg1/vengeance borg1/yeah_get_some borg2/bridge_el borg2/deactivate
borg2/el_one borg2/el_two borg2/machine borg2/sam1 borg2/walkway borg4/vengeance_alcove
borg5/betrayed borg5/nodedestruct deck01/chakotay2 deck01/janelookaway deck05/doctor_use
deck09/worker1_working deck11/panel1 deck11/panel2bad dn6/vengeance forge5/vengeance
forge5/vengeance_forward scav2/bridgeguymad scav2/kneckalarm scav2/vengeance voy2/explode1
voy2/force_on voy2/im_dead voy2/running_man voy2/worker1_working voy2/worker2_boom
voy2/worker2_stopfire voy3/panel1 voy3/panel2bad voy4/ordermunro1 voy4/runtosafetyfailed
voy4/secondoffense voy4/secondoffense2 voy4/spawnattackers1 voy4/stealbox1 voy4/telsia2
voy6/doctor_use voy6/paris_use
```

### genuinely absent — 2 (the boundary)

| reference | where | what it is |
|---|---|---|
| `bs_run_and_shoot` | `usescript` in Voyager `voy16`, `voy4`; Forge `forge5`; Scav `scav2`; Stasis `stasis2`; Dreadnought `dn5` | a **behaviour state**. The maps use `SET_BEHAVIOR_STATE "BS_RUN_AND_SHOOT"` in their own scripts (`_holodeck_highnoon/runshoot.txt`, `scavboss/showoff.txt`), and then put the same string in a `usescript`/`spawnscript`/`painscript` key. It is not, and was never, a script. |
| `newearthhome` | `usescript` in Virtual Voyager `deck08` | a **`ref_tag` targetname**. The entity is `"classname" "ref_tag" / "targetname" "newearthhome"`; the same name is a `$tag( "newearthhome", ORIGIN )` in the deck08 scripts. A tag, not a script. |

These are malformed fields in third-party shipped maps that the game ignores (a `ref_tag` has no
`usescript` behaviour; an NPC's script key is not a behaviour state). **They are not the expansion's
scripts, and no corpus spelling satisfies them.** The ledger's guess — *"most likely the expansion's
scripts"* — is wrong: none of the eleven map-source archives is expansion content, the scripts that
were "missing" were base-game Virtual Voyager and Holodeck content the GDK ships all along, and the
last two are not scripts.

## 4. The resolutions

**a. `scripts/fetch-gdk.sh` restored 2,024 of the GDK's 2,408 scripts.** The `Level Scripts` archive
it unpacked is a strict subset of the tree the installer itself carries: 384 files — the Virtual
Voyager deck scripts and most of the Holodeck programmes — were downloaded and never laid down. The
validator, fed that corpus, reported ~170 references "the corpus lacks" that the GDK has. The fetch
now lays the installer's own copy over the archive:

```
$ scripts/fetch-gdk.sh
…
    scripts:             2408 files
```

**b. `scripts/check.sh` now expects the fourteen corpus rejections, not eight.** With the whole
tree, the corpus pass is 2,394 compiled and read back, 14 rejected: the three scripts named in
`docs/evidence/g0-script-compiler.md` (`voy1/scene7.TXT`, `voy1/scene10.TXT`, `voy5/beamstart.TXT`)
and eleven files that are not scripts — the five the check already knew, plus
`_holodeck_camelot/behaved_francais.txt`, `_holodeck_highnoon/behaved_deutsch.txt`,
`_holodeck_highnoon/behaved_francais.txt`, `_holodeck_temple/behaved_deutsch.txt`,
`_holodeck_temple/behaved_francais.txt` and `borg3/behaved.txt`. The first five are sound-precache
tables; `borg3/behaved.txt` is a source-control listing (`$/StarTrek/BaseQ3/real_scripts/…`). These
match the compiler evidence's own *2,394 / 11 non-scripts / 3 rejected* exactly — that document was
right and the ledger's 2,016/2,024 was the short corpus.

```
$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
…
==> compiler corpus pass (every shipped script: compile, then read back)
    2408 files: 2394 compiled and read back, 14 rejected by the compiler, 0 read-back failure(s)
    the 14 rejections are exactly the known, documented set
```

**c. Recorded, not fixed, the two-malformed-reference boundary.** They are in maps we do not own and
must not edit; the reason for each is recorded here and in the guard.

**d. `docs/scenario-manifest.md` gains the case rule.** The glob it taught (`scripts/**/*.txt`) is the
trap that manufactured the phantom references; the doc now names it and gives the
case-insensitive spelling.

**e. `docs/gates.md` corrected in place** — the residual, and the GDK-closed entry's corpus figures
(2,016 of 2,024 → 2,394 of 2,408), with the correction shown rather than quietly overwritten.

## 5. The guard

`scripts/references-check.sh` fails when the unresolved set is not exactly the recorded boundary —
grows (a new dangling reference) or shrinks (the record is stale). It is wired into `scripts/check.sh`
as the *published map sources* step. **Planted and caught:** a dangling `usescript` added to a copy of
`_brig.map`, all other maps symlinked in place:

```
$ printf '…"usescript" "lwh/planted_dangling_reference"…' >> …/brig-map/_brig.map
$ scripts/references-check.sh --map-root …/plant/maps --corpus build/gdk/scripts
…
  genuinely absent (the boundary): 3
    bs_run_and_shoot   -- …
    lwh/planted_dangling_reference   -- UNKNOWN
    newearthhome   -- …

FAIL  reference(s) the corpus does not satisfy and the record does not name:
      lwh/planted_dangling_reference   (referenced by maps/brig-map/_brig.map)
$ echo $?
1
```

Restored (the planted map is a temporary copy; `build/gdk/maps` is untouched). The guard also has
real work to do beyond the plant: it is exactly the check that would have caught the corpus 384
files short, which is the regression that produced the residual.

## 6. The definition — what counts as a reference

The validator counts **`usescript` only** (`USESCRIPT = re.compile(r'"usescript"\s+"([^"]+)"')`).
The published maps carry other script-valued keys, and far more of them:
`spawnscript` (2,274), `painscript` (385), `ffdeathscript` (134), `angerscript` (82), `awakescript`
(68), `deathscript` (59), `touchscript`, `attackscript`, `blockedscript`, `endscript` and others —
and the maps put the same malformed values in them (`"spawnscript" "BS_RUN_AND_SHOOT"`). Widening
the validator's definition would be a behaviour change to a tool with stable error codes, across
keys the entity dictionary does not uniformly mark as script-valued; it is **not** done here, and
the broader set is named so the next pass can rule on it (judgement call, §7.3). The residual's
126 was a `usescript` number, and the census answers the same question.

## 7. What could not be established — and the judgement calls, named

1. **The 126 is a reconstruction, not a reproduction.** The validator that produced it de-duplicates
   overlapping globs and no longer can be run as it was; 2 × 63 is what the evidence's own E001
   doubling supports. I state it as the arithmetic that fits, not as observed.
2. **Whether the two malformed references resolve at run time** would need the retail paks: no game
   data is on this host. The reading is from the map sources themselves — one is a behaviour state,
   one a targetname — so they should be inert, but that is a reading, not a session.
3. **The reference definition was not widened** to `spawnscript` and friends (§6). Named, not decided.
4. **The twelve basename-only references may be genuine runtime mismatches.** The validator's
   basename fallback resolves them; the game does not resolve scripts by basename. Whether the
   shipped paks lay the Virtual Voyager and Holodeck scripts at the path the map names (matching the
   map) or at the GDK's source path (`_holodeck_minigame/…`) needs the paks. Flagged for whoever next
   loads a Holodeck programme.
5. **"Expansion" was never established as a category here.** None of the eleven map-source archives
   is expansion content; the ledger's guess came from the same incomplete corpus, not from a source.

## The checks

- `scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts`
  — **exit 0**: dictionary 318 classes; negative tests all pass; corpus 2,408 files (2,394 compiled
  and read back, 14 known rejections); the 2-reference boundary unchanged.
- `scripts/references-check.sh` — **exit 0** (boundary); **exit 1** with a planted dangling
  reference, restored.
- `scripts/test.sh`, `scripts/docs-check.sh` — **exit 0**. `PATH` carries
  `/home/robot/.cache/lwh-venv/bin` first, or `test.sh` fails for a missing `cmake`.
