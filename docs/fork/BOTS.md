# Fork survey: multiplayer bots

This note is **fork-only**. It is not intended for an upstream pull request.
It maps how OpenMoHAA bots work today and where a private experiment should start.

Upstream already documents the player-facing limits:

- Bots navigate and attack; they do not complete objectives.
- Minefields are poorly detected and can fully block a path (Omaha Beach west Axis spawn is the canonical example).
- Dynamic obstacles (vehicles in the middle of a map) often stop Recast paths.
- There is no skill system yet; difficulty is a pile of independent cvars.

See `docs/markdown/03-configuration/01-configuration.md` and `docs/markdown/03-configuration/03-configuration-bots.md`.

## Layers

```
server / game frame
  G_BotFrame / G_BotThink          code/fgame/g_bot.cpp, g_bot.h
    BotManager::Frame              code/fgame/playerbot.h
      BotControllerManager
        BotController::Think
          CheckStates  (priority state machine)
          BotMovement::MoveThink   playerbot_movement.cpp
          BotRotation::TurnThink   playerbot_rotation.cpp
          CheckUse / CheckValidWeapon / CheckReload
```

Legacy Quake III `code/botlib/` is still in the tree. The live multiplayer bots do **not** use AAS/AI nodes from that library for decision making. Pathing is Recast (`IPather` / `navigation_path.h`) plus `AttractiveNode` waypoints from `navigate.h`.

`playerbot_strategy.h` / `playerbot_strategy.cpp` exist as a stub: `NavigationPoint` is an empty `SimpleEntity` with a `// TODO`.

## Spawn and identity

`g_bot.h` is the public spawn API:

- `G_AddBot` / `G_AddBots` / `G_RemoveBot`
- `G_IsBot` via `SVF_BOT`
- `G_GetBotSkill` exists, but docs say there is no skill system yet
- Caps: `sv_maxbots` plus the 64-client engine limit (`sv_maxclients + sv_maxbots` cannot exceed 64)

Bots are real `Player` entities driven by a `BotController`, not separate pawn classes.

## State machine

`MAX_BOT_FUNCTIONS` is 5. `BotController::Init` wires only four:

| Slot | State    | Role |
|------|----------|------|
| 0    | Attack   | Highest priority. Scan `SentientList`, pick a valid enemy, aim, burst-fire, close or kite. |
| 1    | Curious  | Move toward last noticed event position. |
| 2    | Grenade  | React to incoming grenades (avoid). |
| 3    | Idle     | Default roam: `MoveToBestAttractivePoint()`, else last death position or random wander. |
| 4    | Weapon   | Implemented but **commented out** (`//InitState_Weapon`). |

`CheckStates` runs conditions in slot order. Attack wins whenever an enemy is valid. Idle is the fallback. There is **no Objective, Capture, Plant, Use-target, or Defend state**.

Each think also:

- `CheckUse`: short traces for usable / ladder (`MASK_USABLE | MASK_LADDER`) and presses use if something is in front. It does not seek objectives across the map.
- `CheckWindows`: special-case for seeing / shooting through windows.
- `CheckValidWeapon` / `UseWeaponWithAmmo` / melee fallback.

Combat tuning cvars (already documented):

- `g_bot_attack_burst_*`, `g_bot_attack_continuousfire_*`
- `g_bot_attack_react_*`, `g_bot_attack_spreadmult`
- `g_bot_turn_speed`
- `g_bot_instamsg_chance` / `g_bot_instamsg_delay`
- `g_bot_initial_spawn_delay`
- `g_bot_manualmove` (debug / override)

## Movement

`BotMovement` (`playerbot.h` + `playerbot_movement.cpp`):

- `MoveTo` / `MoveNear` / `AvoidPath` / `MoveToBestAttractivePoint`
- Recast path (`IPather *m_pPath`)
- Attractive nodes with priority (`AttractiveNode::m_iPriority`)
- Block / unstuck counters (`m_iNumBlocks`, `m_iLastBlockTime`, `m_iTempAwayState`)
- Jump and edge-jump checks
- Short-range collision avoidance (`m_bAvoidCollision`)

This is why obstacles and minefields show up as “stuck in spawn”: the pather treats those areas as walkable or as a dead end, and Idle keeps asking for an attractive point on the other side.

## What is missing (ranked for this fork)

### 1. Objective play (largest gameplay gap)

Need a new state, probably slot 4 or a sixth function with `MAX_BOT_FUNCTIONS` bumped.

Sketch:

- If `g_gametype` is `GT_OBJECTIVE`, `GT_TOW`, or Liberation: find team-relevant objective entities (`Objective`, `TOWObjective`, use triggers, capturable points).
- Path to them with existing `MoveTo`.
- When within use range, rely on `CheckUse` or explicitly set `BUTTON_USE`.
- Attack still overrides when an enemy is visible.

Risk: objective entities are script-driven and map-specific. A first version should only chase named / classed entities that already exist in fgame, not invent new map data.

### 2. Unstuck / dynamic obstacles

`BotMovement` already counts blocks. Raise the usefulness of that:

- After N failed path polls, `ClearMove` and pick a different attractive node or a random nearby navpoly.
- Treat large movers (vehicles) as temporary avoidance volumes in `AvoidPath`.

Smaller than objectives, easier to test on any FFA map.

### 3. Minefields as bad areas

Breakthrough landmines / scripted mine volumes need to be injected into Recast as unwalkable or high-cost. Until that exists, a cheap heuristic: if a bot takes mine damage, mark that origin as a temporary avoid radius (`AvoidPath`).

### 4. Skill cvar

`G_GetBotSkill()` is a hook. Wire `g_bot_skill` 1–5 to scale spread, react delay, turn speed, and burst gaps. No new AI, only the existing cvars. Good first *code* change because it is isolated in `gamecvars.cpp` + attack state.

### 5. Enable or delete Weapon state

`InitState_Weapon` is dead. Either finish “seek ammo / better gun” or remove the slot so `MAX_BOT_FUNCTIONS` is honest.

### 6. Strategy file

Fill `NavigationPoint` or delete it. Do not grow a second unused abstraction next to `AttractiveNode`.

## Suggested experiment order on this fork

1. Skill cvar (safe, testable in FFA).
2. Unstuck timeout using existing block counters.
3. Mine-damage avoid radius.
4. Objective state behind `g_bot_do_objectives` (default 0) so stock roam is unchanged until enabled.
5. Only then touch Recast cost meshes.

Keep all of that on `experiment/*` branches. Do not open a PR against `openmoh/openmohaa`.

## How to test (after a local build)

```
set sv_maxclients 8
set sv_maxbots 8
set g_bot_initial_spawn_delay 2
map <mp map>
addbot   (or whatever console command G_AddBot is bound to)
```

Watch:

- FFA: roam + fight only.
- Objective maps: do they ever walk to the flagged object?
- Omaha west Axis spawn: do they leave the beach?
- Maps with parked vehicles: do they repath or freeze?

## Files to touch later

| File | Why |
|------|-----|
| `code/fgame/g_bot.cpp` / `g_bot.h` | spawn, skill string |
| `code/fgame/playerbot.h` | extra state, flags |
| `code/fgame/playerbot.cpp` | state machine |
| `code/fgame/playerbot_movement.cpp` | unstuck, avoid |
| `code/fgame/playerbot_strategy.*` | either implement or drop |
| `code/fgame/navigate.*` | attractive nodes |
| `code/fgame/gamecvars.cpp` | new cvars |
| Recast wrappers under `code/fgame/navigation_*` | mine / obstacle costs |
