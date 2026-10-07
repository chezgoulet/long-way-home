# Evidence — Janeway: a cheeseburger, and an order under pressure

Date: 2026-10-07. Branch `feat/janeway-cheeseburger`, cut from `testing`. The question this pass asks:

> **Janeway's shipped lines are bridge dialogue. Does the mechanism carry her out of that domain — a
> register she never uses in the game — recognisably, and does it carry the *performance* (prosody) or only
> the *timbre* of the voice?**

The mechanism is `tools/voice/analyze.py` and `tools/voice/synthesize.py`, and it is **unchanged** by this
pass. The two MP3s are local and are **not** in this repository. They are for the owner's ear, and nothing
here substitutes for it.

**Read the last pass first.** The method is re-run from `docs/evidence/voice-examples.md`, which at the time
of writing lives on the **unmerged branch `feat/voice-examples`** (not on `testing`), so it was read from the
ref: `git show origin/feat/voice-examples:docs/evidence/voice-examples.md`. Its install sequence, its trap,
and its caveats all carry over and are not repeated in full here; what differs is recorded below.

## The reference

`janeway` is in the shipped `voice/` tree. The census counts **97 dialogue lines, 6.6 min of speech** — the
brief's 97, exactly. (The brief's "122 files" I could **not** reproduce: the paks name **120** unique paths
under `sound/voice/janeway/` — 97 `.mp3` plus 23 directory entries. Recorded as a discrepancy, not smoothed
over; the 97 dialogue lines is the number that matters.)

Built exactly as the last pass did, at `analyze.py` defaults (`--lines 2 --target-sec 10`):

```
tools/voice/analyze.py reference --game "$GAME" --character janeway \
    --out voice-scratch/refs/out/janeway.wav
```

It chose **one line**, not two:

| | |
|---|---|
| chosen line | `sound/voice/janeway/voy1/backtogether.mp3` |
| source line length | 8.67 s |
| reference after silence-strip | **7.99 s** (7.988 s), 255,816 B, 16 kHz mono s16 |

One clean utterance, so the reference is a single-speaker pass with no concatenation — unlike the last
pass's computer reference, which was its weakest result.

## The two lines

1. **The line he asked for** (text used exactly):
   *"I'd like a cheeseburger, with everything on it, and coffee — black. And that's an order."*
2. **The urgency test** — invented for this pass, in the fiction of *Long Way Home*, not a show quote:
   *"All hands, brace for impact! Seal every bulkhead and report damage the moment it clears!"*

## The two MP3s, for the owner to play

All under `/home/c/big/git/lwh-voice/voice-scratch/out/` (gitignored; never committed, never uploaded):

```
/home/c/big/git/lwh-voice/voice-scratch/out/janeway_burger.mp3
/home/c/big/git/lwh-voice/voice-scratch/out/janeway_command.mp3
```

## The numbers beside each

Hardware: CPU only, 12 cores, no GPU; `torch 2.6.0+cpu`. Model load **7.8 s** warm (the burger line's 67.8 s
load is the **cold** load, which included the one-time weight download); peak RSS ~6.87 GB.

| clip | render time (CPU) | wav duration | wav size | mp3 128 kbps | mp3 duration | mp3 size |
|---|---|---|---|---|---|---|
| janeway_burger | 35.3 s (0.14× realtime) | 4.78 s | 229,518 B | 128.7 kbps | **4.848 s** | **77,997 B** |
| janeway_command | 29.2 s (0.21× realtime) | 6.10 s | 292,878 B | 128.6 kbps | **6.168 s** | **99,117 B** |

Wall clock: burger 1:52.57 (cold, download included), command 0:43.72 (warm). Loudness (mean / peak): burger
−19.4 / −0.5 dB, command −18.5 / −0.4 dB. Wavs are 24,000 Hz mono PCM s16.

## The install sequence, including the setuptools pin

Re-run from scratch so the next pass does not rediscover the trap:

