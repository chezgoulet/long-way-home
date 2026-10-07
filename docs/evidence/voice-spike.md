# Evidence — the voice spike: one character, one sentence, one wav

Date: 2026-10-07. Branch `feat/voice-spike`, cut from `testing`. The question the spike asks:

> **Can a line a character never said be produced, on this machine, from the lines they did say —
> recognisably?**

Answer: **yes, the mechanism works** — a novel line rendered from a retail character's own voice, clean
and intelligible, on CPU, in half a minute. **The verdict on whether it sounds like the character is the
owner's**, and is outstanding; nothing here substitutes for his ear.

The mechanism is `tools/voice/analyze.py` and `tools/voice/synthesize.py`. The one wav this spike
produced is local and is **not** in this repository.

## Stage one — what the retail voice assets actually are

The decisive finding came before any model was installed, and it is the one that mattered:

> **The dialogue is already separated per character, and already separated from the music.**

So the analyzer needs neither speaker diarization nor source separation. That removed two of the named
dependencies (`silero-vad`, `demucs`) and most of the risk.

**Location.** The host's retail install is a GOG installation reached through a Steam compatibility
prefix:

```
/home/c/.local/share/Steam/steamapps/compatdata/4009300482/pfx/drive_c/Program Files/GOG Galaxy/Games/Star Trek Elite Force
    BaseEF/pak0.pk3   566 MB
    BaseEF/pak1.pk3     6.5 MB
    BaseEF/pak2.pk3    12.4 MB
    BaseEF/pak3.pk3    88.9 MB
```

**Container and codec.** A `.pk3` is a zip archive. The voice is **MP3**, mono, 44,100 Hz, 47–71 kbps
(mean ~58) — from August 2000, so lossy but not lo-fi. Each file is **one utterance**, already
segmented.

**Organisation.** pak0.pk3 holds 7,547 MP3s in total; the dialogue sits under `sound/voice/<character>/`,
one directory per speaker. Across all paks:

```
voice/:  124 characters, 3,911 lines      # the shipped tree
vox_d/:  113 characters, 3,815 lines      # a second tree of the same lines, a different encode
```

`vox_d/` is the downloadable voice pack (the "1.2 voice pack" of `docs/client-modes.md`); the two trees
carry the same characters and similar line counts, and `analyze.py` prefers the shipped `voice/` tree.
Music is under `sound/music/` and effects/ambience under `sound/ambience/` and `sound/enemies/` — the
dialogue files do not contain them by construction.

**How clean, and how much per character.** Every line is one speaker, one utterance, level-normalised
(mean ≈ −16 to −19 dB, peak ≈ 0 dB). Per-character totals, measured by `analyze.py census` plus
`ffprobe`:

| character | lines | speech |
|---|---|---|
| computer | 375 | — |
| munro (the player) | 324 | 14.7 min (earlier count of 321 lines) |
| alexa | 321 | — |
| **tuvok** | **242** | **20.6 min** |
| chell | 182 | — |

Tuvok's 242 lines average 5.2 s (median 4.1 s, longest 44.6 s). **Twenty minutes of clean,
single-speaker speech is far more than the ~5 s a modern cloner needs.**

**Stage-one verdict: the assets are usable.** Rich, clean, per-character, and already segmented.

## Stage two — the mechanism

**What was installed, and what it cost.** `ffmpeg`/`ffprobe` were already on the host (6.1.1). Into a
throwaway venv under the gitignored `voice-scratch/`:

| installed | disk |
|---|---|
| `chatterbox-tts` 0.1.7 + `torch`/`torchaudio` 2.6.0+cpu + `transformers` 5.2.0 and deps | **5.2 GB** |
| Chatterbox weights (`t3_cfg`, `s3gen`, `ve`, `conds`) in the HF cache | **3.0 GB** |
| **total** | **~8.2 GB** |

Disk before: 21 GB free. Disk after: 14 GB free. It fits, but this host is at 94 % used; a host with
less headroom should stop at stage one as the brief directs.

**Which synthesizer, and why.** **Chatterbox** (Resemble AI, MIT) — it installed cleanly from pip on the
first try, and the brief says to try whichever installs. **OmniVoice** (k2-fsa, Apache-2.0) was not
attempted, because Chatterbox did not need a second. A trap worth recording: `pip install chatterbox-tts`
pulls **torch 2.6.0+cu124**, the CUDA build (~2.5 GB of NVIDIA libraries) even on a GPU-less host; it was
replaced with `torch 2.6.0+cpu` from the PyTorch CPU index to reclaim the space.

**Separation and segmentation were skipped, and said so reason to skip:** the assets are already
per-character and per-line, so `demucs` and `silero-vad` were never installed. The cheapest pipeline that
works is the right one.

