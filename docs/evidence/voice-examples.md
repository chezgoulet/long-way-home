# Evidence — the voice examples: four characters, one line each

Date: 2026-10-07. Branch `feat/voice-examples`, cut from `testing`. The question this pass asks:

> **Across four distinct voices, does each generated line sound like the character?**

The mechanism is `tools/voice/analyze.py` and `tools/voice/synthesize.py`, and it is **unchanged** by this
pass. The four MP3s are local and are **not** in this repository. Whether each clip is *the character* is the
owner's judgement; nothing here substitutes for his ear.

## What was produced

Four characters, chosen because the census shows they lead the shipped `voice/` tree in line count and because
their registers are distinct — a synthetic ship's computer, a female officer, a deep male Vulcan officer, and the
player's own voice. `analyze.py census` confirms the brief's suggestion:

| character | register | lines in `voice/` | speech |
|---|---|---|---|
| computer | synthetic feminine ship's computer | 375 | 28.2 min |
| alexa | female officer | 321 | 14.9 min |
| tuvok | deep male Vulcan officer | 242 | 20.6 min |
| munro | the player's own voice | 324 | 14.5 min |

The lines — invented for this pass, one sentence each, in the fiction of *Long Way Home*, not quotes from the
show (a negative against all 3,911 shipped lines was **not** proven; see below):

- **computer** — *"Warning: the port nacelle is drawing more power than the grid can safely provide."*
- **alexa** — *"I have the structural scan, Commander, and the damage is worse than the board shows."*
- **tuvok** — *"The breach is contained, Captain, but it will not seal itself."*
- **munro** — *"I am reading weapons fire on the far side of the debris field."*

## The four MP3s, for the owner to play

All under `/home/c/big/git/lwh-voice/voice-scratch/out/` (gitignored; never committed, never uploaded):

```
/home/c/big/git/lwh-voice/voice-scratch/out/computer.mp3
/home/c/big/git/lwh-voice/voice-scratch/out/alexa.mp3
/home/c/big/git/lwh-voice/voice-scratch/out/tuvok.mp3
/home/c/big/git/lwh-voice/voice-scratch/out/munro.mp3
```

## The numbers beside each

Hardware: CPU only, 12 cores, no GPU; `torch 2.6.0+cpu`. Model load **~7.9–8.0 s** each (weights cached);
peak RSS ~6.87 GB.

| character | reference | drawn from | pool | render time (CPU) | wav duration | wav size | mp3 128 kbps | mp3 duration |
|---|---|---|---|---|---|---|---|---|
| computer | **10.85 s** | 2 lines | 375 lines / 28.2 min | 36.8 s (0.13× realtime) | 4.96 s | 238,158 B | **80,685 B** | 5.016 s |
| alexa | **8.69 s** | 1 line | 321 lines / 14.9 min | 28.1 s (0.19×) | 5.48 s | 263,118 B | **89,133 B** | 5.544 s |
| tuvok | **7.84 s** | 1 line | 242 lines / 20.6 min | 17.8 s (0.18×) | 3.26 s | 156,558 B | **53,421 B** | 3.312 s |
| munro | **8.68 s** | 1 line | 324 lines / 14.5 min | 21.7 s (0.18×) | 3.88 s | 186,318 B | **63,405 B** | 3.936 s |

The wavs are 24,000 Hz mono PCM s16. Loudness of the rendered wavs (mean / peak): computer −14.1 / 0.0 dB,
alexa −15.2 / 0.0, tuvok −15.6 / −0.1, munro −14.7 / 0.0 — level with the spike's Tuvok clip.

The references were built by the analyzer at its defaults (`--lines 2 --target-sec 10`); computer used the
allowed two lines (`malfunctioning.mp3` + `ejectioninitiated.mp3`), the other three one line each. The
**computer reference is the only one concatenated from two utterances** — a possible cause of its weaker
result, unmeasured (see below).

## How each one actually sounded to me

**Honest limit first: I am an agent and I have no ear.** I cannot answer the question the clips exist to
answer, and I will not present a number as that answer. As the closest available proxy I transcribed each with a
**borrowed** on-device ASR model (Whisper-tiny.en, via `sherpa-onnx`) — a *mechanism* check (is it speech? is it
on-text?), explicitly **not** an acceptance test.

- **computer** — the float model read back *"Warning. The portanosell is drawing more power than the grid can
  safely provide."*, and the int8 model returned **nothing at all** for this clip. "nacelle" is garbled in both.
  On this evidence the **computer clip is the weakest of the four.**
- **alexa** — read back cleanly and on-text: *"I have the structural scan commander and the damage is worse than
  the board shows."*
