#include "stdafx.h"
#include "doctest.h"
#include "GameLogic/Combat/ServerReposition.h"
#include "Engine/Object/ZzzCharacter.h"

using GameLogic::Combat::KeepsActionOnReposition;
using GameLogic::Combat::KeepsAttackingDuringSlide;

namespace
{
void SetUp(CHARACTER& c, int type, int action)
{
    c.Object.Type = type;
    c.Object.CurrentAction = static_cast<unsigned short>(action);
    c.Dead = 0;
}
} // namespace

TEST_CASE("A knockback keeps a player's attack swing [combat][reposition]")
{
    CHARACTER c;
    SetUp(c, MODEL_PLAYER, PLAYER_ATTACK_FIST);
    CHECK(KeepsActionOnReposition(&c, false));
}

TEST_CASE("A knockback keeps a skill cast outside the classic attack range [combat][reposition]")
{
    CHARACTER summoner;
    SetUp(summoner, MODEL_PLAYER, PLAYER_SKILL_SUMMON);
    CHECK(KeepsActionOnReposition(&summoner, false));
    CHARACTER evilSpirit;
    SetUp(evilSpirit, MODEL_PLAYER, PLAYER_SKILL_HELL);
    CHECK(KeepsActionOnReposition(&evilSpirit, false));
}

TEST_CASE("A knockback stops a walk, a stand, a sit and a dead character [combat][reposition]")
{
    CHARACTER walking;
    SetUp(walking, MODEL_PLAYER, PLAYER_ATTACK_FIST);
    CHECK_FALSE(KeepsActionOnReposition(&walking, true));

    CHARACTER standing;
    SetUp(standing, MODEL_PLAYER, PLAYER_STOP_MALE);
    CHECK_FALSE(KeepsActionOnReposition(&standing, false));

    CHARACTER sitting;
    SetUp(sitting, MODEL_PLAYER, PLAYER_SIT1);
    CHECK_FALSE(KeepsActionOnReposition(&sitting, false));

    CHARACTER dead;
    SetUp(dead, MODEL_PLAYER, PLAYER_ATTACK_FIST);
    dead.Dead = 1;
    CHECK_FALSE(KeepsActionOnReposition(&dead, false));
}

TEST_CASE("A pushed monster keeps its attack but not its idle pose [combat][reposition]")
{
    CHARACTER attacking;
    SetUp(attacking, MODEL_PLAYER + 1, MONSTER01_ATTACK1);
    CHECK(KeepsActionOnReposition(&attacking, false));
    CHARACTER idle;
    SetUp(idle, MODEL_PLAYER + 1, MONSTER01_STOP1);
    CHECK_FALSE(KeepsActionOnReposition(&idle, false));
}

TEST_CASE("The hero keeps attacking through a slide with a target or a held skill [combat][reposition]")
{
    CHECK(KeepsAttackingDuringSlide(true, false));
    CHECK(KeepsAttackingDuringSlide(false, true));
    CHECK(KeepsAttackingDuringSlide(true, true));
    CHECK_FALSE(KeepsAttackingDuringSlide(false, false));
}
