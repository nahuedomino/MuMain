#pragma once

#include "Core/Globals/_enum.h"

namespace GameLogic::Party
{
// Whether a hero of this base class casts this skill on a party member chosen through the member's party card.
[[nodiscard]] bool IsPartyMemberSkill(CLASS_TYPE heroBaseClass, ActionSkillType skill);

// What the hero has selected when casting Damage Reflection (Reflect).
struct ReflectSelection
{
    int index = -1;            // the selected character, -1 for none
    bool isLivePlayer = false; // a player who is not dead
    bool isHero = false;
    bool isPartyMember = false;
};

// The character Damage Reflection goes on: the selected player when it is the hero or one of the hero's party
// members, otherwise -1, which casts it on the hero (no selection, a monster, a dead player or a stranger). The
// server only accepts the Summoner and their party (TargetRestriction Party).
[[nodiscard]] int ReflectTarget(const ReflectSelection& selection);
} // namespace GameLogic::Party
