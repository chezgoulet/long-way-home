# The client's three modes

Owner's framing, 2026-10-04, as a later roadmap goal. Recorded here because it changes the client's
*contract*: the original game is not a stepping stone toward our content, it is a mode we support
permanently, and our experiences ride alongside it.

| mode | what it is | state |
|---|---|---|
| **1. The original game and all downloadable content** | the retail single-player campaign, the Expansion Pack (Virtual Voyager decks, expansion maps), the 1.2 voice pack | **nearly there** — the SP path runs the campaign, saves and reloads; the expansion is installed; cutscenes now decode. Missing: polish, and the Virtual Voyager walk (G2) |
| **2. Original LAN multiplayer** | retail Holomatch, over a LAN or a VPN (Tailscale, Nebula, ZeroTier) | **closer than it looks** — see below |
| **3. Long Way Home** | single player (Track A/B/C) and multiplayer (Track D: one ship = one server) | the programme as chartered |

## Why this framing is better than the one it replaces

Earlier documents treated the retail game as the thing we port in order to reach our own content. Under
this framing the retail game is a supported mode in its own right, which:

- **makes the client worth using immediately**, rather than only after the capstone exists;
- **is the recruitment path for the MMO.** Track D's hardest problem is population, and a client people
  want for the original game is the top of that funnel. Modes 1 and 2 are not a detour from the MMO,
  they are how anyone ever hears about it;
- **states the compatibility promise explicitly**, which is a thing we would otherwise argue about later.

## Mode 2 is closer than it looks, and one session settles it

The evidence is an accident: the *first* playtest run — before our SP module was on the loader path —
was mode 2. The log shows Holomatch loading, `qagame.qvm` coming up from the paks, `BotLib` parsing
**69 bots**, `AAS initialized`, a map load, a running match, and a clean shutdown. The engine already
carries both paths; in SP mode it deliberately bypasses the HM one
(`EFSP Route-b: HM spmap/qagame/cgame NOT loaded; SP cgame is sole`).

**Tasks carried out of this analysis, both needing one session:**

- [ ] explain and close the control fault ("connecting to localhost", no avatar control) — the difference
      between Holomatch booting and Holomatch being playable
- [ ] decide: retail `qagame.qvm` through the interpreter (proven working) or a native module built from
      the released Holomatch source (faster, moddable, and the same job we already did for single-player)

Two open questions, both answerable in one session:

1. **The control fault.** That session showed "connecting to localhost" and no avatar control. Unexplained
   so far, and it is the difference between "Holomatch boots" and "Holomatch is playable".
2. **Which game module.** Today HM runs the retail `qagame.qvm` through the interpreter — proven working.
   The **Holomatch game source is released** (v1.1 / v1.2, same Raven licence we build under), so a
   native module is possible: faster than the interpreter and moddable, and it is how the same game code
   would be extended later. Building it is work we have already done once, for the single-player module.

## Tailscale, Nebula, ZeroTier — nearly free, and it is the point

The engine is a UDP client/server; a tailnet address is routable as-is, so **direct connect already works**.
No NAT traversal to write, no relay to run. Two deliberate choices:

- **document it as a supported path rather than build UI for it** — a launcher field for "connect to
  address" is the whole feature;
- note that **the freeware Holomatch base means other players do not need to own the game.** That is what
  makes playing with family actually happen, and it is why mode 2 costs so little.

The House already runs a tailnet, so a private server is a scripted launch away, not a project.

## The principle: inherit, do not own

Owner's direction, 2026-10-04: *"I'd rather be in the best position to inherit community work than
increase our support of more stuff."*

That is the deciding rule for every architectural choice on this roadmap, and it is better than the
engine-count trade-off I was weighing. We are one small group with a long programme; the community
stacks we build on are maintained by people who have been fixing them for twenty years. Our position
should be to **consume their maintenance, and to keep our own delta thin enough to rebase onto it
whenever they release.**

Applied concretely:

- **Mode 2 is cMod as shipped — zero delta from us.** Native on Linux, maintained, with the connect-path
  fixes our own tree never inherited (see the changelog notes below). We ship it, we do not fork it, and
  their next release is ours for free.
- **Modes 1 and 3 build on maintained upstreams too**: the SP engine is a thin patch series on the pinned
  upstream port, rebasable on every upstream release, and the modules are ours only because nobody else
  has written them.
- **Our inventions live behind the module boundary, not in the engine.** The autonomy layer, the ship
  server, the authored content — all of it belongs in a game module the engine loads, because that is
  what keeps the engine inheritable. Anything we put *into* the engine is a delta we own forever, and a
  rebase cost on every upstream release.
- **Where the engine must change, prefer contributing upstream** — Track D's engine-level needs (many
  clients, persistence, orchestration) are exactly the kind of change worth offering to the projects
  maintaining those engines, so the maintenance is shared rather than duplicated.

The honest cost: we give up the freedom to change multiplayer engine behaviour unilaterally, and upstream
acceptance is not guaranteed. That is a real constraint, and it is why the principle has a boundary — new
game logic is ours and lives in the module; engine internals are theirs and we ask first.

### Why this matters here specifically

cMod's changelog is twenty years of *connection-path* fixes our lilium-lineage tree never inherited:

- *"connection issues / userinfo — client side fix, instead of sending `connect <userinfo>` packet we now
  send `connect \"<userinfo>\"`"*
- *"backported from RTCW, don't get dropped if the server changes map while connecting (ignore outdated cp)"*
- *"backport fix to pk3 reordering… bad order from connection may break stuff"*
- *"extended the getIpAuthorize (server->auth message) syntax"*, plus a `com_errorMessage` UI so a kick or
  drop is *shown* rather than silently stalling
- `CL_InitDownloads` / `FS_ComparePaks` diagnostics naming the missing paks that caused a connection to
  fail — *"typically when the user is sent back to the main screen"*

A silent stall at "connecting to localhost" is precisely the pre-fix symptom in that list. Inheriting
those fixes costs nothing; re-deriving them costs everything they cost, again.

## The promise, stated precisely

"Full original" means **compatibility of gameplay and content** — retail maps, saves, configuration and
demos behave as they do in the retail game — not a byte-identical binary. cMod deliberately changes
rendering behaviour and limits, and we want those changes. Writing this down now prevents the argument
later about whether mode 1 is faithful enough.

## Roadmap consequence

Modes 1 and 2 go **before** G4 and G5: they are cheap, they make the client useful, and they are the
funnel. Mode 3's multiplayer is Track D, unchanged. Suggested gates:

- **G6 — retail single player**, including the Expansion Pack: the G2 acceptance bar, plus a save that
  survives a mode switch.
- **G7 — retail multiplayer**, LAN and over a VPN: a match between two machines, bots as a fallback
  population, and the control fault explained and closed.
