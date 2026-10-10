# Evidence: the appearance derivation — the face comes from the same seed

Date: 2026-10-10. Branch `feat/the-appearance`, cut from `testing`, PR back into `testing`. This is
`docs/character-attributes.md`, *Appearance: the face is part of the person, so it comes from the
same seed*, implemented. The design was already written; this pass makes it so.

**Observed, and the command that produced it.** Everything below is the output of a command named
beside it. The unit transcript is `test_ship_core --appearance` and the unit tests are in
`scripts/test.sh`; the rendered half is `scripts/appearance-check.sh`, which drives the engine
headless under `xvfb-run` with the engine's own screenshots.

## Task A — `DeriveAppearance(record, seed)`, beside `DeriveCharacter`

### The pool, enumerated from the shipped data

Not hand-typed: listed off the paks. The command (from `build/baseEF`, the read-only symlink to the
GOG installation):

```sh
python3 - <<'PY'
import zipfile, re, collections
paks=["pak0.pk3","pak1.pk3","pak2.pk3","pak3.pk3"]
dirs=collections.OrderedDict()
for pk in paks:
    with zipfile.ZipFile(pk) as z:
        for n in z.namelist():
            m=re.match(r'models/players/([^/]+)/head[^/]*\.(md3|skin|jpg)$', n)
            if m: dirs.setdefault(m.group(1), set()).add(pk)
print(len(dirs)); print(" ".join(sorted(dirs)))
PY
```

Result: **63 unique player-model directories ship a head asset**, across `pak0.pk3` and `pak3.pk3`
(`pak1`, `pak2` ship none). They are:

```
Garren alexandria alexandria_lt alexascav avatar biessman boothby borgThin3 borgThin4 borgbig
borgbig2 borgbig3 borgbig4 borgfoster borgthin borgthin2 chakotay chang chaoticaguard chell doctor
forge_boss foster generic1 goodheart green hed_borg3 hed_borg_bolian hirogen hirogen_boss imperial
imperial2 imperial3 imperial4 imperial5 imperial6 impfem impfem2 janeway kim klingon klingonfem
mackey malon munro munro_lt munrocrew munroscav neelix oviedo_h paris pelletier proton reaver
scoutbot seven species8472 telsia torres tuvok tuvok_h warbot warbot_boss
```

**The brief said 66; the measured count is 63.** The 66 is the `(directory, pak)` occurrences —
`doctor`, `kim` and `munrocrew` each appear in both `pak0` and `pak3`, so three directories are
counted twice. Corrected here, visibly, rather than left as a number that gets quoted back.

**Where the pool came from, and a second source for the species tags.** The shipped
`ext_data/NPCs.cfg` (pak3, read with `unzip -p pak3.pk3 ext_data/NPCs.cfg`) is the game's own cast
list: it dresses its random Starfleet crew from exactly these parts, and it carries a `race` field.
The game's own **generic crew** draw from eleven heads —

```
chakotay/nelson  doctor/pasty  garren  garren/mackey  garren/salma  generic1
kim/durk  paris/chase  paris/kray  pelletier/generic2  pelletier/klein
```

— and its species blocks name their own: `klingon`/`klingonfem`, `chell` (bolian), `telsia/jurot`
(betazoid), `munro/kenn` (bajoran), `neelix` (talaxian), `tuvok`/`tuvok_h`/`tuvok/vorik` (vulcan).
Our species tags follow that field; where it is absent (a placeholder carve) the code says so.

### What the derivation fills

`module/ship/ship_core.{h,cpp}`. `DeriveAppearance(CrewMember&, uint32_t&rng)` is called from
`BuildRoster` immediately after `DeriveCharacter(c, r)`, **over the same stream**, and again on load
(`Unpack` calls `BuildRoster`). It fills an `Appearance { head, build, colour }`:

- **head** — an index into the shipped pool, drawn with **one** `NextRandom(rng)`: the face comes
  off the same stream as the traits.
- **build** — `crewthin` or `crewfemale`, the record's own, as the game's type ("GoldM3"/"RedF1")
  already carries it.
- **colour** — the department colour, as the player-body path always computed it.

It is **static content, derived and never stored**: `Pack` does not write `Appearance`, so a save
replays identically and a person's face can never drift from their record.

The design names the slots *head, hair, tone, build, uniform, department colour*. The shipped head
is **one part that carries face, hair and tone together** — a separate hair or tone asset does not
exist, and inventing a slot for it would be a slot that draws nothing. The honest reading is
therefore: **head** (the skin carries hair and tone), **build**, and **department colour**. Named
as a call, below.

