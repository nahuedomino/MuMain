#include "stdafx.h"
#include "doctest.h"
#include "Core/Input/KeyState.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInterface.h"
#include "GameLogic/Combat/CombatTarget.h"
#include "Guild/GuildTypes.h"
#include "World/MapInfra/MapManager.h"

#include <SDL3/SDL.h>
#include <memory>

namespace
{
struct AreaTargetFixture
{
    CHARACTER hero;
    std::unique_ptr<CHARACTER[]> characters = std::make_unique<CHARACTER[]>(MAX_CHARACTERS_CLIENT);
    CHARACTER* previousHero = Hero;
    CHARACTER* previousCharacters = CharactersClient;
    int previousSelection = SelectedCharacter;
    int previousWorld = gMapManager.WorldActive;
    SDL_Keymod previousModifiers = SDL_GetModState();

    AreaTargetFixture()
    {
        Hero = &hero;
        CharactersClient = characters.get();
        gMapManager.WorldActive = WD_0LORENCIA;
        SelectedCharacter = 0;
        hero.Key = 512;
        auto& target = characters[0];
        target.Key = 123;
        target.PK = PVP_NEUTRAL;
        target.Object.Live = true;
        target.Object.Kind = KIND_PLAYER;
        SDL_SetModState(SDL_KMOD_NONE);
    }

    ~AreaTargetFixture()
    {
        SDL_SetModState(previousModifiers);
        Hero = previousHero;
        CharactersClient = previousCharacters;
        SelectedCharacter = previousSelection;
        gMapManager.WorldActive = previousWorld;
    }
};
}

TEST_CASE("Neutral AoE targets require Ctrl at the outgoing cast boundary [skills][area-pvp]")
{
    AreaTargetFixture fixture;
    CHECK_FALSE(Core::Input::IsKeyDown(VK_CONTROL));
    CHECK_FALSE(CheckAttack());
    CHECK(GameLogic::Combat::GetAimedAttackTargetKey(Hero) == 0xffff);
    // Legacy target lookup alone is not permission to initiate PvP.
    REQUIRE(getTargetCharacterKey(Hero, 0) == 123);
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 0xffff);
    SDL_SetModState(SDL_KMOD_LCTRL);
    CHECK(CheckAttack());
    CHECK(GameLogic::Combat::GetAimedAttackTargetKey(Hero) == 123);
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 123);
    SDL_SetModState(SDL_KMOD_RCTRL);
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 123);
    SDL_SetModState(SDL_KMOD_NONE);
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 0xffff);
}

TEST_CASE("Saved AoE targets are checked independently of the current selection [skills][area-pvp]")
{
    AreaTargetFixture fixture;
    auto& monster = fixture.characters[1];
    monster.Key = 124;
    monster.Object.Live = true;
    monster.Object.Kind = KIND_MONSTER;
    SelectedCharacter = 1;
    REQUIRE(CheckAttack());
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 0xffff);
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(124) == 124);
    SelectedCharacter = -1;
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 0xffff);
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(124) == 124);
    SDL_SetModState(SDL_KMOD_LCTRL);
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 123);
}

TEST_CASE("An outlaw caster still needs Ctrl to initiate AoE PvP against a neutral player [skills][area-pvp]")
{
    AreaTargetFixture fixture;
    for (const int casterState : {PVP_NEUTRAL, PVP_MURDERER1, PVP_MURDERER2})
    {
        CAPTURE(casterState);
        Hero->PK = casterState;
        SDL_SetModState(SDL_KMOD_NONE);
        CHECK_FALSE(CheckAttack());
        CHECK(GameLogic::Combat::GetAimedAttackTargetKey(Hero) == 0xffff);
        CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 0xffff);
        SDL_SetModState(SDL_KMOD_LCTRL);
        CHECK(CheckAttack());
        CHECK(GameLogic::Combat::GetAimedAttackTargetKey(Hero) == 123);
        CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 123);
    }
}

TEST_CASE("AoE target validation requires Ctrl for hostile players but preserves monsters and events [skills][area-pvp]")
{
    AreaTargetFixture fixture;
    auto& target = fixture.characters[0];
    target.Object.Kind = KIND_MONSTER;
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 123);
    target.Object.Kind = KIND_PLAYER;
    target.PK = PVP_MURDERER2;
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 0xffff);
    SDL_SetModState(SDL_KMOD_LCTRL);
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 123);
    SDL_SetModState(SDL_KMOD_NONE);
    target.PK = PVP_NEUTRAL;
    target.GuildRelationShip = GR_RIVAL;
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 0xffff);
    SDL_SetModState(SDL_KMOD_LCTRL);
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 123);
    SDL_SetModState(SDL_KMOD_NONE);
    target.GuildRelationShip = GR_NONE;
    gMapManager.WorldActive = WD_18CHAOS_CASTLE;
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 123);
}

TEST_CASE("Every ordinary AoE player cast needs Ctrl regardless of either players karma [skills][area-pvp]")
{
    AreaTargetFixture fixture;
    for (const int casterState : {PVP_NEUTRAL, PVP_MURDERER1, PVP_MURDERER2})
    {
        for (const int targetState : {PVP_NEUTRAL, PVP_MURDERER1, PVP_MURDERER2})
        {
            CAPTURE(casterState);
            CAPTURE(targetState);
            Hero->PK = casterState;
            fixture.characters[0].PK = targetState;
            SDL_SetModState(SDL_KMOD_NONE);
            CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 0xffff);
            SDL_SetModState(SDL_KMOD_LCTRL);
            CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 123);
            SDL_SetModState(SDL_KMOD_NONE);
            CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 0xffff);
        }
    }
}

TEST_CASE("Invalid removed dead and self AoE targets cannot authorize an opener [skills][area-pvp]")
{
    AreaTargetFixture fixture;
    SDL_SetModState(SDL_KMOD_LCTRL);
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(-1) == 0xffff);
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(0xffff) == 0xffff);
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(10000) == 0xffff);
    fixture.characters[0].Dead = 1;
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 0xffff);
    fixture.characters[0].Dead = 0;
    fixture.characters[0].Object.Live = false;
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 0xffff);
    fixture.characters[0].Object.Live = true;
    Hero = &fixture.characters[0];
    CHECK(GameLogic::Combat::ValidateAreaAttackTargetKey(123) == 0xffff);
}