**The analyzer** (`tools/voice/analyze.py`): finds `baseEF` case-insensitively, opens the paks, lists a
character's MP3s, extracts and measures them, picks the line nearest the target length, strips its
silence, and writes a 16 kHz mono reference wav. `census` reports the whole per-character inventory
without decoding anything.

**The synthesizer** (`tools/voice/synthesize.py`): loads Chatterbox, clones from the reference, writes one
wav, and prints the numbers. It watermarks every clip (Chatterbox's PerthNet watermarker is loaded on
startup); the output is local and is never committed.

## Stage three — the listening test

**The wav, for the owner to play:**

```
/home/c/big/git/lwh-voice/voice-scratch/out/tuvok_line.wav
```

**What to listen for, in one line:** whether this is *Tuvok's* voice — the flat Vulcan delivery — and
whether the sentence below is clean and free of artefacts, not whether it is "close enough" on a score.

- **Character:** Tuvok (Tim Russ), the best-represented clean single-speaker character (242 lines).
- **Sentence, invented for this spike and never spoken in the game:**
  *"Logic is a beginning, Captain, but it is rarely the whole of the answer."*

**The numbers beside it:**

| number | value |
|---|---|
| reference used | **7.84 s, from 1 line** (`sound/voice/tuvok/cin/40/thegood.mp3`, 9.48 s, silence-trimmed) |
| pool it was drawn from | 242 lines / 20.6 min |
| model load | 8.0 s (warm cache; 93 s on the first, cold run) |
| render time | **20.1 s for 3.26 s of audio, 0.16× realtime**, CPU, 12 cores, no GPU |
| output | **156,558 bytes**, 24,000 Hz mono PCM s16, 3.26 s |
| loudness | mean −14.3 dB, peak 0.0 dB |

**How it actually sounded to me — and the honest limit.** I am an agent and I have **no ear**: I cannot
answer the question the wav exists to answer, and I will not pretend a number is that answer. As the
closest available proxy I transcribed the rendered clip with an on-device ASR model and it read back:

```
Logic is a beginning captain but it is rarely the whole of the answer.
```

So the audio is **intelligible and on-text** — the mechanism produced speech, not noise. Whether it is
*Tuvok* is the owner's judgement, and his answer overrides every number above.

## Licences verified

From each project's own `LICENSE`, via `gh api repos/<owner>/<repo> --jq '.license.spdx_id'`:

| project | licence |
|---|---|
| `resemble-ai/chatterbox` | **MIT** |
| `k2-fsa/OmniVoice` | **Apache-2.0** (the alternative, not used) |
| `k2-fsa/sherpa-onnx` | **Apache-2.0** (only for my own ASR check, not part of the mechanism) |

Not used, so not re-verified here: `demucs` (MIT), `silero-vad` (MIT) — the licence research for the
whole candidate list is in the brief and was not redone. Ruled out by the brief and untouched: XTTS v2,
fish-speech.

## What I could not determine

- **Whether it sounds like Tuvok.** No ear. This is the open question, not an oversight.
- **Whether any individual dialogue line carries a hidden music or effect bed.** I could not hear it.
  The evidence that the lines are dry is structural — the dialogue is a separate tree from
  `sound/music/` and `sound/ambience/` — not aural.
- **How much the 2000-vintage ~58 kbps MP3 encode limits clone fidelity.** Not measured.
- **OmniVoice against Chatterbox.** Only one was tried, deliberately.
- **The chain of title on the underlying performances.** That is the owner's legal reading in
  `docs/staff-meetings.md`, not something this spike can settle.

## Judgement calls, named as calls

1. **Tuvok** as the character — most clean lines of any named character (242) and a distinctive,
   recognisable voice.
2. **The sentence is mine**, invented for the spike; I assert it is not a shipped line, but I did not
   search all 3,911 lines' audio to prove a negative.
3. **Skipped `demucs` and `silero-vad`** — the stage-one finding made them unnecessary.
4. **Chatterbox, not OmniVoice** — whichever installed cleanly was the brief's instruction.
5. **CPU torch, not the CUDA build** — no GPU on this host; the CUDA detour was reclaimed.
6. **A one-line, ~8 s reference** rather than several lines — Chatterbox clones from about five seconds
   and a longer reference risks importing more than the voice.
7. **The shipped `voice/` tree, not `vox_d/`** — the pack is a second encode of the same lines.
8. **ASR as a proxy for "how it sounded"** — offered as a *mechanism* check (is it speech, is it the
   sentence), explicitly not as the acceptance test.
9. **Installed `sherpa-onnx` for my own verification only** — it is not a dependency of the mechanism,
   and the ASR model was borrowed from another project's cache.

## The boundary, kept

No generated audio, no reference clips and no extracted game assets are committed. Everything the spike
produced lives under `voice-scratch/`, which is gitignored. The retail install was read only. Nothing is
committed to `main`.