### The same seed gives the same person, face and all — shown twice, and through a save

The unit test `TestAppearanceDerivation` builds two fresh ships from the same seed and finds every
record identical in **traits and appearance**; a second seed differs in more than ten faces;
and the two derivations are one stream — the same seed, with `DeriveCharacter` before it, draws a
different face from the same seed without it, over twenty seeds.

```sh
$ <build>/test_ship_core --appearance | sed -n '/determinism/p'
  determinism: 141 of 141 records identical on a second derivation from the same seed
```

`TestAppearanceSaveRoundTrip` packs, unpacks, and finds `Pack(back) == blob` **and every face and
trait identical** — the face is not in the save at all, and comes back from the seed.

## Task B — the pool cannot produce a canon face, in code, with a planted failure

The boundary is `IsCanonFace(directory)`, **one place**: the show command crew whose likeness the
generation may not produce (`janeway`, `chakotay`, `tuvok`, `tuvok_h`, `paris`, `kim`, `torres`,
`doctor`, `seven`, `neelix`). Both the derivation and the check call it, so they cannot disagree.

The check `TestAppearancePoolExcludesCanon` **walks the whole pool** — every one of the 52 entries,
not a sample — and asserts `CanonFaceInPool(dirs, n) == nullptr`; asserts the predicate is not
vacuous (it names `janeway`, `Tuvok`, `tuvok_h`; it does not name `Garren` or the empty string);
then **plants a canon head in the pool**:

```c++
const char *planted[257];
for (int i = 0; i < n; ++i) planted[i] = dirs[i];
planted[n] = "janeway";
CHECK(std::string(CanonFaceInPool(planted, n + 1)) == "janeway");
```

and shows the check **catches it by name**. `scripts/test.sh` runs it; the run is green.

Beyond the check, the derivation itself refuses a canon face at the point of selection
(`!IsCanonFace(HEADS[i].dir)`), so even a pool edited by hand cannot put a canon likeness on a
generated crew member. The rendered check greps the headless run's own transcript for any
`.../default` canon head and fails if one appears.

## Task C — species constrains the pool

`SpeciesHeadCount(species)` / `SpeciesHeadAt(species, i)` return a species's own part of the pool,
and the pools are **disjoint by construction** (each head is tagged once). `TestSpeciesConstrainsFacePool`
shows it:

```
  Human           : Garren/default Garren/mackey Garren/salma generic1/default green/default
                    chang/default chang/tom foster/default biessman/default telsia/default
                    munro/default munro/kenn munro_lt/default munroscav/default munrocrew/jar
                    munrocrew/jon imperial/default imperial/paladin imperial2/default
                    imperial3/default imperial4/default imperial5/default imperial6/default
                    impfem/default impfem2/default
  Vulcan          : pelletier/default pelletier/klein pelletier/generic2 oviedo_h/default
                    oviedo_h/csatlos oviedo_h/jaworski mackey/default
  Klingon         : klingon/default klingon/angry klingon/sleep klingonfem/default
                    klingonfem/auburn klingonfem/ugly
  Bolian          : chell/default chell/long
  Betazoid        : telsia/jurot
  Borg-recovered  : borgthin/default borgthin2/default borgThin3/default borgThin4/default
                    borgfoster/default
  Ocampa          : alexandria/default alexandria_lt/default alexascav/default
  Talaxian        : boothby/default proton/default goodheart/default
  Hologram        : (none -- the game ships no non-canon face)
```

The test asserts `Vulcan ∩ Human = ∅`, `Klingon ∩ Human = ∅`, `Bolian ∩ Human = ∅`,
`Betazoid ∩ Human = ∅`; that every generated Vulcan wears a Vulcan face, every human a human one,
every Klingon a Klingon one; and that no species pool reaches a canon face.

**The Klingon pool is visibly Klingon** — the rendered lineup below shows one. **Vulcan, Ocampa and
Talaxian have no shipped non-canon face at all** (the game ships Tuvok/Vorik, Kes and Neelix, and
those likenesses are forbidden). Their pools are therefore a **named placeholder carve of human
faces**, which keeps the species pools disjoint and the derivation total, and is honest about what
it is. This is the one place the design's rule *"a Vulcan draws from Vulcan parts"* cannot be fully
kept: the palette has no Vulcan parts that are ours. Making them is **whole-cloth generation**, the
later step this pass is told not to build. Named as a shortfall, not smoothed over.

## Task D — named crew are untouched

