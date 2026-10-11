# The crew's manner: the state selects a cue, wired where the state changes

`docs/character-derivation.md` section 7 (the manner, the second half of the walkthrough's **G14**),
`docs/character-attributes.md` (the four kinds and the derivation), `docs/morale.md` (the three
components) and `docs/staff-meetings.md` / `docs/programme-meetings-and-voice.md` (the voice mechanism
that already exists), applied on `feat/the-manner` (cut from `testing`), 2026-10-11.

**The finding this closes**, from `docs/gates.md`, S4's remainder: *"What remains is content, not
rules: the crew's **manner** (barks wired to morale — needs the crew layer, session to judge)…"* The
rule was designed (`MannerLine`) and the crew layer was built; **what was missing was the wiring that
makes a person's state audible and visible in the world.**

**Observed, and the command that produced it.** Everything below is the output of a command named
beside it, on this tree. This is a **static host**: no engine run and no game data, so the
**simulation half is real and driven headlessly** (`scripts/manner-check.sh`, which compiles
`tests/ship` and runs `test_ship_core`), and the **engine half** — the `ship manner` console verb and
the played audio — is compiled into the module and named here, not driven. No audio, no game assets
and no generated clip are produced or committed.

## What was built, and where

One mechanism in `module/ship/ship_core.{h,cpp}`, beside the record and the manner it extends: the
selection (`MannerFor`), the line per manner (`MannerLineFor`), the emission (`EmitManner`), and the
hooks. It is in `ship_core` rather than a new translation unit for the reason recorded in
`docs/evidence/meeting-brief.md`: the module build finds our sources with a configure-time glob, and
the save format lives beside the state. No new `.cpp` was added, so the glob trap does not apply.

- `module/ship/g_ship.cpp` — `ship manner [crew]`, the console read and emission (the engine surface).
- `module/ship/console_api.def` — the `manner` row, so `scripts/api-check.sh` holds both directions.
- `tests/ship/test_ship_core.cpp` — `TestMannerSelection`, `TestMannerWiring`,
  `TestMannerSurfaceAndAbsence`, `TestMannerDeterminismAndSave`, and the two demonstration modes
  (`--manners`, `--manners-broken`).
- `scripts/manner-check.sh` — the check, wired into `scripts/test.sh`.

## The selection: state → manner, demonstrated

`scripts/manner-check.sh` (`test_ship_core --manners`). One person, the whole state read: the morale
band, the conditions, an injury, and the ship's alert. The simulation's own output, quoted:

```
== the crew's manner: state -> a cue (G14's second half; docs/character-derivation.md)
  the selection is wired: the whole state
  one person: Vorik, Vulcan; wants to prove something; afraid of dying alone
  steady      -> steady           morale 0.94 (fit): "Give me the hard one. I want to be the one who did it."
              why: the morale band (fit): carrying on
  tired       -> tired            morale 0.94 (fit): "I'll stand the watch. I just need to sit down first."
              why: exhausted: hasn't slept enough
  afraid      -> afraid           morale 0.94 (fit): "At stations, I just keep my head down and my hands moving."
              why: afraid: the ship is at battle stations
  grieving    -> grieving         morale 0.94 (fit): "They died on my watch. I keep hearing it."
              why: grieving: lost someone and still carries it
  hurt        -> hurt             morale 0.94 (fit): "It's nothing. I've had worse. Don't send me back."
              why: injured, on the ward
  hungry      -> hungry           morale 0.94 (fit): "Feed me and I'll hold anything you like."
              why: hungry: the galley is empty
  breathless  -> breathless       morale 0.94 (fit): "I can't -- I can't get a breath. Get me to air."
              why: the deck has no air
  heartened   -> heartened        morale 0.94 (fit): "Put me on it. I've got it in me today."
              why: lifted: something was done for them
  at red      -> afraid           morale 0.94 (fit): "At stations, I just keep my head down and my hands moving."
              why: the ship is at general quarters
```

That is the good one (`steady`) and seven bad — more than the four the brief asks for — each selected
by its own state and each naming the state that chose it. **When several are true at once**, the
precedence picks one and names the rest, so the choice is explainable rather than silent:

```
  several at once: grieving -- grieving: lost someone and still carries it; also afraid, exhausted
              says: "They died on my watch. I keep hearing it."
```

The precedence, written down, highest first:

```
  hurt > breathless > grieving > afraid > tired > hungry > heartened > the morale band
```

## The wiring: raised where the state changes and where the person acts

Not on a timer. The hook sites, and why:

| site | trigger | why there |
|---|---|---|
| `Sicken` / `Cure` (a condition arriving or clearing) | a state change | the person's state changed, and the manner follows. Gated on `CVIS_LOG`: a *recorded* change raises a bark, the tick's automatic conditions (which carry no `CVIS_LOG`) do not each — a red alert would otherwise bury the log. That is the same call the character layer already made (`docs/evidence/the-character-layer.md`, judgement call 4). |
| `DispatchOrder` (the recipient answers) | an order | where the person acts; what they were asked to do and how they bear it are on the record together. |
| `SetAlert` | the alert | the ship's state changed; the watch's manner follows **once, for the officer of the deck**. One line, not 141, for the same reason. |

The same person's manner changing when their state does, from the demonstration:

```
  wiring: a recorded grieving arrives, and the log carries it:
    [crew] "They died on my watch. I keep hearing it." -- grieving, raised by a state change (grieving: lost someone and still carries it)
  wiring: the same person, just given an order:
    [crew] "They died on my watch. I keep hearing it." -- grieving, raised by an order (grieving: lost someone and still carries it)
```

## The surface, and which half is real

Three surfaces, and they are not the same kind of thing:

1. **The log — real, and the floor.** `EmitManner` writes the line to the log in the person's own
   name, always, whatever the voice layer can do. This is ship state and it is the surface the
   demonstration reads headlessly. The player reads it through the existing `ship log` verb and the
   log screen; no new screen was built. The line above (`[crew] "…" -- grieving, raised by a state
   change (…)`) is the simulation's own output.
2. **The voice render queue — real.** With a bank and a render, the cue carries the line, the cast
   reference and the delivery direction, and `EmitManner` queues a `RenderJob` keyed
   `RenderKey(voice, text, delivery)` — the plumbing `docs/evidence/audio-plumbing.md` built:
   ```
   surface: Vorik, voice munro, delivery report: "Give me the hard one. I want to be the one who did it."
   the cue: spoken; 1 render job(s) queued
   ```
   **No clip is produced here.** The renderer (`tools/voice/render.py`) and the played audio are
   phase three and need a host with audio; the repository ships the mechanism, never the output.
3. **The console verb `ship manner [crew]` — the engine half, named.** It prints the selection, the
   reason and the cue, and emits it, exactly as the demonstration does; it is compiled into the
   module (`module/ship/g_ship.cpp`, `console_api.def`) and held by `scripts/api-check.sh`. **It is
   not driven on this host** — that needs a logged-in session — which is the half named and left.

## The absence case: no bank, or no render

The simulation still runs and the cue degrades to text — the line is written and read, and no render
is queued. A manner that needed audio to be perceived would not be built, so this is the floor, not a
fallback:

```
  no bank: text (no bank); the line is still written and read
  no render: text (no render); the simulation ran
```

`TestMannerSurfaceAndAbsence` asserts the three outcomes: with a bank and a render the cue is
`MANNER_EMIT_SPOKEN` and one job is queued with the key `RenderKey(voice, line, delivery)`; with the
bank absent (a host switch, `SetMannerVoiceBank(false)`) it is `MANNER_EMIT_TEXT_NO_BANK` and the
queue is empty; with the render absent (`nullptr`) it is `MANNER_EMIT_TEXT_NO_RENDER`. In all three
the log gained exactly one entry, and the dead (`MANNER_NONE`) emit nothing.

## The falsification

The state before this pass is planted: with the selection unwired, every bad state reads the morale
band alone. `scripts/manner-check.sh` runs both and fails on the planted one:

```
  the selection is NOT wired (planted): the morale band alone
  ...
  tired       -> steady           morale 0.94 (fit): "Give me the hard one. I want to be the one who did it."
  afraid      -> steady           morale 0.94 (fit): "Give me the hard one. I want to be the one who did it."
  grieving    -> steady           morale 0.94 (fit): "Give me the hard one. I want to be the one who did it."
  hurt        -> steady           morale 0.94 (fit): "Give me the hard one. I want to be the one who did it."
```

The same greps that pass on the wired output (`tired -> tired`, `grieving -> grieving`, …) fail on
this one, and the check says so rather than passing vacuously. `TestMannerSelection` also breaks the
selection from inside: with the wiring off the four bad states all answer `MANNER_STEADY`.

## Save and determinism

**The manner is a read, never a store, so it costs no save state and needs no version bump**
(unchanged at **59**). The selection is a function of state the save already carries — the three
morale reads, the bounded conditions, the injury and the ship's alert — so the same state selects the
same manner for the same person, and a save replays it:

```
  same state, same manner twice: yes
  a save replays the manner: yes
```

The only thing an emission writes is the log entry (the surface); measured by
`TestMannerDeterminismAndSave`, packing a ship before and after an emission grows the save by exactly
that entry and nothing else. The cast reference is derived from the record and the seed, exactly as
`CastVoice` already is, never stored; the two host switches (`SetMannerWired`, `SetMannerVoiceBank`)
are host state and are not saved.

## The checks, and what they returned

```
$ scripts/test.sh
  -> crew_core: all checks passed
     ship_core: all checks passed, including TestManner, TestMannerSelection, TestMannerWiring,
       TestMannerSurfaceAndAbsence, TestMannerDeterminismAndSave
     hooks: every public function is registered, and every registered hook exists
     manner: the crew's state is audible and visible, and degrades without audio (G14) -- PASS
     api: every console verb is in the ship's API, and every API verb is answered by the console
     docs: cited paths resolve; owned numbers agree; register rows agree with the tree
     tools OK; python syntax ok; shellcheck ok; 24 patches
     all checks passed

$ scripts/manner-check.sh
  -> PASS  the manner is selected from the state (steady and the bad states, each named, with the
     precedence when several are true); raised where the state changes and where the person acts;
     written to the log and handed to the voice; degrades to text with no bank or no render; the
     same state selects the same manner and a save replays it -- and the planted state before this
     pass makes the check fail.

$ module build (the engine half)
  -> cmake -S /home/robot/upstream/efgame -B <build> -DLWH_MODULE_DIR=.../module ; cmake --build
     [100%] Built target efgame
     libefgame.so exports only GetGameAPI, vmMain and dllEntry; ship::MannerFor and ship::EmitManner
     are in it.

$ scripts/check.sh --source-map .../voy1.map --script-corpus .../gdk/scripts
  -> entity dictionary built; validator negative tests ALL PASS;
     2408 files: 2394 compiled and read back, 14 rejected -- exactly the known, documented set;
     the 2 unresolved script references are exactly the recorded boundary; exit 0
     (this pass touches no script, no map and no entity class)
```

`docs/hook-register.md` carries the ten new hooks and the corrected `MannerLine` row;
`docs/hook-triage.md` moves `MannerLine` from **genuine gap** to **wired** (the count falls 62 → 58,
corrected in place).

## What could not be verified

- **Whether a manner reads as a *person* rather than as a notification.** That is the owner's ear,
  and the ledger already says so; a green check is the mechanism, not that judgement. The
  demonstration prints nine states for one person and is the thing to hear.
- **The console read at a logged-in session** (`ship manner`) and **the played audio.** Both are the
  engine half; this host runs neither, and no clip was rendered.
- **The clips themselves.** No cue or manner clip has been assembled or heard; the mechanism names
  the cast reference and the delivery, and `render.py` produces the audio on the player's own machine.

## Judgement calls, named as calls

1. **The precedence order is ours** (`hurt > breathless > grieving > afraid > tired > hungry >
   heartened > band`). The design names the states and does not order them; this order is small and
   recorded so it can be overruled. The losing states are always named, so the order is visible.
2. **The action hook is `DispatchOrder`, and the alert hook is the officer of the deck.** The brief
   says "where the state changes and where the person acts" without naming the sites; these are the
   three the model already has, and the alert is one line and not 141 by the character layer's own
   no-flood rule. **This is a call.**
3. **The trigger is carried and named, not a second selection input.** `MannerTrigger` records where
   the cue was raised (a state change, an order, the alert, a question) and the line carries it; the
   selection reads state, not the trigger. Making the trigger itself change the manner (a resentful
   line for an order taken badly) is a named seam, **not built** — `DispatchOrder` already answers the
   order in the recipient's voice (`docs/evidence/orders.md`), and duplicating it was declined.
4. **The lines are content, and ours.** The lines for the acute manners are written here and read the
   person's drives as `MannerLine` does; `MannerLine`'s three band lines are unchanged. Whether they
   land is the owner's read.
5. **The two host switches are host state, not save state.** `SetMannerWired` and `SetMannerVoiceBank`
   default on and are never saved; they exist for the falsification and the absence case, exactly as
   `g_crewDoorOpen` does for the doors.
6. **No save version bump.** The manner adds no field; bumping would invalidate older saves for
   nothing. **This is a call**, and the cost is exactly zero — the one thing it changes is the log's
   content, which is text and is not versioned.

**Boundary kept.** No audio, no reference clip and no extracted asset is committed — not to this
file, not to the branch. Nothing was published or uploaded. Nothing is committed to `main`.
