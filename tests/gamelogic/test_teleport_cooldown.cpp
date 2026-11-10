#include "stdafx.h"
#include "doctest.h"
#include "GameLogic/Skills/SkillManager.h"
#include "GameLogic/Combat/ClassAttack.h"
#include "Engine/Object/ZzzCharacter.h"
#include "UI/HUD/SkillList.h"
#include <array>

using GameLogic::Skills::TeleportCooldown;
using namespace std::chrono_literals;

TEST_CASE("Held teleport input sends one request until the server responds [skills][teleport]")
{
    TeleportCooldown cooldown;
    const auto now = TeleportCooldown::Clock::time_point{};
    CHECK(cooldown.Request(now));
    for (int frame = 1; frame < 60; ++frame)
        CHECK_FALSE(cooldown.Request(now + frame * 10ms));
    CHECK(cooldown.Begin(now + 600ms));
    CHECK_FALSE(cooldown.Begin(now + 610ms));
    CHECK_FALSE(cooldown.Request(now + 1800ms));
    CHECK(cooldown.Land(now + 1800ms));
    CHECK_FALSE(cooldown.Begin(now + 1900ms));
    CHECK_FALSE(cooldown.Request(now + 3799ms));
    CHECK(cooldown.Request(now + 3800ms));
}

TEST_CASE("Rejected teleport corrections never animate or start recovery [skills][teleport]")
{
    TeleportCooldown cooldown;
    const auto now = TeleportCooldown::Clock::time_point{};
    CHECK(cooldown.Request(now));
    CHECK_FALSE(cooldown.Land(now + 20ms));
    CHECK(cooldown.IsReady(now + 20ms));
    CHECK(cooldown.Fraction(now + 20ms) == 0.f);
    CHECK(cooldown.Begin());
    CHECK(cooldown.Land(now + 1800ms));
    CHECK_FALSE(cooldown.Land(now + 1900ms));
    CHECK(cooldown.IsReady(now + 3800ms));
}

TEST_CASE("Unanswered teleport requests expire and reset without an animation [skills][teleport]")
{
    TeleportCooldown cooldown;
    const auto now = TeleportCooldown::Clock::time_point{};
    CHECK(cooldown.Request(now));
    CHECK_FALSE(cooldown.IsReady(now + 1999ms));
    CHECK_FALSE(cooldown.RequestTimedOut(now + 1999ms));
    CHECK(cooldown.IsReady(now + 2000ms));
    CHECK(cooldown.RequestTimedOut(now + 2000ms));
    cooldown.Reset();
    CHECK(cooldown.IsReady(now));
    CHECK_FALSE(cooldown.Land(now));
}

TEST_CASE("Teleport recovery begins on confirmed landing and lasts exactly two seconds [skills][teleport]")
{
    TeleportCooldown cooldown;
    const auto now = TeleportCooldown::Clock::time_point{};
    CHECK(cooldown.IsReady(now));
    CHECK(cooldown.Fraction(now) == 0.f);
    cooldown.Begin();
    CHECK_FALSE(cooldown.IsReady(now + 10s));
    CHECK(cooldown.Fraction(now + 10s) == 1.f);
    cooldown.Land(now + 1800ms);
    CHECK_FALSE(cooldown.IsReady(now + 3799ms));
    CHECK(cooldown.IsReady(now + 3800ms));
    CHECK(cooldown.Fraction(now + 2800ms) == doctest::Approx(.5f));
    CHECK(cooldown.Fraction(now + 3800ms) == 0.f);
}

TEST_CASE("Teleport state resets on session changes and ignores unrelated warps [skills][teleport]")
{
    TeleportCooldown cooldown;
    const auto now = TeleportCooldown::Clock::time_point{};
    cooldown.Land(now);
    CHECK(cooldown.IsReady(now));
    cooldown.Begin();
    cooldown.Reset();
    CHECK(cooldown.IsReady(now));
    CHECK(cooldown.Fraction(now) == 0.f);
    cooldown.Begin();
    cooldown.Land(now);
    cooldown.Reset();
    CHECK(cooldown.IsReady(now));
}

TEST_CASE("Teleport cast gate and skill bar use the same cooldown without blocking other spells [skills][teleport]")
{
    CHARACTER hero;
    CHARACTER_MACHINE machine{};
    std::array<SKILL_ATTRIBUTE, MAX_SKILLS> attributes{};
    const auto previousHero = Hero;
    const auto previousCharacter = CharacterAttribute;
    const auto previousAttributes = SkillAttribute;
    Hero = &hero;
    CharacterAttribute = &machine.Character;
    SkillAttribute = attributes.data();
    CharacterAttribute->Skill[0] = AT_SKILL_TELEPORT;
    Hero->CurrentSkill = 0;
    CSkillManager manager;
    CHECK(manager.CheckSkillDelay(0));
    manager.BeginHeroTeleport();
    CHECK_FALSE(manager.CheckSkillDelay(0));
    CHECK(manager.GetTeleportCooldownFraction() == 1.f);
    manager.CompleteHeroTeleport();
    CHECK_FALSE(manager.CheckSkillDelay(0));
    CHECK(manager.GetTeleportCooldownFraction() > .99f);
    manager.ResetHeroTeleport();
    CHECK(manager.CheckSkillDelay(0));

    gSkillManager.ResetHeroTeleport();
    gSkillManager.BeginHeroTeleport();
    mu::ui::window::CSkillList skills;
    CHECK(skills.GetCurrentSkillCooldownFraction() == 1.f);
    hero.Class = CLASS_WIZARD;
    hero.Object.CurrentAction = 0;
    GameLogic::Combat::AttackWizard(&hero, AT_SKILL_TELEPORT, 5.f);
    CHECK(hero.Object.CurrentAction == 0);
    gSkillManager.CompleteHeroTeleport();
    GameLogic::Combat::AttackWizard(&hero, AT_SKILL_TELEPORT, 5.f);
    CHECK(hero.Object.CurrentAction == 0);
    CHECK(skills.GetCurrentSkillCooldownFraction() > .99f);
    CharacterAttribute->Skill[0] = AT_SKILL_FIREBALL;
    SkillAttribute[AT_SKILL_FIREBALL].Delay = 0;
    CHECK(gSkillManager.CheckSkillDelay(0));
    gSkillManager.ResetHeroTeleport();
    Hero = previousHero;
    CharacterAttribute = previousCharacter;
    SkillAttribute = previousAttributes;
}