`DeriveAppearance` leaves a named record's `head` unset (`0xFFFF`); the player-body path keeps the
model its type ships, through the same one list (`NamedHeadModel`) that decides the boundary. The
`--appearance` transcript names them one by one:

```
  Kathryn Janeway Human            NAMED  type janeway    head janeway/default (unchanged)
  Chakotay       Human            NAMED  type chakotay   head chakotay/default (unchanged)
  Tuvok          Vulcan           NAMED  type tuvok      head tuvok/default (unchanged)
  Tom Paris      Human            NAMED  type paris      head paris/default (unchanged)
  Harry Kim      Human            NAMED  type kim        head kim/default (unchanged)
  B'Elanna Torres Klingon          NAMED  type torres     head torres/default (unchanged)
  The Doctor     Hologram         NAMED  type doctor     head doctor/default (unchanged)
  Seven of Nine  Borg-recovered   NAMED  type seven      head seven/default (unchanged)
  Neelix         Talaxian         NAMED  type neelix     head neelix/default (unchanged)
  Vorik          Vulcan           NAMED  type vorik      head vorik/default (unchanged)
  Les Foster     Human            NAMED  type Foster     head <the game default> (unchanged)
  Alexander Munro Human            NAMED  type munro      head munro/default (unchanged)
  Rick Biessman  Human            NAMED  type Biessman   head <the game default> (unchanged)
  Austin Chang   Human            NAMED  type Chang      head <the game default> (unchanged)
  Telsia Murphy  Human            NAMED  type Telsia     head <the game default> (unchanged)
  Chell          Bolian           NAMED  type Chell      head <the game default> (unchanged)
  Juliet Jurot   Betazoid         NAMED  type Jurot      head <the game default> (unchanged)
  Kenn           Human            NAMED  type Kenn       head <the game default> (unchanged)
  Odell          Human            NAMED  type Odell      head <the game default> (unchanged)
```

The show crew and Munro keep `<type>/default`; the hazard team (Foster, Biessman, Chang, Telsia,
Chell, Jurot, Kenn, Odell) keeps the game default it always had (their types are not in the named
list, so this pass changes nothing for them — *unchanged*, as required). Two findings are named
rather than fixed, both pre-existing and outside this pass's scope:

- **Vorik has no model directory.** `NamedHeadModel("vorik")` returns `vorik/default`, but
  `models/players/vorik/` does not exist — Vorik's face is the skin `tuvok/vorik`. The named path
  builds a path that falls back, unchanged by this pass.
- **The hazard team wears the game default** (`munro/default`) because their types are not in the
  named list. That is the same residual, on named rather than generated crew.

## Task E — the residual, addressed

`ApplyPlayerBody` (`module/ship/g_ship.cpp`) no longer gives a generated crew member
`munro/default` (or `torres/default`). A generated record walks as its **derived head**, torso and
legs. The old fallback is **removed, not renamed**:

- the named branch is the original code, unchanged: `<type>/default` and the department uniform;
- the generated branch reads `c.appear` and dresses from it;
- a guard remains for an impossible state (a generated record with no derived face): it falls to
  the pool's own first head and **says so in the log** — explicitly *not* a canon default. There is
  no code path left that hands a generated crew member a canon face.

Measured on the rendered run (below): the derived heads are `generic1`, `munrocrew/jon`,
`klingon`, `pelletier`, `oviedo_h/jaworski` — **no `munro/default`**, and the check fails if one
reappears.

## The picture — a lineup of generated crew, photographed

The harness case `g_shipTest 90` finds the Starfleet NPCs already on a deck, re-dresses each from a
generated crew record's derived appearance (setting the render info and re-registering the models),
stands them in a rank in front of the player, and screenshots. Command:

```sh
$ scripts/appearance-check.sh
==> the rendered lineup: the Starfleet crew on tour/deck04, re-dressed from the records
    appearance lineup: Crewman 020 (Human) -> generic1/default, torso crewthin/red, legs crewthin/default
    appearance lineup: Crewman 050 (Human) -> munrocrew/jon, torso crewthin/gold, legs crewthin/default
    appearance lineup: Crewman 080 (Klingon) -> klingon/default, torso crewthin/gold, legs crewthin/default
    appearance lineup: Crewman 110 (Vulcan) -> pelletier/default, torso crewthin/blue, legs crewthin/default
    appearance lineup: Crewman 141 (Vulcan) -> oviedo_h/jaworski, torso crewthin/blue, legs crewthin/default
```

