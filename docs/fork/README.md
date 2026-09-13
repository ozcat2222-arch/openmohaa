# Fork bot work (`experiment/bots`)

Stock `playerbot_movement.cpp` is unchanged.

Unstuck is wired from `BotController::UpdateBotStates` via `movement.TickUnstuck()` after `MoveThink`.
Idle bots call `WanderNearby()` instead of a 2k-unit AvoidPath dart.

Do not use branch `experiment/bot-unstuck`.
