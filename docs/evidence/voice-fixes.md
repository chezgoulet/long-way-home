# Evidence — the fix pass: one change at a time

Date: 2026-10-07. Branch `feat/the-voice-fixes`, cut from `testing`. This pass answers the owner's review
(`docs/evidence/voice-review.md`) with **three renders, each changing exactly ONE thing**, so that a better
clip can be attributed to a named cause. The question this pass asks:

> **For each of the two artefacts the owner heard — the computer's reverb and Janeway's British tail — does
> the diagnosed lever move the clip, and does raising Janeway's emotion control give the emergency line its
> missing immediacy?**

The mechanism is `tools/voice/analyze.py` and `tools/voice/synthesize.py`. `synthesize.py` **changed** in this
pass (Task A, below); the model, the pipeline and the reference method did not. The three MP3s are local and
are **not** in this repository. They are for the owner's ear, and nothing here substitutes for it.

## Task A — the three knobs, exposed, defaults preserved

`synthesize.py` now takes the model's own delivery controls alongside `--reference`, `--text`, `--out`:

| flag | default | passed to `generate` as |
|---|---|---|
| `--exaggeration` | **0.5** | `exaggeration` (enters the model as `emotion_adv`) |
| `--cfg-weight` | **0.5** | `cfg_weight` |
| `--temperature` | **0.8** | `temperature` |

The defaults are the model's own defaults, read from `inspect.signature(ChatterboxTTS.generate)` in the
installed library and re-checked when the flag help was printed:

```
generate(self, text, repetition_penalty=1.2, min_p=0.05, top_p=1.0, audio_prompt_path=None,
         exaggeration=0.5, cfg_weight=0.5, temperature=0.8)
```

So **every existing invocation that names none of the three behaves exactly as it did before** — it passes
0.5 / 0.5 / 0.8, the same values it passed implicitly. The script prints the three values it used on every
run (`knobs : exaggeration=... cfg_weight=... temperature=...`), so an invocation is self-documenting.
Nothing else in the script changed: same model, same load path, same `torchaudio.save`, same watermark note.

## Task B — the computer, from one dry line

**The artefact and its cause.** The owner heard reverb and inferred the reference carried it
(`docs/evidence/voice-review.md`, finding one). He was right: the previous computer reference was built by
**concatenating two lines** (`malfunctioning.mp3` + `ejectioninitiated.mp3`, 10.85 s), which joins two
recording conditions.

**The reference rule, applied.** `analyze.py reference` was run with **`--lines 1`**, so the reference is one
speaker, one utterance, one condition:

```
python3 tools/voice/analyze.py reference --game "$GAME" --character computer --lines 1 \
    --out voice-scratch/refs/out/computer_dry_ref.wav
# -> chose: sound/voice/computer/cin/06/malfunctioning.mp3  5.69 s
# -> reference: voice-scratch/refs/out/computer_dry_ref.wav  5.65 s, 181,084 B
```

The chosen line is one of the two the old reference concatenated, so the change from the previous pass is
exactly the removal of the join.

**The encode, measured.** The brief asked to also try the other encode (`vox_d/`, the downloadable pack)
and, if a cheap measurement could choose, to name it. Two cheap measurements were run on the **raw decoded
lines** (no silence-strip, so nothing is trimmed) — the same line in both encodes, 16 kHz mono:

| line | encode | decay tail (time to fall 30 dB below peak) | tail energy in the 200 ms after speech end, rel. peak | spectral flatness |
|---|---|---|---|---|
| `cin/06/malfunctioning.mp3` | `voice/` | **10 ms** | **−31.4 dB** | 0.0344 |
| `cin/06/malfunctioning.mp3` | `vox_d/` | 20 ms | −25.4 dB | 0.0134 |
| `cin/06/ejectioninitiated.mp3` | `voice/` | **20 ms** | **−31.1 dB** | 0.0224 |
| `cin/06/ejectioninitiated.mp3` | `vox_d/` | 125 ms | −27.0 dB | 0.0162 |

Cross-checked on the RMS envelope: in the gaps between words the `voice/` encode falls to a floor of about
**−45 dB** while `vox_d/` holds a floor of about **−35 dB** — 10 dB more room/processing tone through the
whole line, not only at the end.

**What was measured, and the choice.** Three measures, two of them agreeing: the `voice/` encode has a
**shorter decay to −30 dB, less energy after speech and a lower room floor** — it is the **drier** encode.
(Spectral flatness points the same way but is the ambiguous one: a lower value can mean a smoother, room-fed
signal *or* a cleaner encode, so it is reported, not relied on.) **The reference therefore stays in the
`voice/` tree** — the analyzer's own default at `--lines 1` — and the encode question is answered rather than
assumed. `vox_d/` was measured but not used.

**The render.** The same text as before, reference the only change:

```
Warning: the port nacelle is drawing more power than the grid can safely provide.
```