- **tuvok** — read back on-text: *"the breeches contained captain, but it will not seal itself."* ("breach is"
  runs together, as the spike's model did).
- **munro** — read back cleanly and on-text: *"I am reading weapons fire on the far side of the debris field."*

So all four are **speech, not noise**, and on-text; the computer one is the doubtful one. Whether each is *the
character* is the owner's judgement, and his answer overrides every number here.

## What was installed, and what it cost

Baseline before install: the volume was **228 GB, 203 GB used, ~13 GB free, 95 % used.** The system `python3`
has neither `pip` nor `python3-venv`; the throwaway environment was built with **`uv`** (the project's
preference) under the gitignored `voice-scratch/`.

| installed | disk |
|---|---|
| `chatterbox-tts` 0.1.7 + `torch`/`torchaudio` 2.6.0+**cpu** + `transformers` 5.2.0 and deps | venv **1.7 GB** |
| `sherpa-onnx` 1.13.8 (my own ASR check only; **not** a mechanism dependency) | in the venv |
| Chatterbox weights (`t3_cfg`, `s3gen`, `ve`, `conds`) in the HF cache | **3.0 GB** |
| `uv` download cache | **74 MB** |
| **total added** | **~4.7 GB** |

The `HF_HOME` was pointed at `voice-scratch/hf-cache`, not the shared `~/.cache/huggingface`, so the 3 GB of
weights was mine and trivially reclaimable.

**Reclaimed.** The venv (1.7 GB), the model cache (3.0 GB) and the `uv` cache (74 MB) were removed; the scratch
directory fell from **4.7 GB to 30 MB**. The four MP3s and the four reference wavs were kept
(`voice-scratch/refs/out/{computer,alexa,tuvok,munro}.wav`). `tools/voice/` was not modified.

**Caveat, because this host is shared.** `df` free space did **not** visibly rise after reclaiming (the exact
reading moved 13,533 MB → 13,527 MB), because concurrent work elsewhere on the host consumed the freed space at
the same time. The reliable measure of the reclaim is the scratch directory itself: **4.7 GB → 30 MB.**

## Traps hit, and how I got past them

1. **The inherited trap.** `pip install chatterbox-tts` pulls **torch 2.6.0+cu124**, the CUDA build (~2.5 GB of
   NVIDIA libraries) even on a GPU-less host. Got past it by pinning `torch==2.6.0+cpu` / `torchaudio==2.6.0+cpu`
   from the PyTorch CPU index *in the same resolver command*, so the CUDA download never happened.
2. **A new trap in this pass.** With the resolver's current `setuptools` (84.0.0), Chatterbox failed at model
   construction with `TypeError: 'NoneType' object is not callable`. The actual fault is
   `perth.PerthImplicitWatermarker` being `None`: `perth` imports `pkg_resources`, which setuptools 84 no longer
   ships, and `perth/__init__.py` swallows the resulting `ImportError` into a `None`. Got past it by pinning
   **`setuptools==80.9.0`** in the throwaway venv, after which the watermarker imports and all renders succeed.
   **This is a fix to the dependency environment, not to `tools/voice/`** — but it is worth a version pin or a
   note in the mechanism's README, because a fresh environment with current setuptools will hit it exactly as I
   did. Per the brief, it is **reported here, not fixed in this pass.**
3. **The int8 ASR model returned empty text** for the synthetic computer voice; the float model recovered it.
   Recorded above; it is a limitation of the check, not evidence about the clip.

## What I could not determine

- **Whether any clip sounds like the character.** No ear. This is the open question, not an oversight.
- **Whether the two-line computer reference diluted its voice** — the only reference built from more than one
  utterance, and the weakest result; not measured.
- **Whether the invented lines are absent from all 3,911 shipped lines.** Not searched as audio.
- **The 2000-vintage ~58 kbps source encode's limit on clone fidelity.** Not measured.
- **The chain of title on the underlying performances** — the owner's legal reading in `docs/staff-meetings.md`,
  not something this pass can settle.

## Judgement calls, named as calls

1. **Characters: computer, alexa, tuvok, munro** — the brief's suggestion, confirmed by the census (they lead the
   shipped tree) and chosen for four distinct registers.
2. **One invented, in-fiction, one-sentence line each** — not a shipped line, not a show quote.
3. **`HF_HOME` redirected into `voice-scratch/hf-cache`** so the model cache was mine, not the shared cache, and
   was reclaimable in one step.
4. **`uv`, not `pip`** — the system python ships neither `pip` nor `python3-venv`; the mechanism scripts are
   unchanged and were run with the throwaway venv's interpreter.
5. **ASR as a proxy for "how it sounded"** — a mechanism check, offered as such, explicitly not the acceptance
   test.
6. **Rendered once at defaults; the poor "nacelle" word was not re-rolled** — the clips are for the owner's ear,
   and re-rolling to chase the check would be fitting to the proxy rather than to the verdict.

## The boundary, kept

No generated audio, no reference clips and no extracted game assets are committed — not to the branch, not to
this file. Everything the pass produced lives under `voice-scratch/`, which is gitignored. The retail install
was read only. Nothing is committed to `main`.
