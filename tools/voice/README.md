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
