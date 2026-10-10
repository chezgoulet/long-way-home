# Evidence — the scream audition: screamed words, cloned from the screamed samples

**This is an experiment, not a feature.** `feat/scream-audition`, cut from `testing`, 2026-10-09, on
`sasquatch` (CPU-only, no GPU). The deliverable is clips the owner can hear and this note. **No code in
`module/`, no new script in `scripts/`, no change to `tools/voice/*`, no casting-map change, no PR.**
Everything audio-shaped lives under the gitignored `voice-scratch/` and is **not** in the repository.

The question: **does cloning from a character's own *screamed* samples — `pain25` / `pain100` / `death3` —
make our synthesizer produce screamed words?**

**Short answer, in the only terms an earless agent can offer:** the screamed reference *does* import its
register — every cell given a scream reference came out markedly **higher-pitched and louder** than its
dialogue control, and the ASR proxy punctuated it as a shout. Cells 4 and 5 recovered the exact line
(*"No!"* / *"NO!"*). But cell 3, with `exaggeration` pushed to 1.3, lost intelligibility and reads to the
proxy as a strained burst. **Whether any of these is a scream rather than a strained shout is the owner's
ear, and is open.** A cell that turned to noise would have been a finding; none did outright, but cell 3
is the weak one.

## The environment, reused from the pass that just landed

`docs/evidence/voice-and-casting.md` (the casting pass) gives the throwaway-environment procedure; this
pass reused it rather than rediscovering it, with the brief's one change — the HuggingFace cache lives at
the **default home**, not in the scratch.

```sh
# throwaway venv, on the gitignored scratch (this host's /home/c/big has 381 G free)
uv venv --python 3.12 voice-scratch/venv
# the CPU build must be pinned by local version, or the resolver takes the CUDA wheel from PyPI
uv pip install --python voice-scratch/venv/bin/python \
    --index-url https://download.pytorch.org/whl/cpu 'torch==2.6.0+cpu' 'torchaudio==2.6.0+cpu'
uv pip install --python voice-scratch/venv/bin/python chatterbox-tts==0.1.7
# the perth trap: resemble-perth imports pkg_resources, so setuptools is pinned below 81
uv pip install --python voice-scratch/venv/bin/python 'setuptools==80.9.0'
# my own screening instrument only (the spike's method, not a mechanism dependency)
uv pip install --python voice-scratch/venv/bin/python 'sherpa-onnx==1.13.8'
```

Verified: `torch 2.6.0+cpu cuda False setuptools 80.9.0`, `import perth` clean (only the
`pkg_resources` deprecation warning), `chatterbox-tts==0.1.7`. Venv 1.7 GB. Unpinning the CPU build first
resolved `torch 2.6.0+cu124` and pulled the NVIDIA libraries; that was replaced with the CPU build and the
CUDA cache entries were cleaned (see the disk note).

**HuggingFace cache:** `/home/c/.cache/huggingface`, **3.0 GB** (`models--ResembleAI--chatterbox`). It was
seeded by copying the weights already on this host from the earlier spike's `lwh-voice` scratch, so this
pass **did not re-download 3.0 GB**, and a second round of the audition will find them in the default home.

**Disk, reported as the brief requires.** `/` (where `/home/c/.cache` lives) is the tight filesystem: 9.0 G
free at the start, 6.0 G after seeding the cache, and it **dipped to 3.9 G** — below the brief's 4 G floor —
because the CUDA-resolver detour had put ~2.3 G of unused wheels into `/home/c/.cache/uv`. That was
reclaimed with `uv cache clean` (the cache is regenerable); `/` now has **14 G free**. The venv and every
clip are on `/home/c/big` (381 G free). Nothing was written into the retail install.

## The references — how the screamed samples were conditioned

`analyze.py reference` picks *dialogue* lines by duration and cannot select a named file, so the named
`misc/` members were pulled from the pak and run through **the analyzer's own filter chain** by a helper,
`voice-scratch/extract-misc.py` (extract + `highpass=f=60`, the same silence trim, `-ar 16000 -ac 1
pcm_s16le`), so the comparison against the controls is fair. The helper is scratch, not repository.

```sh
GAME="/home/c/.local/share/Steam/steamapps/compatdata/4009300482/pfx/drive_c/Program Files/GOG Galaxy/Games/Star Trek Elite Force"

# the two controls — the references we ship today
python3 tools/voice/analyze.py reference --game "$GAME" --character jaworski \
    --out voice-scratch/refs-audition/jaworski-dialogue.wav
python3 tools/voice/analyze.py reference --game "$GAME" --character janeway \
    --out voice-scratch/refs-audition/janeway-dialogue.wav

# the named screamed samples
python3 voice-scratch/extract-misc.py --game "$GAME" \
    --member sound/voice/jaworski/misc/pain100.mp3 --out voice-scratch/refs-audition/jaworski-pain100.wav
python3 voice-scratch/extract-misc.py --game "$GAME" \
    --member sound/voice/jaworski/misc/death3.mp3  --out voice-scratch/refs-audition/jaworski-death3.wav
python3 voice-scratch/extract-misc.py --game "$GAME" \
    --member sound/voice/jaworski/misc/pain25.mp3  --out voice-scratch/refs-audition/jaworski-pain25.wav
python3 voice-scratch/extract-misc.py --game "$GAME" \
    --member sound/voice/janeway/misc/pain100.mp3  --out voice-scratch/refs-audition/janeway-pain100.wav
```

