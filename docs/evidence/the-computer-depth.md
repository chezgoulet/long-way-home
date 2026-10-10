# The computer as somewhere to dive in — a session, scoped answers, the API applied, and the price of autonomy

`docs/staff-meetings.md` (the owner's design of 2026-10-10, "the computer as somewhere to dive in",
and the tools ruling beneath it), `docs/evidence/the-computer-api.md` (M6, the one-shot brief this
pass deepens), `docs/evidence/model-call.md` (M4, the novelty seam and the verdict index),
`docs/access-and-authority.md` (`PersonalVisibleTo`, `LogScopeForCrew`, `MayCommand`,
`PlayerMayCommand`, `PlayerMayOperate`), `docs/the-record-and-the-log.md` (the two logs), and
`docs/power-assignment.md` (the delegation shape autonomy would have to copy — named here, not
built), on `feat/the-computer-depth` (cut from `testing`), 2026-10-10.

**Observed, and the command that produced it.** Everything below is the output of a command named
beside it, on this tree, after
`cmake --build ../upstream/efgame/build-linux --target efgame --target efui`. The engine runs are
headless (`xvfb-run`, `SDL_AUDIODRIVER=dummy`) on `sasquatch`; the frames are rendered, not inferred.
No model, cache, audio or generated content is committed. **The model is never called by the engine**:
the computer's classification is the API's membership test, and nothing here writes to a model.

## What was built, and where

| file | what it is |
|---|---|
| `module/ship/ship_core.{h,cpp}` | the computer **deepened**: `AddressComputer` now answers from the ship's state, scoped by the records' own rule, and applies an act the asker is cleared for; `ComputerRefusal` and its five named kinds; the pills become the pass's knowledge topics; `SystemIdByName`, the one place the console and the computer resolve a system name. |
| `module/ship/g_ship.cpp` | the host half: the session (`computerSession`, published as cvars), the refusal kind published and logged, and `g_shipTest 85` rewritten to demonstrate A–D. |
| `module/ui/ui_lwh_meeting.cpp` | **the same overlay, edited not replaced**: the room draws the computer's transcript rather than one line, and a computer pill asks at once and the room stays open. There is no second UI. |
| `tests/ship/test_ship_core.cpp` | `TestTheComputer`: the read, the applied act, and all the refusals, without an engine. |
| `scripts/computer-check.sh` | rewritten: one headless run of the session, the scope by contrast, the applied request, the three refusals, and the call counts. |
| `docs/hook-register.md` | two readers added (`SystemIdByName`, `ComputerRefusalName`), both reached in play; the counts re-derived. |

## Task A — a session, not a shot

The computer's room is the meeting's own (`MEET_COMPUTER`), and the change that makes it a session is
two lines of behaviour, not a new screen. A pill or a typed line asks at once and the room **stays
open**; the host keeps a bounded run of exchanges (`computerSession`, up to `COMPUTER_TURN_MAX`),
publishes it as `lwh_ship_meeting_turn<i>`, and the same overlay draws it. Three exchanges in one
sitting, driven by the overlay's own keys (`lwh_meet_key`), which are exactly what a hand presses:

```
    computer test: session: 3 exchange(s) in one sitting
    computer test:   ask "status" -> alert green, alpha watch, 141 of 141 fit; 1350 power available, 1350 committed (short 0); deuterium 100%, rations 100 days
    computer test:   ask "jobs" -> no jobs outstanding: the crew have no work waiting
    computer test:   ask "chart" -> beacon 0 of 11 in sector 1; 1 of 12 charted; here is open space
    computer test: the room is still open after 3 exchange(s): yes
```

The frame is `build/g3-home/baseEF/screenshots/lwh_computer_session.tga` (not committed): the rail
reads `THE COMPUTER - ASK IT ANYTHING`, the transcript shows the three exchanges, the four pills are
below it, and the free-text pill carries `VOICE`. The engine's version stamp is not overlapped —
measured, not asserted:

