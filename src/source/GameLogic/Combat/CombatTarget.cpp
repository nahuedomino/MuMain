#include "stdafx.h"
#include "GameLogic/Combat/CombatTarget.h"
#include "GameLogic/Combat/SkillAim.h"
#include "Core/Input/KeyState.h"
#include "GameLogic/Combat/DuelMgr.h"

// Includes mirror ZzzInterface.cpp, the unit this was extracted from.
#include "Core/Platform/Imm.h"
#include "UI/Core/UIManager.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Render/Models/ZzzBMD.h"
#include "Render/Terrain/ZzzLodTerrain.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Engine/Object/ZzzObject.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/PlayerActionState.h"
#include "Render/Textures/ZzzTexture.h"
#include "Engine/AI/ZzzAI.h"
#include "Engine/Object/ZzzInterface.h"
#include "UI/Chat/ChatInput.h"
#include "GameLogic/Combat/ClassAttack.h"
#include "GameLogic/Combat/SkillCast.h"
#include "GameLogic/Combat/SkillExecution.h"
#include "Core/Input/ImeInput.h"
#include "Engine/Object/ZzzInventory.h"
#include "Engine/Object/ZzzOpenData.h"
#include "Render/Effects/ZzzEffect.h"
#include "Scenes/SceneCore.h"
#include "Engine/Pathing/ZzzPath.h"
#include "Audio/DSPlaySound.h"
#include "I18N/All.h"
#include "GameLogic/Events/MatchEvent.h"
#include "GameLogic/Items/PersonalShopTitleImp.h"
#include "GameLogic/Quests/CSQuest.h"
#include "GameLogic/Items/CSItemOption.h"
#include "GameLogic/NPCs/npcBreeder.h"
#include "GameLogic/Pets/GIPetManager.h"
#include "Character/CSParts.h"
#include "UI/HUD/UIMapName.h"	// rozy
#include "GameLogic/Events/Cinematic/CDirection.h"
#include "World/MapInfra/MapManager.h"
#include "GameLogic/Events/Event.h"
#include "UI/Core/WindowSystem.h"
#include "GameLogic/Events/w_CursedTemple.h"
#include "GameLogic/Social/PartyManager.h"
#include "UI/Dialogs/CommonMessageBox.h"
#include "GameLogic/Skills/SummonSystem.h"
#include "GameLogic/Skills/SkillManager.h"
#include "World/MapInfra/w_MapHeaders.h"

extern int SelectedCharacter, TargetX, TargetY;
extern bool SelectFlag;

namespace GameLogic::Combat
{
int ValidateAreaAttackTargetKey(int targetKey)
{
    if (targetKey < 0 || targetKey == 0xffff)
        return 0xffff;
    const int index = FindCharacterIndex(targetKey);
    if (index < 0 || index >= MAX_CHARACTERS_CLIENT)
        return 0xffff;
    const CHARACTER* target = &CharactersClient[index];
    if (target == Hero || !target->Object.Live || !CanAttackCharacter(index))
        return 0xffff;
    if (target->Object.Kind == KIND_PLAYER && !Core::Input::IsKeyDown(VK_CONTROL)
        && !g_DuelMgr.IsDuelEnabled() && !gMapManager.InChaosCastle()
        && !battleCastle::IsBattleCastleStart() && !IsStrifeMap(gMapManager.WorldActive)
        && !gMapManager.IsCursedTemple() && !EnableGuildWar)
        return 0xffff;
    return targetKey;
}

int GetAimedAttackTargetKey(CHARACTER* c)
{
    return CheckAttack() ? getTargetCharacterKey(c, SelectedCharacter) : 0xffff;
}

bool CheckTarget(CHARACTER* c)
{
    if (SelectedCharacter >= 0 && SelectedCharacter < MAX_CHARACTERS_CLIENT)
    {
        TargetX = (int)(CharactersClient[SelectedCharacter].Object.Position[0] / TERRAIN_SCALE);
        TargetY = (int)(CharactersClient[SelectedCharacter].Object.Position[1] / TERRAIN_SCALE);
        VectorCopy(CharactersClient[SelectedCharacter].Object.Position, c->TargetPosition);
        return true;
    }
    else
    {
        RenderTerrain(true);
        if (SelectFlag)
        {
            VectorCopy(CollisionPosition, c->TargetPosition);
            if (c == Hero)
            {
                const auto skill = CharacterAttribute->Skill[Hero->CurrentSkill];
                if (SkillAim::IsAimedByDirection(skill))
                {
                    SkillAim::PullIntoRange(c->Object.Position, c->TargetPosition,
                                            gSkillManager.GetSkillDistance(skill, c), TERRAIN_SCALE);
                }
            }
            TargetX = (int)(c->TargetPosition[0] / TERRAIN_SCALE);
            TargetY = (int)(c->TargetPosition[1] / TERRAIN_SCALE);
            return true;
        }
    }
    return false;
}
}
