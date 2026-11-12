#include "stdafx.h"
#include "GameLogic/Combat/ServerReposition.h"
#include "GameLogic/Combat/SkillExecution.h"
#include "Engine/Object/PlayerActionState.h"
#include "Engine/Object/ZzzCharacter.h"

namespace GameLogic::Combat
{
bool KeepsActionOnReposition(const CHARACTER* c, bool wasWalking)
{
    if (wasWalking || c->Dead > 0)
    {
        return false;
    }

    if (Engine::Object::IsSitOrPoseAction(c->Object.CurrentAction))
    {
        return false;
    }

    return !IsSkillActionReady(c);
}

bool KeepsAttackingDuringSlide(bool hasAutoAttackTarget, bool isRightButtonHeld)
{
    return hasAutoAttackTarget || isRightButtonHeld;
}
} // namespace GameLogic::Combat