No knob was moved: `exaggeration=0.5 cfg_weight=0.5 temperature=0.8`, so the comparison against the previous
computer clip is the reference and nothing else.

## Task C — Janeway's tail, and Janeway's urgency

Same reference (`voice-scratch/refs/out/janeway.wav`, the single 7.99 s line from the last pass), same text
as last pass, one knob each.

- **`janeway_burger`, `--cfg-weight 0.8`.** The British tail is the reference's influence decaying as the
  utterance lengthens, so the lever is prompt adherence (`docs/evidence/voice-review.md`, finding two).
  **Raised 0.5 → 0.8**: a decisive step toward the reference — large enough that the effect is attributable
  rather than marginal, and still inside the model's usual band (a very high weight trades naturalness for
  adherence). Text unchanged: *"I'd like a cheeseburger, with everything on it, and coffee — black. And
  that's an order."*
- **`janeway_command`, `--exaggeration 0.8`.** The last pass rendered it at the default 0.5 and it carried
  the cheeseburger's emotional temperature (finding three). **Raised 0.5 → 0.8**: the emotion control
  (`emotion_adv`) moved up toward urgency, a decisive step without pushing into caricature. Text unchanged:
  *"All hands, brace for impact! Seal every bulkhead and report damage the moment it clears!"*
- **`temperature` left at 0.8** in both, so exactly one knob moves per clip.

## The three MP3s, for the owner to play

All under `/home/c/big/git/lwh-voice/voice-scratch/out/` (gitignored; never committed, never uploaded):

| file (absolute path) | knob changed from default | mp3 duration | mp3 size | bitrate |
|---|---|---|---|---|
| `/home/c/big/git/lwh-voice/voice-scratch/out/computer_dry.mp3` | **none** — new single-line `voice/` reference | 4.656 s | 74,925 B | 128.7 kbps |
| `/home/c/big/git/lwh-voice/voice-scratch/out/janeway_burger_cfg.mp3` | `--cfg-weight 0.8` | 4.152 s | 66,861 B | 128.8 kbps |
| `/home/c/big/git/lwh-voice/voice-scratch/out/janeway_command_exag.mp3` | `--exaggeration 0.8` | 4.368 s | 70,317 B | 128.8 kbps |

All encoded 128 kbps (`libmp3lame -b:a 128k`). The wavs behind them are 24,000 Hz mono PCM s16.

## The numbers beside each

Hardware as before: CPU only, 12 cores, no GPU; `torch 2.6.0+cpu`. Model load 7.9–9.9 s warm; peak RSS
~6.87 GB; venv and model cache kept (sizes at the end).

| clip | render (CPU) | wav duration | wav size | wav mean / peak |
|---|---|---|---|---|
| computer_dry | 20.7 s, 0.22× realtime | 4.60 s | 220,878 B | −15.4 / −0.6 dB |
| janeway_burger_cfg | 21.0 s, 0.20× | 4.10 s | 196,878 B | −19.1 / −0.0 dB |
| janeway_command_exag | 22.2 s, 0.19× | 4.30 s | 206,478 B | −17.6 / −0.2 dB |

For comparison, the previous renders were: burger 4.78 s / 229,518 B, command 6.10 s / 292,878 B, computer
4.96 s / 238,158 B.

## The F0/rate proxy, new against previous

The proxy from the last pass (`voice-scratch/work/fix/prosody_fix.py`; an autocorrelation F0 track, 60–400 Hz,
plus speaking rate). **It screens; it does not decide.**

| clip | duration | rate | F0 mean | F0 p10–p90 | F0 sd |
|---|---|---|---|---|---|
| reference (janeway) | 7.99 s | — | 143.8 Hz | 103.2–183.9 | 39.7 |
| burger **previous** (cfg 0.5) | 4.78 s | 3.14 w/s | 139.9 Hz | 97.4–169.0 | 43.3 |
| burger **new** (cfg 0.8) | 4.10 s | 3.66 w/s | 136.3 Hz | 95.5–165.5 | 46.6 |
| command **previous** (exag 0.5) | 6.10 s | 2.46 w/s | 144.5 Hz | 111.6–165.5 | 37.3 |
| command **new** (exag 0.8) | 4.30 s | 3.49 w/s | 150.5 Hz | 102.6–187.5 | 52.2 |
| computer reference (dry, 1 line) | 5.65 s | — | 178.7 Hz | 132.2–215.9 | 39.7 |
| computer **new** (dry ref) | 4.60 s | 3.04 w/s | 164.6 Hz | 124.5–215.8 | 36.8 |

Reading: **raising `cfg_weight` shortened the burger** (4.78 → 4.10 s; the model spent less time past the
end of the text) and left its pitch band essentially where it was. **Raising `exaggeration` widened the
command's pitch range markedly** (p10–p90 span 53.9 → 84.9 Hz, sd 37.3 → 52.2 Hz) and **sped it up**
(2.46 → 3.49 w/s) — the opposite of the last pass, where the command was *narrower* and *slower* than the
burger and the proxy flagged it. On this number the emergency line is now the more animated of the two,
which is what the owner asked the knob to do. The proxy still cannot say whether any of it is *Janeway*.

