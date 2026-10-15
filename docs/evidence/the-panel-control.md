# Evidence: the in-world control at the panel (O10) — the premise, the probe, and what it needs

**Status: NOT BUILT.** This is the recon the item stopped on, landed so the next pass does not
re-derive it. The **mechanism is proven**; the **panel's identity is the blocker**, and it is a design
decision rather than an engineering one.

O10 (`docs/omissions.md`): *"The in-world control at the panel (the 'glance') is not built. The panel
carries the ship's live state as a **read**; the missing half is an interactive surface on the panel
brush."* The brief's acceptance: **the control is operated in the world, at the panel, and the panel's
own readout changes; a console command reaching the same state is not this item.** Closure, per
`docs/walkthrough.md` **G20**, is *"an engine/world affordance on panel and door brushes."*

**Host:** `sasquatch`, the engine host. **Nothing was committed for this item.** (The probe lived in
`module/ship/g_ship.cpp` as a temporary `g_shipTest 104` and was reverted.)

## What was verified: the use can be driven headlessly

The control is the world's **use** action (`BUTTON_USE`, `q_shared.h:1016`), which the engine turns
into `pm.useEvent` and consumes in `g_active.cpp`'s `ClientThink` (`TryUse( ent )`).

- **A console `+use` does not reach the game.** Sent from the console, the button never arrives —
  `client->buttons` stays **0** at every sample. The client's key state is not reachable from the
  game module or the console.
- **A forged usercmd does reach it.** The game can forge one, the way `g_client.cpp:910-911` runs a
  client frame, and the engine's **own** use path then runs:

  ```
  # forge: ps.commandTime = level.time - 100; ucmd = pers.lastCommand; ucmd.serverTime = level.time;
  #        ucmd.buttons |= BUTTON_USE; ClientThink( 0, &ucmd );
  SHIP: use probe: locked 0 buttons 32 useTime 2000 cmdTime 3000->3000 moved 0.0
  ```

  `buttons 32` is `BUTTON_USE`; `useTime 2000` is `PM_Use` (`bg_pmove.cpp`) firing `pm.useEvent`, so
  `TryUse` is called — **the world's use, not a console verb.**
- **`player_locked` was the guard.** On `tour/deck04` a script sets it, and `ClientThink_real`
  (`g_active.cpp:866`) zeroes `ucmd->buttons` while it is set — the first forge reached the game and
  was still `buttons 0`. Clearing it (the harness's job) is what makes the forge take:
  `locked 1 buttons 0 ...` → `locked 0 buttons 32 useTime 2000`.

**And the stock panels are inert to the player's use.** Using the panel `StandAtPanel` stands at
(`tour/deck04`, `(-2614 -3814 -3857)`) leaves it unchanged (`solid 16777215`, `nodraw 0`): a
`func_usable` is player-usable only with `SVF_PLAYER_USABLE` (`g_utils.cpp` `ValidUseTarget`), which
`SP_func_usable` does **not** set. **That inertness is exactly the gap O10 names** — and it means the
module can add the affordance with **no engine patch and no conflict** with the stock use.

## What is unresolved: which brush is "the panel"

The control has to be on the brush whose **readout** is the ship's live state. Two probes show the
candidate brushes do not line up:

- **`func_usable` on the ship map (`voyager`) is mostly doors and hatches, not panels.** 20+ of them,
  with spans `396×460×90`, `272×23×44`, `140×90×30`, `29×29×28`, `12×68×38`, `10×22×22`, and some
  `STARTOFF`. `StandAtPanel`'s own criteria — *a standing box, not a floor, with a standable side* —
  **a door satisfies**, so the existing glance/panel checks may already be standing at a door.
- **The live panel image is on none of them reliably.** `LWH_Panel_Frame` draws the state into
  `panel[]` and remaps three shader names to it (`textures/transporter/panel`,
  `textures/sickbay/panel12`, `textures/engineering/conpanel`, `patches/0014`). On the **generated
  decks** the status screen is a plain **brush** remapped by a **`target_shaderremap`** entity — a
  probe at `d06_arrival` (`voyager`) finds `info_notnull`, `target_shaderremap` and teleporters, and
  **no `func_usable`** at all.

So "the panel brush" is not one identifiable thing yet, and picking one (a size heuristic to tell a
panel from a door, or the `target_shaderremap` marker as a proxy) would be inventing a rule.

## What it needs

1. **A decision: which brushes are the panels.** Either mark the status-screen brushes as the
   affordance — a small change to the deck-redress tools or the ship map — or accept the stock
   `func_usable` panels and say plainly that a door is not a panel. `docs/walkthrough.md` G20 also
   notes that full closure is *"a logged-in session walking to each station"* (S4's exit criterion),
   which no headless run substitutes for.
2. **Then the module control** — proven cheap: with the brush known, the module reads the use
   (`client->buttons` edge, or `ps.useTime` rising), acts on the panel, and the act is a **model call**
   (`SetAlert`) with the stations' own authority (`ship::MayCallAlert` — a department head or above, or
   command). The panel already draws `Condition` and the alert band (`module/ship/lwh_panel.cpp`), so
   **the readout changes exactly where the control is.**
3. **The check**, with the harness forging the use (above) and clearing `player_locked`; a refusal for
   a person who may not call the alert (`lwh_panel_refusal`), and the panel unchanged for them.

## Judgement calls, named as calls

1. **I did not invent the panel's identity.** Three ways to force it were available — a size
   heuristic, the `target_shaderremap` marker as a proxy, or hijacking the `func_usable` brushes
   (which are doors) — and each would have built the item on a rule the design does not carry.
2. **The finding is landed rather than dropped**, because the probe is the expensive part and the
   next pass must not re-derive it (this is the same discipline Item 0b applied to the survey).
3. **The `player_locked` clearing is the test's, not the game's.** A run that presses use must clear
   the scripted lock; a *hand* at a panel is not locked. Said plainly so the check does not read as
   "the player could always use it".