```sh
cd /home/c/big/git/lwh-voice
export HF_HOME=/home/c/big/git/lwh-voice/voice-scratch/hf-cache   # cache lands beside the venv, and is kept

uv venv --python 3.12 voice-scratch/venv

uv pip install --python voice-scratch/venv/bin/python \
    "torch==2.6.0+cpu" "torchaudio==2.6.0+cpu" \
    --extra-index-url https://download.pytorch.org/whl/cpu \
    "chatterbox-tts==0.1.7" "setuptools==80.9.0" "sherpa-onnx==1.13.8"

# VERIFY the watermarker before rendering — the trap is here, not at render time:
voice-scratch/venv/bin/python -c "import perth; print(perth.PerthImplicitWatermarker)"
# -> <class 'perth.perth_net.perth_net_implicit.perth_watermarker.PerthImplicitWatermarker'>
```

- Pinning `torch==2.6.0+cpu` / `torchaudio==2.6.0+cpu` against the PyTorch CPU index **in the same resolver
  command** is what stops a CPU-less host downloading the ~2.5 GB CUDA build. Resolved: `torch 2.6.0+cpu`,
  `torchaudio 2.6.0+cpu`, `transformers 5.2.0`.
- **The trap: `setuptools==80.9.0`.** Chatterbox's watermarker (`perth`) imports `pkg_resources`, which
  setuptools 84 no longer ships; the import fails silently into `None` and the render dies *after* the model
  has loaded. `perth` is not dropped by pinning setuptools; with `80.9.0` it imports and prints its class.
  This is an environment fix, not a change to `tools/voice/`, which was not modified.
- `sherpa-onnx==1.13.8` is the pass's **own** ASR check, not a mechanism dependency.

Cost: venv **1.7 GB**, Hugging Face cache **3.0 GB**, scratch total **4.7 GB**. Scratch is on `/home/c/big`
(383 GB free). The shared `uv` cache at `~/.cache/uv` is **8.6 GB** and sits on the **root** drive (12 GB
free); it was **not** touched — see the judgement call below.

## The mechanism check (ASR), and a prosody measurement

**An agent has no ear.** As the closest available proxy I transcribed both clips with a borrowed on-device
ASR model (Whisper-tiny.en, via `sherpa-onnx`), using **both** the int8 and float models — a *mechanism* check
(is it speech? is it on-text?), explicitly **not** an acceptance test:

| clip | int8 | float |
|---|---|---|
| burger | *"I'd like a cheeseburger with everything on it and coffee, black, and that's an order."* | same |
| command | *"All hands, brace for impact, seal every bulkhead and report damage the moment it clears."* | same |

Both read back **cleanly and on-text with both models** — a stronger result than the last pass, where the
computer clip garbled "nacelle" and returned empty text from the int8 model. There is no garbled word here,
including the out-of-domain words "cheeseburger" and "coffee".

To give the timbre-vs-prosody question a number as well as an ear, a local autocorrelation F0 track (crude,
60–400 Hz):

| | duration | speaking rate | F0 mean | F0 p10–p90 | F0 sd |
|---|---|---|---|---|---|
| reference (janeway) | 7.99 s | — | 143.8 Hz | 103.2–183.9 Hz | 39.7 Hz |
| burger | 4.78 s | 3.14 words/s | 139.9 Hz | 97.4–169.0 Hz | 43.3 Hz |
| command | 6.10 s | 2.79 words/s | 144.5 Hz | 111.6–165.5 Hz | 37.3 Hz |

Both clips sit in the reference's pitch band and carry real pitch movement (sd ≈ 37–43 Hz), so this is not a
flat monotone. **But these are measurements, not the answer**: they cannot tell the owner whether the
*performance* is hers. The command line's pitch range is actually **narrower** and its rate **slower** than
the burger's — if he hears it as less urgent than the burger, that is consistent with the number, and neither
number settles it.

## How each one actually sounded to me

**First, the limit, plainly: I am an agent and I have no ear.** I cannot answer the question the clips exist
to answer, and I will not present a score as that answer.

