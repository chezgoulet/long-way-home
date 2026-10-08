# Evidence — the owner's review of the voice examples

Date: 2026-10-07. **The acceptance for this mechanism is the owner's ear**, and this is what it returned.
The clips were `computer`, `tuvok`, `alexa`, `munro` (pass one) and `janeway_burger`, `janeway_command`
(pass two). Nothing here is a measurement; the measurements are in `voice-examples.md` and
`janeway-cheeseburger.md`.

## The verdicts, in his words

| clip | verdict |
|---|---|
| **computer** | **really good** — but *"it sounds like it might have a weird reverb effect on. Or the sample it learned from had a reverb on it."* The synthesis itself, otherwise: great. |
| **tuvok** | **perfect** |
| **alexa** | **just fine** — *"though clearly, their voices are not as distinct as the main cast"* |
| **munro** | **just fine** — same note |
| **janeway_burger** | **pretty good** — *"it sounds like she slips into a British accent at the end of it (that's an ordah)"* |
| **janeway_command** | **good, no British invasion** — *"though her tone lacks immediacy considering the emergency content she's talking about, it sounds close enough to her real voice"* |

**Five of six land. Two carry an artefact. Both artefacts are diagnosable, and neither is a defect of the
mechanism.**

## Finding one — the computer's reverb is a reference defect, and the reference is known

The owner's own inference — *"or the sample it learned from had a reverb on it"* — is the right one.

**The last pass's computer reference was two lines concatenated**, and that pass named it its weakest result.
A concatenation joins **two different recording conditions**, which is exactly what produces an audible seam
and a room-tone artefact. And canon's ship-computer lines are **PA announcements**, so the source asset
plausibly carries room processing of its own.

**Fix: select one dry line, not two.** The instrument is the reference itself: **one speaker, one utterance,
one condition.** There is a rule here that applies to every character, not just this one — **a concatenated
reference buys duration at the cost of the room**, and duration is not what the model needs.

## Finding two — the British tail is prompt-influence decay, and `cfg_weight` is the lever

Read from the installed library, not recalled:

```
ChatterboxTTS.generate(text, repetition_penalty=1.2, min_p=0.05, top_p=1.0,
                       audio_prompt_path=None, exaggeration=0.5,
                       cfg_weight=0.5, temperature=0.8)
```

- **There is no `language` parameter.** The accent **cannot be pinned by a setting**, which rules out the
  obvious fix and matters: any accent repair has to come from **the reference or the guidance weight.**
- **`cfg_weight` is the lever.** It trades adherence to the prompt against fluency. **Drift at the *end* of an
  utterance is the signature of the reference's influence decaying as the output lengthens**, with the model
  falling back on its own prior — which is British-trained, for this model. **Raise `cfg_weight`.**
- **And a longer reference conditions more**, over the model's own `DEC_COND_LEN` / `ENC_COND_LEN` windows.
  The Janeway reference was a single 7.99 s line; the cheeseburger output was 4.85 s.
- **The design consequence, and it is a real one: if a clone drifts across ~5 seconds, then line length is a
  rendering constraint, not just a writing choice.** The meeting design is already safe here, because every
  line is rendered separately with its own conditioning — but the constraint must be **written down**, because
  a future lane will otherwise render a long speech in one call and get a drift nobody can explain.

## Finding three — the missing immediacy is a missing *control*, not a missing capability

**`exaggeration` defaults to 0.5**, and the source shows it entering the model as `emotion_adv`. **The second
Janeway pass used defaults throughout** — so the emergency line was rendered with **exactly the same emotional
setting as the cheeseburger.** It is not that the model cannot do urgency. **Nobody asked it to.**

**And this is a feature the plumbing needs anyway.** The meeting brief already knows what each line *is* — an
order, a condolence, a confession, a report — because it enumerates what is being decided and what each option
costs. **So delivery direction belongs in the brief, per line, and the synthesizer takes it.** That is a
requirement this review produced, and it lands exactly on the design's existing shape rather than beside it.

## Finding four — the prosody proxy predicted the owner's ear, so it is a usable instrument

The second pass measured the urgency clip's pitch range as **narrower** and its rate **slower** than the
cheeseburger's, and **explicitly refused** to call it urgent, saying the proxy could not hear urgency.

**The owner then heard it as lacking immediacy. The measurement and the ear agreed.**

**So the F0/rate proxy is validated as a screening tool** — it can catch a flat render before the owner is
asked, which is worth having because it means his ear is spent on verdicts rather than on triage. **What it
cannot do is decide whether a clip is the character**, and the pass was right to say so.

## Finding five — "not as distinct as the main cast" is a property of the source, and it argues for the design already chosen

**This is not a quantity problem.** By the census, Alexa has 321 lines and 14.9 minutes and Munro 324 and 14.5 —
Comma **more than Tuvok's 242 lines.** So the difference is not how much source exists.

**It is that a minor character's performance carries less vocal characterisation.** A clone reproduces what is
there, and there is less there. That is correct behaviour rather than a shortfall.

**And it is the argument for the design already chosen.** The canon few are cloned from what exists; **our own
crew get voices we choose**, so their distinctness is a casting decision rather than a limit of the mechanism.
The owner's note, read that way, is a reason the split is right.

## The knobs, by name, for the next pass

| parameter | default | what it does | what to use it for |
|---|---|---|---|
| `exaggeration` | **0.5** | enters the model as `emotion_adv` — the emotion control | **delivery direction.** Up for urgency, down for the flat and procedural |
| `cfg_weight` | **0.5** | prompt adherence against fluency | **drift and accent.** Up when the voice wanders off the reference |
| `temperature` | 0.8 | sampling variety | lower for consistency across a character's lines |
| `repetition_penalty` | 1.2 | discourages loops | leave unless a render loops |
| `min_p` / `top_p` | 0.05 / 1.0 | sampling floor | leave |
| *(no `language`)* | — | **accent cannot be pinned by a setting** | fix it in the reference or `cfg_weight` |

## What this review changes

1. **Reference selection is a rule, not a convenience: one line, one speaker, one utterance condition, as dry
   as the source allows.** A concatenation buys duration at the cost of the room.
2. **Line length is a rendering constraint.** Render per line, condition per line; never one long call.
3. **The brief carries delivery direction per line**, and the synthesizer takes it as `exaggeration`.
4. **The prosody proxy screens; the ear decides.**
