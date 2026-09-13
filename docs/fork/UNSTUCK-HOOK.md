# Unstuck hook (apply in `playerbot_movement.cpp`)

Helpers are already on this branch:

- `code/fgame/playerbot_unstuck.cpp` — `WanderNearby`, `RecoverFromStuck`
- `code/fgame/playerbot.h` — declarations + `m_vLastFailedGoal` / `m_iStuckUntilTime`

CMake globs `fgame/*.cpp`, so the new file is compiled automatically.

## Required hook

In `BotMovement::MoveThink`, replace the give-up block:

```cpp
        if (m_iNumBlocks >= 5) {
            // Give up
            ClearMove();
        }
```

with:

```cpp
        if (m_iNumBlocks >= 3) {
            RecoverFromStuck();
            return;
        }
```

Optional: in the constructor, add:

```cpp
    m_iStuckUntilTime = 0;
    m_vLastFailedGoal = vec_zero;
```

Optional idle wander (`playerbot.cpp` `State_Idle`): if `!movement.IsMoving()` and no attractive point, call `movement.WanderNearby()` instead of `AvoidPath` with a 512+2048 radius (that huge radius is a common way bots walk into walls).

## What this does

After three blocked path checks (~3 seconds of no progress), drop the current attractive node and path to a nearby Recast-tested point instead of standing still. The last failed goal is skipped so they do not immediately retry the same dead destination.

This will not teach minefields or objectives. It only stops the freeze loop.