What the analyzer's picks and the named samples are, with their own pitch (the proxy's F0, median and
range, via `librosa.pyin`):

| reference | source | duration | F0 med | F0 range |
|---|---|---|---|---|
| `jaworski-dialogue` | `analyze.py reference`, 2 lines (`voy9/odelldead.mp3` 5.28 s + `voy9/odellok.mp3` 5.12 s) | 9.17 s | 101 Hz | 60–157 Hz |
| `janeway-dialogue` | `analyze.py reference`, 1 line (`voy1/backtogether.mp3` 8.67 s) | 7.99 s | 148 Hz | 64–208 Hz |
| `jaworski-pain100` | `sound/voice/jaworski/misc/pain100.mp3` | 0.68 s | 836 Hz | 817–836 Hz |
| `jaworski-death3` | `sound/voice/jaworski/misc/death3.mp3` | 1.23 s | 476 Hz | 425–491 Hz |
| `jaworski-pain25` | `sound/voice/jaworski/misc/pain25.mp3` | 0.29 s | 408 Hz | 214–411 Hz |
| `janeway-pain100` | `sound/voice/janeway/misc/pain100.mp3` | 0.71 s | 353 Hz | 233–411 Hz |

**The screamed samples are 2.4× to 8× the dialogue register in pitch, and they are short — 0.29 s to
1.23 s, all well under the ~5 s Chatterbox conditions on.** They are single dry utterances, so the
reference rule (*one line, one speaker, one condition, as dry as the source allows*) is satisfied; there is
no concatenation and therefore no seam to import. The condition they carry is the scream's register itself.

## The matrix, as rendered

Eight cells, **one `synthesize.py` invocation per cell** (line length is a rendering constraint). Only
`--exaggeration` was varied; `cfg_weight` and `temperature` were left at the model defaults (0.5 / 0.8)
throughout, so each cell differs from its control in exactly one thing.

| # | voice | reference | line | exaggeration | output (under `voice-scratch/out-audition/`) |
|---|---|---|---|---|---|
| 1 | jaworski | dialogue (control) | `No!` | 0.5 | `cell1-jaworski-dialogue-no-exag050.wav` |
| 2 | jaworski | pain100 | `No!` | 0.5 | `cell2-jaworski-pain100-no-exag050.wav` |
| 3 | jaworski | pain100 | `No!` | 1.3 | `cell3-jaworski-pain100-no-exag130.wav` |
| 4 | jaworski | death3 | `No!` | 0.5 | `cell4-jaworski-death3-no-exag050.wav` |
| 5 | jaworski | pain25 | `No!` | 0.5 | `cell5-jaworski-pain25-no-exag050.wav` |
| 6 | janeway | dialogue (control) | `Get down — that console is going!` | 0.5 | `cell6-janeway-dialogue-getdown-exag050.wav` |
| 7 | janeway | pain100 | `Get down — that console is going!` | 0.5 | `cell7-janeway-pain100-getdown-exag050.wav` |
| 8 | janeway | pain100 | `Get down — that console is going!` | 1.3 | `cell8-janeway-pain100-getdown-exag130.wav` |

The exact per-cell command (driven by `voice-scratch/render-audition.sh`, whose whole output is in
`voice-scratch/work/audition-render.log`), for example:

```sh
export HF_HOME=/home/c/.cache/huggingface
voice-scratch/venv/bin/python tools/voice/synthesize.py \
    --reference voice-scratch/refs-audition/jaworski-pain100.wav \
    --text "No!" --out voice-scratch/out-audition/cell2-jaworski-pain100-no-exag050.wav --exaggeration 0.5
```

An `.mp3` sits beside every `.wav`, for the owner's ear. **The clips, to play** (absolute paths):

