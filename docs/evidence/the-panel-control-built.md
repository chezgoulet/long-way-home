# Evidence: the in-world control at the panel (O10 / G20, the panel half) — built

**O10's panel half is built and demonstrated.** The recon (`docs/evidence/the-panel-control.md`)
proved the mechanism and left the decision; the owner made it (2026-10-15, *mark the screens as the
affordance*) and this pass builds it. The door half of **G20** (sealed quarters' world prompt) is
**not** this pass; it is registered as its own omission (O21 in `docs/omissions.md`).

## What a panel is, enumerated from the code

The list is the module's own (`module/ship/lwh_panel.cpp`): `PANEL_REMAPPED[]` -- the three console
shaders the module remaps -- plus `PANEL_READOUT`, the shader the readout is drawn into:

```
textures/transporter/panel, textures/sickbay/panel12, textures/engineering/conpanel
+ textures/lwh/panel
```

A panel is the brush whose surface carries `textures/lwh/panel` (the ship's own readout, the image
`gi.UpdatePanelImage` overwrites). Enumerated from the **built** ship map with the module's own list
(`tools/shipmap/markpanels.py`; it reads the list out of the module source, so the enumeration moves
with the remap):

```
==> the panels, from the module's list
panel list (from lwh_panel.cpp): textures/transporter/panel, textures/sickbay/panel12, textures/engineering/conpanel + textures/lwh/panel
  readout brushes still in worldspawn: 0
  console-shader brushes (the read, not marked): 90 spanning engineering/conpanel, sickbay/panel12, transporter/panel
```

* **The readout brush** -- the screens the deck tooling authors with `lwh/panel` (nine on the built
  ship: deck 6's holodeck console and status screen, deck 7's core console and status screen,
  deck 12's status screen, deck 13's plant panel and status screen, deck 14's holodeck panel and
  status screen). These carry the whole 128x128 image; they are the affordance.
* **The console-shader brushes** -- 90 worldspawn brushes on the published decks (transporter room,
  sickbay, engineering), which the module remaps to the readout. They are the **read**, and they are
  **not marked**, because making them brush entities is what the next section measures as fatal.

**A correction, as the brief requires when the code differs from its own paragraph:** the brief said
"on the generated decks the status screen is a plain brush remapped by a `target_shaderremap`
entity". The code says otherwise. The generated decks' screens are plain brushes that *name*
`textures/lwh/panel` directly (`tools/shipmap/dressdeck*.py`, `gendeck.py`); the `target_shaderremap`
on those decks maps `textures/lwh/panel` to the **Borg** shader (S8's deck-take), not to the readout.
The code's answer is the one the list above uses.

## Why the published decks' consoles are not marked (the stop condition, measured)

The brief's decision names the console shaders, so the first attempt was to mark every brush
carrying any list shader -- the literal reading. **It leaks the map.** Converting the panel-carrying
worldspawn brushes to brush entities removes sealing geometry from the structural BSP, and q3map2
says so:

```
******* leaked *******
Entity 1502, Brush 0: Entity leaked
```

That was with all 98 panel-carrying brushes; the same with the 63 whole-screen ones. Only the
dedicated readout screens convert cleanly:

```
MODE lwh marked 8
lwh exit=0 areaportalWarn=0
writing /tmp/qq_lwh/maps/vm.prt
```

So the affordance is placed on the readout brushes -- the *screens* the owner's decision names --
and the console-shader brushes stay world geometry and stay the read. This is a judgement call
(below), not a silent narrowing: `markpanels.py` names the 90 every build.

## The affordance, placed where the deck is authored

The screens are marked at their source, by the tools that author them, not in the built map:

* `tools/shipmap/dressdeck06/07/12/13/14.py` -- each status screen and station panel is emitted as a
  named `func_static` brush entity (`lwh_panel_<deck>`, `lwh_panel_<deck>_station`), the module's
  `LWH_PanelTag` prefix, instead of a worldspawn brush.
* `tools/shipmap/gendeck.py` -- the placeholder decks' screen likewise.
* `tools/shipmap/markpanels.py`, called by `scripts/build-ship.sh` after the stitcher, enumerates the
  panels from the module's list and marks any readout brush the sources left in worldspawn (a no-op
  today; the ship map is authoritative).

The module's `LWH_Panel_Frame` and the tooling both read the same list, so adding a panel moves the
remap, the marking and the check together. `markpanels.py --require-marked` fails if a readout brush
is left in worldspace, and `scripts/panel-control-check.sh` runs it before the engine test.

## The control, demonstrated in the world

`scripts/panel-control-check.sh`, one headless run (`g_shipTest 104`) on the merged ship. The harness
stands a facing-pace from a status screen, clears the script's own movement lock (the **test's** job:
a hand at a panel is not locked), and forges the use the way the engine's own use runs -- the recon's
`ClientThink` recipe (`ps.commandTime = level.time - 100; ucmd = pers.lastCommand;
ucmd.buttons |= BUTTON_USE; ClientThink(0,&ucmd)`), which reaches `PM_Use`/`TryUse`:

```
    panel test: standing at lwh_panel_06 (panel)
    panel test: the use is made by Kathryn Janeway
    panel test: forged at lwh_panel_06 (panel); condition 0 -> 1
    panel test: standing at lwh_panel_06 (the refusal)
    panel test: the refusal is tried by Harry Kim
    panel test: refusal "the condition is not yours to call: a department head at Engineering or Tactical, or command, calls it"; condition 1 -> 1
    panel test: standing at the plain brush (lwh/env) at (-3766 -3776 -37670)
    panel test: the plain brush left the condition at 1 (before 1)
    panel test: the map carries lwh_panel_06_station,lwh_panel_06,lwh_panel_07_station,lwh_panel_07,lwh_panel_12,lwh_panel_13_station,lwh_panel_13,lwh_panel_14_station,lwh_panel_14
```

