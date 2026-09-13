# Fork bot work

Use branch `experiment/bots`.
Do not use `experiment/bot-unstuck` — `playerbot_movement.cpp` there is damaged.

This branch keeps stock `playerbot_movement.cpp`.

Added:
- `code/fgame/playerbot_unstuck.cpp`
- extra methods on `BotMovement` in `playerbot.h`

When you clone, wire unstuck with this edit in `MoveThink`:

```
if (m_iNumBlocks >= 3) {
    RecoverFromStuck();
    return;
}
```

in place of `m_iNumBlocks >= 5` / `ClearMove()`.

Also init in the BotMovement constructor:

```
m_iStuckUntilTime = 0;
m_vLastFailedGoal = vec_zero;
```