```
/home/c/big/git/long-way-home/voice-scratch/out-audition/cell1-jaworski-dialogue-no-exag050.wav
/home/c/big/git/long-way-home/voice-scratch/out-audition/cell2-jaworski-pain100-no-exag050.wav
/home/c/big/git/long-way-home/voice-scratch/out-audition/cell3-jaworski-pain100-no-exag130.wav
/home/c/big/git/long-way-home/voice-scratch/out-audition/cell4-jaworski-death3-no-exag050.wav
/home/c/big/git/long-way-home/voice-scratch/out-audition/cell5-jaworski-pain25-no-exag050.wav
/home/c/big/git/long-way-home/voice-scratch/out-audition/cell6-janeway-dialogue-getdown-exag050.wav
/home/c/big/git/long-way-home/voice-scratch/out-audition/cell7-janeway-pain100-getdown-exag050.wav
/home/c/big/git/long-way-home/voice-scratch/out-audition/cell8-janeway-pain100-getdown-exag130.wav
```

## Render times and clip durations

Rendered on this host, CPU-only, 24 kHz mono output. `load` and `generate` are the synthesizer's own
figures; `wall` is the whole invocation including the model load; `0.xx× rt` is audio seconds per generate
second (higher is faster). The brief's ~0.16× realtime is the baseline; these cells ran at 0.15–0.25× once
warm.

| # | load | generate | audio | realtime | wall |
|---|---|---|---|---|---|
| 1 | 13.4 s | 21.2 s | 1.28 s | 0.06× | 40.0 s |
| 2 | 8.1 s | 5.9 s | 1.30 s | 0.22× | 20.3 s |
| 3 | 7.9 s | 4.6 s | 0.94 s | 0.20× | 17.8 s |
| 4 | 7.9 s | 4.6 s | 0.78 s | 0.17× | 17.6 s |
| 5 | 7.9 s | 5.0 s | 1.08 s | 0.22× | 18.1 s |
| 6 | 7.9 s | 13.4 s | 2.06 s | 0.15× | 26.5 s |
| 7 | 7.9 s | 8.6 s | 2.14 s | 0.25× | 21.6 s |
| 8 | 8.1 s | 8.4 s | 1.98 s | 0.24× | 21.6 s |

Cell 1 (the first invocation) carries the cold load and is the outlier on `generate`; every later cell
reached the warm-cache ~8 s load. Total render wall, eight cells, ~184 s.

## What each cell sounded like — the earless proxy, described honestly

I am an agent and **have no ear**: I cannot answer "is it a scream?". As the closest available proxy I
measured each clip (duration, mean/peak level, F0 median and range, zero-crossing rate, spectral centroid,
harmonic fraction) and transcribed it with whisper-tiny.en ASR. `docs/evidence/voice-review.md` finding
four validated this kind of proxy for *screening*, not for verdicts. Numbers beside the words, then the
honest reading.

| # | mean | peak | F0 med | F0 range | ZCR | centroid | ASR heard |
|---|---|---|---|---|---|---|---|
| 1 | −18.8 dB | 0.0 dB | **none** | — | 0.203 | 2533 Hz | *"end goes."* |
| 2 | −14.5 dB | −0.1 dB | 413 Hz | 203–466 Hz | 0.192 | 2062 Hz | *"Here, no."* |
| 3 | −13.5 dB | 0.0 dB | 841 Hz | 699–850 Hz | 0.294 | 3055 Hz | *"You're the reason!"* |
| 4 | −14.6 dB | 0.0 dB | 464 Hz | 408–475 Hz | 0.142 | 1674 Hz | **"No!"** |
| 5 | −17.6 dB | −0.9 dB | 318 Hz | 244–347 Hz | 0.119 | 1865 Hz | **"NO!"** |
| 6 | −22.5 dB | −1.7 dB | 61 Hz | 60–65 Hz | 0.112 | 1508 Hz | *"Get down, that console is going."* |
| 7 | −12.7 dB | 0.0 dB | 232 Hz | 107–368 Hz | 0.169 | 2250 Hz | *"Get down! That console's going!"* |
| 8 | −12.7 dB | 0.0 dB | 262 Hz | 112–406 Hz | 0.140 | 2063 Hz | *"Get down! That console is going!"* |

- **Cell 1 (jaworski, dialogue control).** The proxy found no stable voiced pitch and did not recover
  *"No!"*; it is the quietest jaworski cell (mean −18.8 dB) but the most spectrally diffuse after cell 3.
  Reading: a short, clipped, unpitched burst rather than a clean spoken word. **This is the control being
  noisy, which matters for cell 2.**
- **Cell 2 (jaworski, pain100, exag 0.5).** 4.3 dB louder than the control and a clear F0 near 413 Hz; the
  ASR recovered "no". Reading: a high, loud, shouted *"No!"* — the scream register arrived.
- **Cell 3 (jaworski, pain100, exag 1.3).** The extreme cell. F0 pinned at 841 Hz — essentially the
  reference's own 836 Hz — the highest zero-crossing rate (0.294) and lowest harmonic fraction (0.03) of
  any cell, and the ASR did **not** recover the text. Reading: a short, very high, strained burst whose
  content is gone; near the noise boundary. **Pushing exaggeration further on a short scream segment
  distorted rather than helped.**
