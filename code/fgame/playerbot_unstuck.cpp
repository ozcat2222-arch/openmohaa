/*
===========================================================================
Copyright (C) 2024 the OpenMoHAA team

This file is part of OpenMoHAA source code.

Fork-only helpers for bot unstuck / wander. Not for upstream PR.
===========================================================================
*/

#include "playerbot.h"

void BotMovement::TickUnstuck()
{
    if (!controlledEntity) {
        return;
    }

    if (m_iNumBlocks >= 3) {
        RecoverFromStuck();
        return;
    }

    if (!IsMoving() || controlledEntity->GetLadder()) {
        return;
    }

    if (controlledEntity->GetMoveResult() >= MOVERESULT_BLOCKED
        || controlledEntity->velocity.lengthSquared() <= Square(8)) {
        if (m_iNumBlocks < 3) {
            m_iNumBlocks++;
        }
        if (m_iNumBlocks >= 3) {
            RecoverFromStuck();
        }
    }
}

void BotMovement::WanderNearby()
{
    Vector dir;
    int    i;

    if (!controlledEntity) {
        return;
    }

    if (!m_pPath) {
        m_pPath = IPather::CreatePather();
    }

    for (i = 0; i < 8; i++) {
        dir = Vector(G_CRandom(384), G_CRandom(384), 0);
        if (dir.lengthSquared() < Square(64)) {
            continue;
        }

        Vector dest = controlledEntity->origin + dir;
        if (m_vLastFailedGoal != vec_zero && (dest - m_vLastFailedGoal).lengthXYSquared() < Square(128)) {
            continue;
        }

        if (CanMoveTo(dest)) {
            MoveTo(dest);
            return;
        }
    }

    AvoidPath(controlledEntity->origin, 256 + G_Random(256), Vector(G_CRandom(1), G_CRandom(1), 0));
}

void BotMovement::RecoverFromStuck()
{
    if (!controlledEntity) {
        ClearMove();
        return;
    }

    m_vLastFailedGoal = m_vTargetPos;
    m_pPrimaryAttract = NULL;
    m_iNumBlocks      = 0;
    m_iTempAwayState  = 0;
    m_iStuckUntilTime = level.inttime + 1500;

    if (m_pPath) {
        m_pPath->Clear();
    }

    WanderNearby();
}
