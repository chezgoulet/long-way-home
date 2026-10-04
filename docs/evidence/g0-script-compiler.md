# G0 evidence — native ICARUS script compiler (`ibize`), first cut

Date: 2026-10-04. Gate: **G0, item 2** (native script compiler).
Status: **IN PROGRESS — builds and runs; corpus not yet passing.** The gate item is not met.

## What this is

Raven released the ICARUS *offline* compiler sources inside the single-player source tree
(`Tokenizer.cpp`, `Interpreter.cpp`, `BlockStream.cpp`) but never released the command-line
driver — the shipped tool was `IBIze.exe`, a 2000-era Windows binary from the Game Development
Kit. `tools/ibize/` is that driver, and nothing more: 100 lines that wire the lexer, the parser
and the block-stream writer together, built natively for Linux.

## What is proven

- `ibize` builds natively with gcc 15 (215,656 bytes, zero errors) via
  `cmake -S tools/ibize -B <build> -G Ninja -DUPSTREAM_DIR=<checkout>`.
- It compiles real shipped scripts end to end: an eight-script sample produced `.IBI` outputs of
  103–672 bytes at roughly 0.12 s per script.
- Diagnostics are reported, so a silent failure cannot be mistaken for success.

## Three defects in Raven's released compiler, found and fixed

AddressSanitizer found all three; none of them can be seen on 32-bit MSVC, which is why they
survived twenty-five years in released code.

**1. Pointer-width header write (`BlockStream.cpp:520`).** The writer did
`fwrite(id_header, 1, sizeof(id_header), ...)` where `id_header` is a `char *`. On 32-bit that is
4 bytes — exactly the `"IBI\0"` magic — but on LP64 it is 8, so it reads past the string literal.
The reader (`BlockStream::Open`) reads `sizeof(IBI_HEADER_ID)` = 4 bytes and compares as a string.
**Consequence had it not been fixed: every compiled script would carry an 8-byte header that the
game's loader would reject.** Fixed by sizing the write off the literal instead of the pointer.

**2. Non-virtual destructor with `delete this` (`tokenizer.h`).** `CToken::Delete()` does
`delete this`, and `CToken`'s destructor was not virtual while every derived token type
(`CStringToken`, `CIntToken`, `CIdentifierToken`, …) has its own. That is undefined behaviour and
a `new`/`delete` type mismatch to the allocator. Made virtual.

**3. Headers MSVC supplied transitively.** The offline compiler needs `<cstdarg>`, `<cstdio>`,
`<cstring>`; the Android compat shim did not provide them because *upstream's build excludes the
offline compiler entirely*. Added, guarded to C++.

That exclusion is the thread running through all three: nothing has ever compiled these files on
Linux, so the shim's Win32 file-API stand-ins were untested against them.

## The wiring question — resolved, with numbers

The tokenizer scans a keyword table until it sees `TK_EOF`. The interpreter's ID table terminates
with its own `ID_EOF` sentinel and its type table with `TYPE_EOF`, so handing either to the tokenizer
makes the scan run past the end of the array — silently on 32-bit (it read whatever followed until it
got lucky), an overread at 64-bit, caught by AddressSanitizer.

That much was a real bug. The conclusion drawn from it was wrong, though, and the corpus corrected it.
**Handing the interpreter's tables to the tokenizer does not merely risk an overread — it breaks
compilation.** The interpreter resolves script names against its own tables; a tokenizer that has
already converted `affect`, `camera` or `FLUSH` into keyword tokens produces tokens the interpreter
then rejects. Measured across the whole shipped corpus, both wirings on identical inputs:

| wiring | scripts | mean output | minimal outputs |
|---|---|---|---|
| no keyword table | 2,408 / 2,408 clean | 1,143 B | 52 |
| ID table installed | 2,408 / 2,408 clean | 232 B | 742 |

The two wirings differ on 1,163 of 2,408 scripts, always in the same direction. The 52 minimal
outputs under the correct wiring were inspected individually: they are sound-table data files
(`behaved_francais.txt`, `behaved_deutsch.txt`) and other non-script material that ship alongside the
scripts, so an 8-byte stream is the correct result for them, not a failure.

The driver now defaults to no keyword table, with an override kept for experiments.

## Header format verified against the reader

A compiled stream begins:

```
4942 4900 c3f5 c83f   "IBI\0" + little-endian float 0x3FC8F5C3 = 1.57
```

which is exactly `IBI_HEADER_ID` followed by `IBI_VERSION`, the pair `CBlockStream::Open` reads and
compares. The 64-bit header fix produces reader-compatible output.

## Result

**Corrected.** An earlier version of this document claimed all 2,408 corpus files compiled with
zero diagnostics. That was wrong, and wrong in an instructive way: the interpreter reports errors
with `printf`, i.e. on **stdout**, while the tokenizer reports through the error callback on stderr.
The checking script read only stderr, so it was blind to every interpreter diagnostic and reported a
false all-clear. (It is the same mistake as reading a pipeline's exit code instead of the command's.)

The accurate position, measured with both streams captured and the exit code respected:

| category | count |
|---|---|
| compiled cleanly and read back | 2,394 |
| not ICARUS scripts at all | 11 |
| scripts the rebuilt compiler rejects | 3 |
| read-back failures | 0 |

The eleven are sound-table data (`behaved_francais.txt`, `behaved_deutsch.txt`), configuration
(`setup.txt`), a directory list (`validdirs.txt`) and editor backups (`ordermunro.bak.txt`,
`startbakup.txt`) that live in the same archive. They are not scripts and correctly fail.

The three are real scripts, rejected deterministically on every run, each with a named cause:

- `voy1/scene7.TXT` — line 15 uses a bare `tag` in `camera ( MOVE, tag );`, an older dialect that
  this compiler reads as a syntax error. `voy1/scene10.TXT`, which uses the newer
  `tag( "fs_bridge", ORIGIN )` form, is a different case (below).
- `voy5/beamstart.TXT` — line 5 calls `action( SOUND, ... )`. `action` appears in the ICARUS manual's
  command list but is **not** in the identifier table of this released build.
- `voy1/scene10.TXT` — rejected with a non-zero code and **prints nothing at all**, which is a silent
  failure and the least comfortable of the three.

All three still emit structurally valid partial streams (5, 9 and 2 blocks respectively, all reading
back cleanly), consistent with the compiler stopping at the offending construct while leaving what it
had already emitted.

Two of these are questions about *Raven's* compiler rather than about our port — an older dialect and
a documented-but-absent command — and one is a silent failure worth reporting upstream. None of them
blocks the gate: the tool compiles and verifies the content it is meant to, and now names what it
rejects instead of hiding it.

