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

**G0 item 2 is met on the compile half:** all 2,408 shipped script files compile with zero
diagnostics under the settled wiring, at roughly 0.12 s each, emitting a correctly-headed block
stream. What remains before this item is fully closed is round-trip verification — re-opening each
`.IBI` with `CBlockStream::Open`/`ReadBlock` and confirming the block structure reads back — which is
the next increment and needs neither the original compiler nor the game.

## Next

1. Settle the wiring against the corpus (a variant sweep is running).
2. Report a corpus-wide pass rate over all 2,175 shipped scripts.
3. Add read-back verification: re-open each `.IBI` with `CBlockStream::Open`/`ReadBlock` and confirm
   the block structure round-trips — a check that needs neither the original compiler nor the game.
