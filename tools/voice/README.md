# tools/voice — the analyzer and the synthesizer

The voice spike's mechanism, and nothing more. Two small scripts that let this project **generate a
line a character never said, from the lines they did say, on the player's own machine**. The
repository carries the mechanism and never the audio: see `docs/asset-doctrine.md` and the ruling in
`docs/staff-meetings.md` (*Voice sources, provenance, and the licence*). The record of the spike — what
the retail assets are, what was installed, and the numbers behind the one clip that was produced — is
`docs/evidence/voice-spike.md`.

- `analyze.py` — find the retail voice assets and distil one clean reference clip from a character's
  lines. It reads the game the player owns and writes only to a path you name.
- `synthesize.py` — clone that voice and speak one new line, using Chatterbox.

## What stage one found, in one line

The retail install **already separates the dialogue per character** (`sound/voice/<character>/`, one
utterance per file) and from the music and ambience, so there is nothing to diarize and nothing to
separate. The pipeline is therefore: decode → pick a line → clone.

## Usage

```sh
GAME="/path/to/Star Trek Elite Force"        # the directory that contains BaseEF

# who has how many lines
python3 tools/voice/analyze.py census --game "$GAME"

# one character's reference clip
python3 tools/voice/analyze.py reference --game "$GAME" --character tuvok --out /tmp/ref.wav

# one new line, from the mechanism installed in a throwaway venv
python3 tools/voice/synthesize.py --reference /tmp/ref.wav \
    --text "Logic is a beginning, Captain, but it is rarely the whole of the answer." \
    --out /tmp/line.wav
```

Write output to a scratch directory, never into the repository. `analyze.py` needs `ffmpeg`/`ffprobe`
(present on this host); `synthesize.py` needs `chatterbox-tts` and `torch`.

## Dependencies, and their licences

Checked against each project's own `LICENSE` (`gh api repos/<owner>/<repo> --jq '.license.spdx_id'`):

| dependency | licence | role |
|---|---|---|
| `chatterbox-tts` (Resemble AI) | MIT | the synthesizer; clones from a few seconds and watermarks every clip |
| `torch` / `torchaudio` | BSD-3 | runtime for the synthesizer |
| `ffmpeg` / `ffprobe` | LGPL/GPL | decode and measure in the analyzer |

Not used: `demucs` (MIT) and `silero-vad` (MIT) — the assets made them unnecessary. Not used:
`OmniVoice` (Apache-2.0); Chatterbox installed cleanly first.

## The three knobs, and the reference rule (owner's review, 2026-10-07)

Read from the installed model, not recalled. `ChatterboxTTS.generate` takes:

```
generate(text, repetition_penalty=1.2, min_p=0.05, top_p=1.0,
         audio_prompt_path=None, exaggeration=0.5,
         cfg_weight=0.5, temperature=0.8)
```

- **`exaggeration` (default 0.5)** — the emotion control; it enters the model as `emotion_adv`. **This is
  delivery direction, and it is ours to set per line**: up for urgency, down for the flat and procedural. A
  render made at the default carries the default's emotional temperature **whatever the text says** — an
  emergency order and an order for lunch come out the same until someone asks for a difference.
- **`cfg_weight` (default 0.5)** — prompt adherence against fluency. **This is the lever for drift.** When a
  voice wanders off the reference as the utterance lengthens, raise it. The model's own prior carries an
  accent, and it takes over as the reference's influence decays.
- **`temperature` (default 0.8)** — sampling variety. Lower it for consistency across one character's lines.
- **There is no `language` parameter.** The accent **cannot be pinned by a setting**; it is fixed in the
  reference or in `cfg_weight`.

**And the reference rule, which the owner's review earned:** ***one line, one speaker, one recording
condition, as dry as the source allows.*** A reference built by concatenating two lines **buys duration at the
cost of the room**: it joins two conditions, and the seam becomes an artefact in every line that voice ever
speaks. A single clean utterance beats a longer joined one.

**And line length is a rendering constraint, not only a writing choice.** Condition per line and render per
line; do not render a long passage in one call.