**Word-count note, stated rather than smoothed:** the last pass's `command` rate (2.79 w/s) used a count of
17; the text has **15** words, and this table uses 15 for both the previous and the new render, so the rates
are comparable to each other (the previous render's other columns reproduce that pass's numbers exactly).
The burger count (15) is unchanged.

## The mechanism check (ASR), and how each one actually sounded to me

**The limit, plainly: I am an agent and I have no ear.** I cannot answer the question the clips exist to
answer, and I will not present a score as that answer. As the closest available proxy I transcribed the three
clips with the borrowed on-device model (Whisper-tiny.en via `sherpa-onnx`), both int8 and float — a
*mechanism* check (is it speech? is it on-text?), explicitly **not** an acceptance test:

| clip | float | int8 |
|---|---|---|
| computer_dry | *"Warning. The portness cell is drawing more power than the grid can safely provide."* | same |
| janeway_burger_cfg | *"I'd like a cheeseburger with everything on it and coffee, black, and that's an order."* | same |
| janeway_command_exag | *"All hands brace for impact, seal every bulkhead and report damage the moment it clears."* | *"All hands, brace for impact. Seal every bulkhead and report damage the moment it clears."* |

- **computer_dry** — the render is speech and on-text; "nacelle" is still garbled ("portness cell"), the same
  word both previous computer clips garbled, so the synthetic computer voice defeats this tiny ASR on that
  word. **But this pass's clip reads back from *both* models, where last pass's computer clip returned
  *nothing* from int8** — the dry reference did not make the word worse and may have made the clip more
  robust to the check. Whether the reverb is gone is the owner's ear, not this.
- **janeway_burger_cfg** — clean and verbatim on both models; no crutch words, no garble, out-of-domain words
  ("cheeseburger", "coffee") intact. I cannot hear whether the British tail is gone.
- **janeway_command_exag** — clean and verbatim on both models. Whether the added pitch movement and speed
  read as *urgency* rather than *rushing* is a judgement I cannot make.

**I cannot tell you any of the three is good or poor.** For the command, the proxy moved in the direction
the owner asked for; for the burger, the render got shorter and I cannot say the drift went with it; for the
computer, the check passes and the reference is now one line, one condition. **The owner's ear is the
acceptance.**

## What I could not determine

- **Whether the reverb is gone** — no ear. The cause (a concatenated reference) was removed and the drier
  encode chosen; whether that is enough is the open question.
- **Whether `cfg_weight 0.8` removed the British tail** without costing naturalness. The proxy shows a
  shorter render, not a different accent.
- **Whether `exaggeration 0.8` reads as urgency** rather than as mere speed. Range and rate moved the right
  way; the proxy cannot hear the difference.
- **The 2000-vintage ~58 kbps source encode's ceiling** on clone fidelity — unchanged from previous passes.
- **The chain of title on the underlying performance** — the owner's legal reading in `docs/staff-meetings.md`.

## Judgement calls, named as calls

1. **`--exaggeration`, `--cfg-weight`, `--temperature` exposed with the model's own defaults**, so no
   existing behaviour shifts. The script now prints the values it used.
2. **The computer reference is one line, `cin/06/malfunctioning.mp3`, from the `voice/` tree**, because the
   encode measurement said `voice/` is drier — chosen by measurement, not assumed. `analyze.py` was not
   modified; `vox_d/` was compared with a scratch harness (not committed) that reuses `analyze.py`'s
   functions, because the CLI cannot select the pack tree.
3. **`cfg_weight 0.8` and `exaggeration 0.8`** — decisive, attributable one-knob steps inside the model's
   usual band, named with the reasoning above.
4. **Rendered once each; not re-rolled.** Re-rolling to chase the proxy would be fitting to the check rather
   than to the owner's ear.
5. **ASR, both models, as "how it sounded"** — a mechanism check, offered as such, explicitly not the
   acceptance test.
6. **The proxy's word count corrected visibly** (15, not the previous pass's 17) with the difference stated,
   rather than quoting a rate that does not reproduce.

## The boundary, kept

No generated audio, no reference clips and no extracted game assets are committed — not to the branch, not to
this file. Everything the pass produced lives under `voice-scratch/`, which is gitignored. The retail install
was read only. Nothing was published, shared or uploaded. Nothing is committed to `main`. The virtualenv
(`voice-scratch/venv`, **1.7 GB**) and the model cache (`voice-scratch/hf-cache`, **3.0 GB**) were **kept**,
per the brief; scratch as a whole is ~4.8 GB on `/home/c/big` (383 GB free). The shared `~/.cache/uv` was not
touched.