Screenshot: `build/g3-home/baseEF/screenshots/lwh_appearance_lineup.tga` (1280x1024, the engine's
own). **Which frames I looked at:** `generic1/default` (a bearded man, red-collar command); the
Klingon `klingon/default` (ridges visible — the species is the point and it reads); `munrocrew/jon`;
and the two Vulcans `pelletier/default` and `oviedo_h/jaworski`. Read at 1280x1024: five crew in a
rank in the deck-4 room, the Klingon's forehead ridges unmistakable, no two faces alike, no obvious
duplicate of a named crewmate in the frame.

**And the judgement, which is not mine.** Whether the rank *reads as people* rather than as a parts
bin is the owner's eye, and it is the only question that matters here. My reading above is offered
as a reading, not a verdict.

## Checks

```sh
$ scripts/test.sh          # includes TestAppearanceDerivation / ...PoolExcludesCanon / ...SaveRoundTrip
                            # and every other check: all pass
$ scripts/hooks-check.sh
hooks-check: module/ship/ship_core.h declares 405 public functions; docs/hook-register.md registers 405 hooks
PASS  every public function is registered, and every registered hook exists
$ scripts/api-check.sh
PASS  every console verb is in the ship's API, and every API verb is answered by the console
$ scripts/appearance-check.sh
PASS  ...
$ scripts/check.sh ...     # unchanged; not touched by this pass
```

## What could not be verified

- **Whether the crew read as inhabited.** The owner's eye, on the lineup above; a check cannot
  answer it.
- **Whether the placeholder Vulcan/Ocampa/Talaxian faces are acceptable.** They are human faces on
  non-human species, because the palette has no non-canon part for those species. The owner's call.
- **Performance.** `docs/character-attributes.md` requires a full crew of derived bodies to be
  counted against the ceiling before it is promised. This pass puts **one** derived body per
  Starfleet NPC already placed by the map; it does not add bodies. The count is unchanged, so the
  survey's ceiling still governs — but a full crew of derived bodies, spawned, has not been
  measured here.
- **Mode 2 (retail multiplayer).** Untouched by construction: this is the single-player game module
  and UI.

## Judgement calls, named as calls

1. **The slots are head, build and department colour** — not hair and tone, which the one shipped
   head skin carries together. Naming a slot that draws nothing would be worse than saying so.
2. **The canon set is the television cast, by directory.** The brief's boundary is *the show
   command crew*; the game's own characters (Biessman, Chang, Foster, Telsia, Chell, Jurot, Kenn,
   Odell, Munro) are Raven's and stay in the pool, per the brief's own list. Consequence: a
   generated crew member may share a face with a named *game* crew member. Named as a call; a
   narrower pool (excluding the game's named types too) is a one-line change.
3. **Vulcan, Ocampa and Talaxian draw from a named placeholder carve of human faces**, because the
   game ships no non-canon face for them. The carve (pelletier/oviedo_h/mackey; alexandria*; boothby/
   proton/goodheart) is a call, recorded so it can be overruled.
4. **Species with no pool fall back to the human pool** in the derivation (nothing emits Hologram,
   so it is dormant). Named in the code; the fallback is total, never a canon face.
5. **The game's type carries the build** (M/F in "GoldM3"/"RedF1"), read case-insensitively — the
   player-body path's older test missed `gold` and `blue` because they are lower-case; the
   derivation reads them. Named crew keep the old test, unchanged.
6. **`NamedHeadModel` is the one list of named types**, moved into the model so the unit transcript
   and the player-body path cannot disagree. Behaviour is unchanged for every named record.
7. **The guard on the impossible state uses the pool's first head and logs it**, rather than
   silently renaming the old default.
8. **The lineup re-dresses the map's placed NPCs** rather than spawning new ones: the mechanism
   exists, the bodies are already on the deck, and nothing is added to the ship's body count.

## Files

- `module/ship/ship_core.h` — `Appearance`, the appearance API, `NamedHeadModel`.
- `module/ship/ship_core.cpp` — the pool table, `IsCanonFace`, the species pools, `DeriveAppearance`,
  `NamedHeadModel`; the `BuildRoster` call.
- `module/ship/g_ship.cpp` — `ApplyPlayerBody` rewritten; `IsNamedType` delegates; harness case 90.
- `tests/ship/test_ship_core.cpp` — the four appearance tests and `--appearance`.
- `scripts/appearance-check.sh` — the transcript and the rendered check.
- `docs/hook-register.md` — thirteen new hooks.
- `docs/character-derivation.md`, `docs/character-attributes.md`, `docs/gates.md` — the derivation
  written down, the design marked built, the ledger updated.