```
$ python3 - .../lwh_computer_session.tga
lwh_computer_session stamp box colours: 1 [(0, 0, 0)]
```

(one uniform colour in the virtual box 371..500 × 444..456, all three frames.)

## Task B — it answers from what it knows, and only what the asker may see

The computer answers about the ship's state and its records: `status` (the ship and every system),
`log` (the official log, scoped), `personal` (the private log), `crew` (the roster and the posts),
`chart`, `jobs`, `dilithium` (the stores), `nav` (the standings), `patients` (the ward). **The
scoping is the records' own rule, reused and not rewritten**: `PersonalVisibleTo` decides the private
log, `LogScopeForCrew` (via `MayCommand`) the official one.

**Demonstrated by contrast — the same question, two ranks.** Reyes (a rank-2 command junior) asks for
the captain's private log; the captain asks the same question:

```
    computer test: the junior asks "personal Janeway" -> not yours: that is Kathryn Janeway's private log, and it is not yours to read
    computer test: the same question is now asked by Kathryn Janeway (rank 6)
    computer test: the captain asks "personal Janeway" -> answered: your private log, newest: the captain's own words, and nobody else's
```

The junior's is **refused in character** — not silently blanked, not answered anyway. The official log
scopes the same way (command reads all, a post its own):

```
    computer test: the captain reads the log -> the official log (all scopes): ...
    computer test: the junior reads the same log -> the official log (command): ...
```

The unit test holds the same property without an engine (`PersonalVisibleTo` path via
`AddressComputer`).

## Task C — the API is reachable by asking, and the refusals are distinct

A request inside the enumerated set reaches the simulation and is answered, whether it arrived as a
pill or as typed prose — membership is `FindConsoleVerb`, the same test for both. **A recognised act
the asker is cleared for is applied by the simulation**, and the state change is the proof:

```
    computer test: the junior asks "alert red" -> not cleared: that is not yours to call: the alert is called by a lieutenant or above, at Engineering or Tactical, or by command
    computer test: the alert is unchanged at index 0 (the permission changed nothing)
    computer test: the captain asks "alert yellow" -> answered: the alert is set to yellow; the alert is now index 1
```

The acts the computer will run are `alert`, `on`, `off` and `priority`; the clearance is the console's
own (`PlayerMayCommand`, `MayCallAlert`/`PlayerMayOperate`, with delegation, override and lock-out
already folded in). **Three refusals, distinct and named** (`ComputerRefusalName`), and the difference
is the point:

1. **not a function** — outside the enumerated set: the canonical *"that function is not available"*,
   and the ship's state is byte-identical across it (the refusal writes nothing):

   ```
       computer test: out of set "warpnine" -> not a function: that function is not available
       computer test: the ship's state is byte-identical across the refusal: yes
   ```
2. **not cleared** — inside the set, beyond the asker's clearance: a permission, refused in character
   (Reyes and `alert red`, above).
