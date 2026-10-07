#!/usr/bin/env python3
"""The synthesizer: one line a character never said, from lines they did.

Chatterbox (Resemble AI, MIT) clones a voice from a few seconds of reference and
**watermarks every clip it produces**. The reference comes from the analyzer,
built out of the retail voice assets the player owns; the output goes to a path
the player names and is never committed. The repository carries the mechanism and
never the audio — see docs/asset-doctrine.md.

    synthesize.py --reference ref.wav --text "..." --out line.wav

Dependencies (checked against their own LICENSE files): chatterbox-tts (MIT),
torch (BSD-3). Install them in a throwaway virtualenv; do not vendor them.
"""

import argparse
import os
import time

import torch
import torchaudio


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--reference", required=True, help="the reference wav to clone from")
    ap.add_argument("--text", required=True, help="the line to speak")
    ap.add_argument("--out", required=True, help="where to write the wav (never in the repo)")
    ap.add_argument("--device", default="cuda" if torch.cuda.is_available() else "cpu")
    args = ap.parse_args()

    from chatterbox.tts import ChatterboxTTS  # imported late: the model load is the slow part

    t0 = time.time()
    model = ChatterboxTTS.from_pretrained(device=args.device)
    loaded = time.time()
    wav = model.generate(args.text, audio_prompt_path=args.reference)
    generated = time.time()
    torchaudio.save(args.out, wav, model.sr)
    secs = wav.shape[-1] / model.sr

    print(f"device    : {args.device}")
    print(f"load      : {loaded - t0:.1f}s")
    print(f"generate  : {generated - loaded:.1f}s for {secs:.2f}s audio "
          f"({secs / (generated - loaded):.2f}x realtime)")
    print(f"output    : {args.out}  {os.path.getsize(args.out)} bytes  sr={model.sr}")
    print("note      : Chatterbox watermarks this clip; it is local and never committed.")


if __name__ == "__main__":
    main()
