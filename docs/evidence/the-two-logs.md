# Evidence — the two logs: the official record and the private one

Date: 2026-10-06. Branch: `feat/the-two-logs`, cut from `feature/g3-reactive-crew`. Implements the
two-log half of `docs/the-record-and-the-log.md`. Save format **version 47**.

The official log was already built and is **not** rebuilt: `docs/evidence/gap-the-log.md` (the log as
an artifact, version 11) and `docs/evidence/the-month-report-and-the-toll.md` (the report, the diff,
the promise, the lie, the orphaned mark and the purge, version 44) were read in the working tree
first. What this branch adds is the **second store** — private, per person, an entry shape of
`time / who / what` and a visibility of its own — and the proof that the two stay separate.

Nothing here is asserted without the command that produced it.

---

## What is built

**State.** `Ship` gains a bounded `personalLog` of `PersonalLogEntry { time, owner, who, what,
visibility }`, distinct from the official `log` of `LogEntry { time, who, scope, what }`. The
personal entry carries `visibility = LOG_PERSONAL`; its owner is the crew member whose private log
it is. `WritePersonalLog` writes one in the owner's name and drops the oldest past
`PERSONAL_LOG_MAX = 128 [inv]`. Save format is now **version 47**.

**The visibility rule, in one place.** `PersonalVisibleTo(e, reader)` is true only when
`reader == e.owner`. `PersonalLog(s, owner)` returns exactly that person's entries and nothing else;
`ReadOfficialLog(s, count, scope)` is the official read and never returns a personal entry, whatever
the scope. This is the whole of "not a post's scope, not command": there is no other path to the
store.

**Control — the player's hand.** `ship personal [count]` reads the player's private log;
`ship personal write <text…>` writes one. The player's own store is `s.player`'s.

**The law over both.** The simulation writes the logs and never reads either: no decision anywhere
consults `log` or `personalLog`. `DraftReport` is built from the record's marks (`MEM_SAW`), never
from a log, and `PurgeLogs` clears the **published** log only — the private one is not published and
is left exactly as it was.

---

## Observed

`test_ship_core --personal` prints one PASS line for each acceptance item. Command and full output:

```
$ /tmp/opencode/lwh-crew-ship/test_ship_core --personal
PASS  two stores: 1 official entry, 1 personal entry
PASS  the private entry is invisible to the official read and to every scope: 0 match(es) across 9 scopes
PASS  another person's read returns nothing: 0 entries
PASS  the player wrote it and reads it back: "the death was my fault, and I will not write it down"
PASS  the month report's draft is unchanged by a personal entry: 3 line(s), identical
PASS  purge: published log 0 entries, private log 1 entries, orphaned mark 1
PASS  save and reload restore both stores byte-for-byte: official 1, personal 1
all demonstrations passed
```

The unit test (`TestTheTwoLogs`) covers the same ground and additionally asserts: a bad write changes
nothing; the private entry's `visibility` is `LOG_PERSONAL`; `PersonalVisibleTo` is true for the
owner and false for another; the report's lines are identical with and without the private entry,
and no drafted line contains the private text; the orphaned `MEM_LOG` mark survives the purge while
the private store does not move; and both stores survive save and load byte-for-byte
(`Pack(back) == blob`).

```
$ /tmp/opencode/lwh-crew-ship/test_ship_core
...
ship_core: all checks passed
$ echo $?
0
```

---

## The module compiles

The brief's one real collision is the compiled module, which lands outside the repository and is
shared by both trees. **I used my own build directory**, from a private clone, so no other lane's
`libefgame.so` could be overwritten or mistaken for mine:

```
$ git clone --no-hardlinks /home/c/big/git/upstream /home/c/big/git/lwh-crew-upstream
$ cd /home/c/big/git/lwh-crew-upstream && git checkout 0d8942e && git clean -qfd
$ for p in /home/c/big/git/lwh-crew/patches/*.patch; do git apply "$p"; done   # 16 patches
$ cmake -S efgame -B efgame/build-linux -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release \
      -DLWH_MODULE_DIR=/home/c/big/git/lwh-crew/module
$ cmake --build efgame/build-linux -j12
...
[100%] Built target efgame
-rwxrwxr-x 1 c c 2184736 .../efgame/build-linux/libefgame.so
-rwxrwxr-x 1 c c  601616 .../efgame/build-linux/libefui.so
```

**Build directory used: `/home/c/big/git/lwh-crew-upstream/efgame/build-linux`** (private to this
worktree; the shared `/home/c/big/git/upstream/efgame/build-linux` was never configured or written).

---

## The two required suites

```
$ scripts/test.sh ; echo $?
...
==> ship simulation: unit tests
ship_core: all checks passed
...
==> patches: numbered without gaps, and each one parses
    16 patches

all checks passed
0
```

```
$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts ; echo $?
...
==> compiler corpus pass (every shipped script: compile, then read back)
    2024 files: 2016 compiled and read back, 8 rejected by the compiler, 0 read-back failure(s)
      ... (all eight named) ...
    the 8 rejections are exactly the known, documented set
...
0
```

Both exit **0**. Entity dictionary: 318 classes, 212 with documented keys, 232 with spawnflag sets.

---

## What was read before writing (the law, checked rather than asserted)

A grep of `module/ship/ship_core.cpp` for reads of `s.log` finds three: `LogEvent` (writes),
`PurgeLogs` (empties), and `Pack`/`Unpack` (persists). No decision consults it, and `personalLog` is
the same. The one place a wrong read would be invisible is the month report, so it is demonstrated
rather than asserted above: the draft is **identical** with a private entry present and absent.

## Judgement calls, named

1. **One store with an owner, not a vector per person.** The private log is a single bounded
   `std::vector<PersonalLogEntry>`; `owner` names the person. The read is what enforces privacy
   (`PersonalLog` filters by owner), and the save stays one block. A `std::vector` inside each
   `CrewMember` would have changed the crew record's layout for no read the API does not already
   provide.
2. **A `visibility` field that is always `LOG_PERSONAL` today.** The design asks for "a visibility
   of its own", so the entry carries one and the save validates it. It is redundant with the store
   it sits in; it is there so the rule is a field rather than an accident of which vector it is in.
3. **The author is the owner's name at the time of writing.** `WritePersonalLog` freezes `who` from
   the crew record, so a later rename does not rewrite what was said. This matches the official
   log's `who`.
4. **The player's store is `s.player`'s.** `WritePersonalLog` takes any crew index, but the console
   wires only the player. NPC private entries (and the Collective reading them on assimilation) are
   the document's other acceptance items and are **not** built here.
5. **`ship personal` is a developer command, like `ship log` and `ship report`.** It is not gated to
   a console station; the ready-room terminal that would open it in the world is the deck-build item,
   as it is for the official log. The private read at the console is the player's; no other scope can
   ask for it.

## What could not be verified, and is said plainly

A transcript can show that nobody else can read the private entry, that the simulation never reads
it, and that a purge leaves it standing. **It cannot show that writing in it *means* anything to a
person** — whether the private log feels like the place you put the thing you cannot say. That is the
owner's walkthrough, and no evidence document can substitute for it.