3. **not yours** — inside the set, a record whose visibility is not the asker's: a permission, decided
   by `PersonalVisibleTo` (Reyes and the captain's private log, above).

A fourth kind exists for completeness — **not exposed**, a verb inside the set that is not a function
the ship exposes to the computer (`ignite`, `board`, `damage` and the rest of the developer tail) — the
console's own *"that is not a console's to do"* in the computer's voice. It is named and unit-tested;
the brief's acceptance needs three and they are the three above.

## Task D — autonomy, named and priced, not built

The owner's words are that exposing the systems would *"facilitate the computer entity's relative
autonomy."* Under the ruling, **autonomy is a delegation and not a discretion**, and the pattern to
copy is the power system's (`GrantBand` / `RevokeBand` / `SetPowerAuto`, `docs/power-assignment.md`).
**It is not built.** What it would need, priced:

- **Who may grant it.** Command, and only command — the same hand that grants a *band*, and for the
  same reason: the authority to decide is not the crew's to give away (`MayGrant`, `docs/access-and-authority.md`).
- **What it covers.** A named scope from the enumerated set — a band of reads (the ship's own
  instruments) and, if ever, a narrow band of acts — held by a **name** (the entity, not a person),
  bounded in time like a delegation's shift.
- **How it is refused.** Without a standing grant, an unasked act is refused as **not cleared** — the
  same permission refusal above, and the same code path. The entity may not act unasked *because it
  has no clearance*, not because a flag forbids it; the refusal is the existing one.
- **How it is revoked.** Immediately, by the grantor, and recorded — `RevokeBand`'s shape: the log
  names who held it and who turned it off.
- **What would have to be true for the ship to act unasked without a model writing state.** The
  decision would have to be the simulation's own and deterministic — a threshold the ship's rules
  already cross (the reserve, the beds, the Borg awareness) — and the act one of the enumerated set,
  applied by the simulation as a pill's outcome is. **The model would still only select from the
  enumerated outcomes; it would never write state.** A record that shows the grant, the moment and the
  act is what keeps "the ship acted" the same claim as "the player acted". Where the model would enter
  at all is the selection, and the standing rule holds there too.

The cost is honest and worth stating: an entity that may act on a threshold is an entity whose actions
the player must read afterwards. That is the delegation's price, and it is the reason the pattern is
grant-and-record rather than a switch.

## Task E — the model calls nothing

The computer's classification is the API's membership test, and the engine makes no model call:

```
    computer test: the classification path is the API's membership test (FindConsoleVerb), not the embedding seam; model calls=0
    computer test: addressed 10 time(s), inference calls=0; nothing invented: yes
```

The novelty seam (`ClassifyNovelInput`, M4) is the *meeting's* classifier, because a meeting's options
are intent descriptors matched by embedding and need a threshold. The computer's set is exact — a verb
is in it or it is not — so its membership test is `FindConsoleVerb` and there is no threshold to
tune and no model to call. The voice remains the pre-generated authored line
(`pre-generated during generation: 4 line(s), voice computer, key 654dd3802aa23ef2, inference calls=0`);
the state reading is what the screen and the log carry. The log's record of every exchange:

```
    computer test: log [command] the computer: the computer applied: alert yellow
    computer test: log [command] the computer: the computer refused (not cleared): alert red -- ...
    computer test: log [command] the computer: the computer refused (not a function): warpnine -- that function is not available
    computer test: log [command] the computer: the computer refused (not yours): personal Janeway -- that is Kathryn Janeway's private log, and it is not yours to read
```

## No second UI

The computer is drawn by `module/ui/ui_lwh_meeting.cpp`, the meeting overlay's own file, edited not
replaced — one transcript function, and one branch in the key handler. The check asserts it by file
count: `find module/ui -maxdepth 1 -name 'ui_lwh_meeting*.cpp' | wc -l` is `1`.

## The transcript wraps — the fourth surface (corrected 2026-10-10)

The frame this pass produced, `lwh_computer_refused.tga`, showed the refusal **clipped** at the frame
edge: the transcript read `>> ALERT RED -> THAT IS NOT YOURS TO CALL: THE ALERT IS CALLED BY A` and
lost the half that explains itself, and the rail's echo of the same line was clipped too. The
transcript and the echo are drawn on one line and wrapped by a character count, not by measurement —
the same mistake the configurator and the meeting pills had shipped before.

Fixed in the wrap pass: the transcript and the rail's echo now call the one measured wrap
(`module/ui/lwh_textfit.h`, `LWH_WrapText`), as every other line on the overlay does. The refusal
reads whole in both places, and no line is wider than its column, proved in the engine's own font:
`scripts/computer-check.sh` now asserts `WRAP: checked … overflows 0` and that the refusal's own end
is a wrapped line of both the transcript and the rail's echo. The frames are regenerated. The full
record is `docs/evidence/the-wrap.md`.

## The commands, and what they returned

```
scripts/computer-check.sh       -> PASS (the session, the scope by contrast, the applied request, the
                                   three refusals distinct and named, the call counts, three frames)
scripts/test.sh                 -> all checks passed
scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
                                -> the entity dictionary and the script corpus as before (rc 0)
tests/ship/test_ship_core       -> ship_core: all checks passed (including TestTheComputer)
scripts/hooks-check.sh          -> PASS (431 public functions, 431 registered hooks)
scripts/api-check.sh            -> PASS (the console's verbs and the API agree, both directions)
scripts/meeting-check.sh        -> PASS
scripts/meeting-overlay-check.sh -> PASS
scripts/meeting-place-check.sh  -> PASS
scripts/room-survey-check.sh    -> PASS
scripts/model-call-check.sh     -> PASS
scripts/audio-check.sh          -> PASS
scripts/voice-in-check.sh       -> PASS
scripts/order-check.sh          -> PASS
scripts/appearance-check.sh     -> PASS
```

## What could not be verified

- **Whether being answered by the computer *feels* like talking to a ship rather than asking a
  database.** That is the owner's, and it needs the picture (above). This lane can show the screen,
  the transcript and the counts; it cannot judge the read.
- **Whether four pills is the right number.** The pill row is a curated subset of the ten reads; a
  computer that answers to free text addresses all of them. A scrolling list of ten is a different
  screen, and this lane did not build one.
- **Whether the acts the computer runs are the right four.** `alert`, `on`, `off`, `priority` are the
  console's simplest controls and the ones whose clearance is unambiguous; a longer act set is a
  curation question against the enumerated set, and this lane stopped at four so the list can be
  argued.

## Judgement calls, named as calls

1. **A curated read/act surface, not the whole API.** The enumerated set is the membership test; the
   *exposed* surface is ten reads and four acts, named in the header. A developer verb inside the set
   is refused as **not exposed**, the console's own *"that is not a console's to do"*. Curating by
   exclusion is a call; the alternative (expose all 129) would put `ignite` and `board` behind a
   spoken word.
2. **The scope refusal is a permission, and it is separate from the clearance refusal.** Both are
   permissions, but they are different locks: `LogScopeForCrew`/`PersonalVisibleTo` decide a *record's*
   visibility, `PlayerMayOperate`/`PlayerMayCommand` an *act's* reachability. Collapsing them would lose
   the distinction the brief asks for, so they are two named kinds.
3. **The pills are the pass's knowledge topics, and there are four.** The M6 pills were six API verbs;
   a session's pills read better as the things you ask. The number is not fixed and is named as a call;
   free text reaches the rest.
4. **The computer's spoken line and its readout are different things.** The pre-generated voice is a
   short authored acknowledgement (so it stays valid and pre-renderable); the state reading is the
   transcript and the log. A state-dependent answer cannot be pre-rendered, and pretending otherwise
   would be the promise the design cannot keep.
5. **The classification is `FindConsoleVerb`, not the embedding seam.** For a meeting, novelty is a
   threshold problem and needs a model; for the computer the set is exact and the membership test *is*
   the classification. This is why the model calls nothing, and it is stated rather than assumed.
6. **The byte-identical proof is on `AddressComputer`, not the host path.** The host path logs the
   refusal (the record carries what was said, which is state); the invariant that the refusal writes
   nothing is proved across the simulation call itself, exactly as M6 did.
7. **The session is host state, never in the save.** The transcript is UI state beside the save, like
   the voice cache; the save blob is unchanged and a save replays identically. The session is cleared
   when the room opens.
8. **`SystemIdByName` is one place, and the console now uses it.** `g_ship.cpp`'s `FindSystem` was the
   only resolver; the computer needs the same resolution for `on shields`, so the console was pointed
   at the new function rather than a second copy being written.

## The boundary, kept

No model file, no cache, no generated content and no audio is committed. Nothing was pulled and no
dependency was added. No second visibility rule, no second UI, and no model that calls anything. The
delegation is named and priced above and **not built**. Nothing is committed to `main`.
