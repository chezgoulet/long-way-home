# Contributing & workflow

## Branch flow

- `main` — releases only.
- `testing` — the integration branch. **All work targets `testing`.**
- `feature/<name>` — branched from `testing`, PR'd back to `testing`.
- `hotfix/<name>` — branched from `main`, PR'd to `main` and `testing`.

Note: GitHub only auto-closes an issue when a PR merges into the *default* branch. Since PRs
target `testing`, close issues by hand with a comment naming the merge commit.

## Gates

Work is organised by gate, not by calendar. See `docs/program-charter-v3.md`:

- **G0** — module builds natively + native script compiler (byte-compared against 2,175
  shipped scripts) + asset validator (with negative tests).
- **G1** — client playable: first campaign mission completed, save and reload.
- **G2** — Virtual Voyager acceptance, and no base-campaign regression.
- **G3** — reactive crew: 5–10 NPCs, one deck, no new animations, measurable criteria.
- **G4** — living ship. Does not start until G3 passes.
- **G5** — capstone scenario, judged by playtest.

Nothing downstream starts before its gate passes.

## Evidence rules

Every claim of "done" ships with the artifact that proves it: a symbol report, a byte
comparison, a passing negative test, a log. A green build is not a green gate.

## Licensing of contributions

By contributing you agree your work is licensed GPL-2.0 for code, except inside `module/`
which follows Raven's STEF terms. Do not commit game assets — see `.gitignore`.
