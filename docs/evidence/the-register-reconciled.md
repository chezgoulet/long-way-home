# Evidence: the ledger reconciled against the tree (Item 0)

**What this closes.** Five landings updated `docs/designs-against-code.md` and `docs/omissions.md` and
left `docs/gates.md` — the ledger and the authority on state — behind. **O15's row** still read *"The
slate itself is still to author, and the evaluator that decides a scenario fires is the next pass"*, and
both halves had shipped. This item corrects the ledger in place, against what is on the ref, and
reconciles the scenario count from the tree.

**Host:** static. The commands below were run on this tree; no engine, no game data.

## 1. What was stale, and what the tree says

```sh
git fetch -q origin --prune && git rev-parse --short origin/testing     # a02c617
ls -d scenarios/*/ | wc -l                                              # 11
```

- **The evaluator shipped.** `feat/the-evaluator` landed as **PR #150** (2026-10-13); the ledger already
  carried a `### The evaluator … ✅ built 2026-10-13` section, so the ledger **contradicted itself**: the
  section said built, the O15 row said "the next pass". The row is the stale side.
- **The nine declarations shipped.** `feat/the-nine-manifests` landed as **PR #154** (2026-10-14); the nine
  are `scenarios/*/scenario.json` manifests, validated and built by `scripts/scenario-check.sh` part four.
- **The start-state configurator's "next pass" line** at the way-in section was likewise superseded by the
  section immediately below it (built 2026-10-08).

## 2. The count, re-derived from the tree

The row named a **nine-scenario first slate**; `scenarios/` holds **eleven** directories.

```sh
for d in scenarios/*/; do
  printf '%s :: ' "$(basename "$d")"
  grep -o 'slate [0-9]*' "$d/scenario.json" 2>/dev/null | head -1 || true; echo
done
git log --diff-filter=A --format='%h %ad %s' --date=short -1 -- scenarios/deck04-watch/scenario.json
```

| directory | slate member? | added by |
|---|---|---|
| `deck11-gel-pack` | **yes — slate 1** | `b6dee54`, 2026-10-13 (the manifest pass) |
| `deck15-nineteen-relays` | yes — slate 2 | `ae4d87f`, 2026-10-14 |
| `deck08-price-of-fuel` | yes — slate 3 | `ae4d87f`, 2026-10-14 |
| `deck09-the-ones-who-stay` | yes — slate 4 | `ae4d87f`, 2026-10-14 |
| `deck08-the-shortcut` | yes — slate 5 | `ae4d87f`, 2026-10-14 |
| `deck01-eleven-minutes` | yes — slate 6 | `ae4d87f`, 2026-10-14 |
| `deck01-the-other-captain` | yes — slate 7 | `ae4d87f`, 2026-10-14 |
| `deck12-a-fire-a-breach-a-deck` | yes — slate 8 | `ae4d87f`, 2026-10-14 |
| `deck06-organising` | yes — slate 9 | `ae4d87f`, 2026-10-14 |
| `deck04-watch` | **no** | `110fc15`, 2026-10-05 (the G3 crew-authoring pass) |
| `testroom-watch` | **no** | `110fc15`, 2026-10-05 (the G3 crew-authoring pass) |

**Which number is right: the tree says two predate the slate, not three.** The eleven are the **nine**
slate members (each whose `canon_root` cites `docs/scenario-atlas.md`'s slate) **plus the two**
pre-existing crew scenarios, `deck04-watch` and `testroom-watch`. `deck11-gel-pack` was written a day
earlier (the manifest pass's worked example) but it **is** slate 1 — in the slate, not before it, which is
the distinction the dispatch's "three" turns on. The row now states all of this, so the count is settled
in the ledger and not in a dispatch.

## 3. The rows corrected

- `docs/gates.md`, **O15's row** — the slate is authored (nine manifests, PR #154) and the evaluator is
  built (PR #150); what remains is the nine as **played content** and the in-game loader verb. The count
  is reconciled in the row, with the two-predate finding.
- `docs/gates.md`, the manifest section's O15 note and the evaluator section's O15 note — both now read
  "the nine as played content remain", not "the nine scenarios are content and remain".
- `docs/gates.md`, the manifest section's evaluator sentence — "is the next pass" is corrected to name the
  PR that landed it.
- `docs/gates.md`, the way-in section's configurator sentence — "is the next pass" is corrected to point
  at the section below it.

`docs/omissions.md` O15 was **already correct** (the manifest and evaluator passes updated it) and needed
no change; `docs/designs-against-code.md` carries no O15 row.

## 4. What I verified by running it

```sh
bash scripts/docs-check.sh
```
```
==> docs: cited paths resolve
    ok
==> docs: owned numbers agree with their owner
    7 claim(s) registered, all agree with their owner
==> docs: register rows agree with the tree
    ... 72 register row(s) annotated, all agree with the tree
==> docs: the hook register's stated counts agree with its rows and the surface
    docs/hook-register.md: counts agree with the rows and the surface (498 hooks: play 300, console 159, nothing 34, content 5)
```

```sh
bash scripts/test.sh
```
```
all checks passed
```

(The full transcript is the run's own log; both checks exit 0.)

## 5. What I could not verify, and why

- **Whether a scenario reads as a situation.** That is the owner's eye, and no check on a static host
  decides it (`docs/evidence/the-evaluator.md`, §9).
- **The in-game loader verb** — nothing in the game loads a `*.scenario` and recruits it in play yet; that
  is the playtest host's, and it is named in the corrected O15 row as what remains.

## 6. What I deliberately left unbuilt

- **Nothing in this item is code.** It is the ledger reconciliation the queue names; the code items
  (Items 1–6) follow on their own branches.

## 7. Judgement calls, named as calls

1. **I treated O15's row as the stale side, not the evaluator section.** The ledger contradicted itself
   (the section said built; the row said "the next pass"); where the ledger disagrees with itself, the
   later-dated built section is the tree's own witness and the row is corrected.
2. **I re-derived the count from each directory's `canon_root` and its adding commit**, rather than
   choosing between the dispatch's "three" and the row's "nine". The tree says two predate the slate; I
   recorded that and why `deck11-gel-pack` is not a third.
3. **I corrected the two other "is the next pass" lines** I met while passing (the manifest's evaluator
   sentence and the way-in's configurator sentence), because the instruction is to correct stale state in
   the same commit that makes it stale. The away-site "next pass" lines (1036, 1040) are **left**: the
   opening *is* still the next pass, and Item 1 is where it lands.
