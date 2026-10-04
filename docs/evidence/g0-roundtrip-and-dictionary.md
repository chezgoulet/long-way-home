# G0 evidence — round-trip verification, and the entity dictionary

Date: 2026-10-04. Covers the verification half of **G0 item 2** and groundwork for **G0 item 3**.

## Round trip: every compiled script read back by the game's own reader

`tools/ibi-dump` opens a `.IBI` with `CBlockStream::Open` — which validates the `"IBI"` magic and the
1.57 version field exactly as the game's loader does — then walks it with
`BlockAvailable`/`ReadBlock` and reports the structure.

Run over every compiled script in the shipped corpus:

| result | count |
|---|---|
| OK (header valid, all blocks read) | 2,408 |
| bad header or version | 0 |
| truncated block | 0 |
| unreadable | 0 |

**2,408 of 2,408.** Scripts that compile also *load*, as far as the released reader is concerned.
Example: `voy1/turbolift.txt` → 24 blocks, 26 members, 9 distinct block ids.

## The entity dictionary

`tools/entitydict/entitydict.py` turns Raven's editor dictionaries into machine-readable form and
cross-checks them against the entity classes that actually appear in the shipped map sources. This is
the check that keeps a dictionary honest: a dictionary that does not cover what the game's own maps use
cannot gate new content.

Sources merged: `SP_entities.def` (single player) and `HM_entities-def.txt` (Holomatch), both from the
official Game Development Kit. Map sources: all eleven published archives — Voyager, Borg, Holomatch,
Virtual Voyager, Stasis, Forge, Scavenger, Dreadnought, Holodeck, CTF and Brig — 106 map files.

| measure | value |
|---|---|
| classes documented | 262 (+2 family templates) |
| classes used across shipped maps | 239 |
| used and documented | 237 |
| **used but undocumented** | **2** |

The two residual classes are `light_generic_street_old` (3 instances) and
`misc_model_scav_sc_scanner` (1 instance) — map-specific variants of documented families, and the only
genuinely undocumented content found in twenty-five years of shipped level data.

### Two parsing findings worth keeping

- **The `QUAKED` line format is not uniform.** Colour is always present, but the mins/maxs bounding
  pairs are optional (classes that inherit bounds omit them), the spawnflag field may be a bare `?`,
  and the class name may be glued to the colour group (`NPC_HunterSeeker(1 0 0)`). A parser that
  assumes the canonical shape silently drops about a tenth of the file and mangles the rest — the
  first version of this tool did exactly that, and the map cross-check is what exposed it.
- **Family templates exist.** The dictionaries document `item_*****` and `weapon_*****` as templates
  covering whole prefixes. Treating a wildcard as a single literal class makes it look like five
  undocumented classes; treating it as a prefix is the correct reading.

## What this gives the validator (G0 item 3)

The validator now has a trustworthy denominator: an entity class used in a scenario either resolves
against the merged dictionary (including family templates) or is reported by name. That is the first of
its checks, and the one that would otherwise have been guesswork.