- **Cell 4 (jaworski, death3).** 464 Hz, and the ASR recovered *"No!"* exactly. Reading: the cleanest
  screamed *"No!"* of the jaworski cells — a shout, intelligible.
- **Cell 5 (jaworski, pain25).** 318 Hz, the least loud of the scream cells (−17.6 dB), ASR *"NO!"*.
  Reading: the mildest of the three samples gives the mildest shout — **severity of the sample tracked the
  severity of the result**, which is what the matrix's cell 5 was for.
- **Cell 6 (janeway, dialogue control).** The quietest clip of all (−22.5 dB, peak −1.7 dB) and a very low
  F0 (61 Hz — at the estimator's floor, so read it as "breathy/low", not as a measured pitch). Reading: an
  even, conversational line, exactly the control it is meant to be. ASR perfect.
- **Cell 7 (janeway, pain100, exag 0.5).** **9.8 dB louder than its control**, F0 wrapped up to 232 Hz
  (range 107–368 Hz), and the ASR put exclamation marks on it — *"Get down! That console's going!"*.
  Reading: the scream condition clearly imported; this is the strongest evidence in the matrix, because the
  same voice and the same line rose in level and pitch *only* because the reference changed.
- **Cell 8 (janeway, pain100, exag 1.3).** The same loudness as cell 7, a little more pitch movement
  (262 Hz, range to 406 Hz), a shorter clip, and the ASR recovered the full line with punctuation.
  Reading: a slightly more urgent shout than cell 7, not a distorted one — for janeway, 1.3 did not break
  the line the way it broke jaworski's cell 3.

**No cell produced outright noise.** Cell 3 is the one that left intelligibility behind.

## Did the reference's condition import?

**Yes, and it is visible in the numbers.** The reference rule says a reference imports its recording
condition; here every scream-referenced cell moved the clone's register toward the reference's — higher
F0 and higher level, one variable changed:

- jaworski: control (cell 1, no measurable F0, −18.8 dB) → pain100 (cell 2, 413 Hz, −14.5 dB) →
  death3 (cell 4, 464 Hz) → pain25 (cell 5, 318 Hz);
- janeway: control (cell 6, 61 Hz, −22.5 dB) → pain100 (cell 7, 232 Hz, −12.7 dB).

Because the screams are single dry utterances, no concatenation seam was available to import — only the
register was, and it was. Two model warnings are worth recording as the shape of that import: **"Reference
mel length is not equal to 2 * reference token length"** fired for 7 of 8 cells (all but cell 5), and
**"Audio values outside normalized range"** fired for cells 1 and 6 — the *dialogue controls*, i.e. the
long 8–9 s references, not the screams. The screams are simply far shorter than the conditioning window the
model expects.

## What I could not determine without the owner's ear

- **Whether any cell is a scream** rather than a loud, high shout; and whether cell 3 is "strained" or
  "broken".
- **Whether cell 7/8 sound like *Janeway* under stress**, or merely like a high, loud render of the same
  voice — the proxy cannot judge identity (`voice-review.md`, finding four).
- **Whether cell 5's mildness is useful** or too subtle next to cell 2; the proxy says it is quieter and
  lower, not that it is better or worse.
- **Whether the cell-1 control itself is a clean jaworski** — the proxy could not find pitch in it, and
  nothing here hears it.

## Judgement calls, named as calls

1. **Copying the weights into `/home/c/.cache/huggingface` instead of downloading, and then letting the
   brief's default-home rule hold.** It satisfied "a second round does not re-download 3.0 GB" while
   costing `/` nothing it could not afford after cleanup. Reported above.
2. **Reclaiming the disk with `uv cache clean`.** The floor was breached by the resolver's CUDA detour, not
   by the audition's inputs; the cache is regenerable and the shared host is better off with the 14 G.
3. **`--cfg-weight` and `--temperature` left at defaults**, so each cell moves exactly one variable from its
   control. The brief varied only `exaggeration`.
4. **The ASR and prosody numbers are a screening proxy, not the acceptance test** — the same standing as
   the spike's ASR check. The verdict is the owner's.
5. **The `misc/` samples were conditioned with the analyzer's own chain, not the analyzer itself**, because
   `analyze.py reference` cannot name a file. The helper is scratch and recorded above so the pass repeats.
6. **The jaworski control is two concatenated lines** (the analyzer's own pick), so its reference rule
   violation is inherited from `analyze.py reference`, not chosen here; the janeway control is one line.

## The boundary, kept

No generated audio, no reference clips and no extracted game assets are committed — not to this file, not
to the branch, not to an issue, not to a pull request. The retail install was read only; the venv, the
references and all eight clips live under the gitignored `voice-scratch/` on this host. **No engine process,
no agent session and no stray venv was left running.** Nothing was committed to `main`, and no code PR was
opened.