- **janeway_burger** — by proxy, clean, on-text speech in the reference's pitch band with no crutch words or
  garble; the mechanism did not stall on the out-of-domain register. Whether it is *her* is the owner's call.
- **janeway_command** — by proxy, equally clean and on-text; pitch sits in band but its range is narrower and
  its rate slower than the burger's, so I would **not** claim it sounds more urgent — the proxy cannot hear
  urgency, and I have flagged the measurement rather than assert the experience.

**I cannot tell you either clip is good or poor.** The last pass could name its weakest clip (the computer
one) because the ASR *failed* on it; here nothing in the check fails, so I have no basis to call either poor —
and equally no basis to sign either off. The owner's ear is the acceptance.

## What I could not determine

- **Whether either clip sounds like Janeway at all**, and whether the out-of-domain register dilutes her. No
  ear. This is the open question, not an oversight.
- **Whether the clone carries prosody or only timbre.** F0 movement and speaking rate are measured above,
  but a machine that reproduces a voice and flattens the performance is exactly what a number cannot rule out.
- **The brief's "122 files"** — I count 120 unique pack paths (97 `.mp3` + 23 directory entries). Not
  reconciled.
- **The 2000-vintage ~58 kbps source encode's ceiling** on clone fidelity. Not measured.
- **The chain of title on the underlying performance** — the owner's legal reading in `docs/staff-meetings.md`,
  not something this pass can settle.

## Judgement calls, named as calls

1. **Two lines, as asked**: the exact cheeseburger text; and an invented, in-fiction, command-urgent line
   (*"All hands, brace for impact! …"*) chosen to test prosody against the burger's calmer register.
2. **One clean line as the reference**, not two — the analyzer's own choice at defaults, and the opposite of
   the last pass's two-line (weakest) computer reference.
3. **`HF_HOME` redirected into `voice-scratch/hf-cache`** so the 3 GB of weights sit beside the venv and are
   kept, per the brief.
4. **The venv and the model cache were left in place** (1.7 GB + 3.0 GB), as instructed. The shared
   `~/.cache/uv` (8.6 GB, on the tight root drive) was **also left** — I did not run `uv cache clean`, because
   it is a shared per-user cache and clearing it could cost another session a re-download. Named here so the
   choice is visible; scratch itself is on `/home/c/big`, which has ample room.
5. **ASR, both models, as "how it sounded"** — a mechanism check, offered as such, explicitly not the
   acceptance test.
6. **Rendered once at defaults; not re-rolled.** Re-rolling to chase a proxy would be fitting to the check
   rather than to the ear.

## The doctrine, and a tension I will not smooth over

`docs/asset-doctrine.md` and `docs/staff-meetings.md` (*Voice sources, provenance, and the licence*) draw the
boundary this pass obeys: generate locally and **never distribute** — no generated audio in the repository,
in an issue, in a pull request, or in a document; the retail install read only; the owner's machine, the
owner's ear.

One sentence in `docs/staff-meetings.md` is **wider than the practice this pass was asked to carry out**: it
says *"nobody on this project runs the tool to produce an actor's voice and puts the result anywhere."* A
generated clip for a living performer's character does exist on this machine as a result of this pass. What
reconciles it is the **distribution** boundary, not any change to it: the clips never leave `voice-scratch/`
(gitignored), are not published, shared or uploaded, and the repository carries only this evidence and no
audio. The last accepted pass did the same (tuvok, alexa, munro are living performers' characters). If the
owner means that sentence literally — that the tool is never run for a performer's character at all, even
locally, even for his ear — then this pass is out of bounds and the sentence should be corrected, because it
currently reads as a flat prohibition and the practice is a bounded one. **Recorded as a tension, not
resolved here.**

## The boundary, kept

No generated audio, no reference clips and no extracted game assets are committed — not to the branch, not to
this file. Everything the pass produced lives under `voice-scratch/`, which is gitignored. The retail install
was read only. Nothing was published, shared or uploaded. Nothing is committed to `main`.