The module's own probe line (the trace the module makes, the same one `TryUse` makes) is the
mechanism, visible in the transcript:

```
SHIP: panel probe: ut 2000 last 0 hit 1123 traceEnt 1123 frac 0.73 eye (-3556 -3862 -19245)
```

`ut 2000` is `PM_Use` firing; `hit 1123` is the `lwh_panel_06` brush entity the trace found;
`PanelUse` then calls `ship::SetAlert` (the model call), which moves the condition.

**The readout changes where the control is** -- read from the panel's own surface, not the code that
draws it. The same run photographs the screen before and after, and the readout's own region
(x 440..900, y 540..660 at 1280x1024) is legible in both and differs:

```
lwh_panel_before.tga                           panel           mean  72.0 median   0.0 p90 208.0 spread 208.0  LEGIBLE
lwh_panel_after.tga                            panel           mean  78.9 median   0.0 p90 253.0 spread 255.0  LEGIBLE
lwh_panel_after.tga                            panel           changed  19.1% mean|d|  32.8 of 55200 px  DIFFERS
```

The before frame reads `CONDITION GREEN` on a green band; the after reads `CONDITION YELLOW` on a
yellow band. The region is the panel's: the HUD and the weapon sit outside it.

**The refusal**, by name, with the panel unchanged: Harry Kim (rank below lieutenant) forges the
same use at the same screen, gets `lwh_panel_refusal` and the condition does not move (1 -> 1).

## The falsification, and it is required

The same forge at a brush that carries no panel shader -- the deck-12 environmental readout
(`lwh/env`) -- leaves the condition unchanged (1 -> 1), and the module's probe shows the trace
reaching the **world** (`traceEnt 8190`), so the null is a real surface, not empty air:

```
SHIP: panel probe: ut 2000 last 0 hit -1 traceEnt 8190 frac 0.62 eye (-3766 -3776 -37642)
```

And the check is shown able to fail: run with `LWH_PANEL_BREAK=1`, which passes `g_shipPanelBreak 1`
and aims the **primary** forge at that plain brush, so the "the condition changed" assertion has
nothing to hold it up:

```
    panel test: forged at lwh_env_screen (plain, no panel shader); condition 0 -> 0
    panel test: the planted break forged at a brush with no panel shader only
FAIL  with the break the primary forge was made at a brush with no panel shader and the
      condition did not change -- the check caught the planted break, as it must
```

## What was run

```
scripts/build-ship.sh                       # the marked screens; no leak, pk3 written
scripts/panel-control-check.sh              # PASS  (19.8 s)
LWH_PANEL_BREAK=1 scripts/panel-control-check.sh   # FAIL at the changed-condition assertion, as it must
scripts/s4-panel-check.sh                   # PASS  (the existing painted-panel check still holds)
scripts/shaderremap-check.sh                # PASS  (the Borg remap still spawns and toggles)
scripts/deck12-readout-check.sh             # PASS  (the env readout, untouched)
scripts/test.sh                             # all checks passed
```

The built module carries the new symbols (`nm -C libefgame.so`): `LWH_PanelShaderCount()`,
`LWH_PanelShader(int)`, `LWH_PanelReadoutShader()`, `LWH_PanelTag()`, and `PanelUse`'s word table --
asserted in the artifact, not the tree.

## What could not be verified, and what is deliberately left

* **A logged-in session walking to each station** -- S4's exit criterion, which no headless run
  substitutes for. The control is proven by the forged use; a person's hand is not.
* **The doors (G20's second half)** -- the sealed-quarters world prompt. Registered as O21; not this
  pass.
* **No engine patch.** The recon proved none is needed, and none was made: the module reads the use
  button's own edge and traces as `TryUse` does.
* **The published decks' console shaders are not the affordance.** They are the read; converting them
  leaks the map (measured above). The screens the decision names are.

## Judgement calls, named as calls

1. **The affordance is the readout screens, not every surface the remap paints.** The brief's
   decision is "mark the screens", and the screens are the `lwh/panel` brushes. The literal reading
   (every list shader, including the console shaders the module remaps) makes the map leak -- 90 of
   98 candidate brushes are structural -- so the two are not the same set, and the safe, intended one
   is the screens. `markpanels.py` reports the 90 it does not mark, so the narrowing is explicit.
2. **The control cycles the condition** (green -> yellow -> red -> green, a stand-down). The brief
   says the act is a model call under `ship::MayCallAlert` authority and that the panel draws
   `Condition` and the alert band; a single rotary condition control is the smallest act that changes
   exactly the readout the panel carries. The act is `ship::SetAlert`, the same the console calls.
3. **The standpoint aims at the face low, not its centre.** The world's use reaches 64 units
   (`g_utils.cpp` `USE_DISTANCE`) and a screen is 128 tall; aiming at the centre leaves the face out
   of reach. This is the harness's choice, and it is why the check uses a forged use rather than a
   distance heuristic.
4. **The lock-clearing is the test's, not the game's.** `player_locked` (a `tour/deck04` script) is
   zeroed by the harness. A hand at a panel is not locked; said plainly so the green line does not
   read as "the player could always use it".
