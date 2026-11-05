#include "stdafx.h"
#include "doctest.h"

#include "Engine/AI/ZzzAI.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Network/Server/WSclient.h"
#include "Render/Models/ZzzBMD.h"
#include "World/MapInfra/MapManager.h"

#include <memory>
#include <span>

namespace
{
constexpr int ObservedKey = 2;
constexpr BYTE StartTile = 50;

// Another player in view of the hero, standing on (StartTile, StartTile).
struct ObservedPlayerFixture
{
    CHARACTER* previousCharacters = CharactersClient;
    CHARACTER* previousHero = Hero;
    BMD* previousModels = Models;
    int previousWorld = gMapManager.WorldActive;
    std::unique_ptr<CHARACTER[]> characters = std::make_unique<CHARACTER[]>(MAX_CHARACTERS_CLIENT);
    std::unique_ptr<BMD[]> models = std::make_unique<BMD[]>(MODEL_PLAYER + 1);
    CHARACTER& observed = characters[1];

    ObservedPlayerFixture()
    {
        gMapManager.WorldActive = WD_0LORENCIA;
        InitPath();
        Models = models.get();
        Models[MODEL_PLAYER].NumActions = MAX_PLAYER_ACTION;
        Models[MODEL_PLAYER].Actions = new Action_t[MAX_PLAYER_ACTION]{};
        CharactersClient = characters.get();
        Hero = &characters[0];
        Hero->Key = 1;
        Hero->Object.Live = true;

        observed.Key = ObservedKey;
        observed.Object.Live = true;
        observed.Object.Type = MODEL_PLAYER;
        observed.Object.Kind = KIND_PLAYER;
        observed.Helper.Type = -1;
        observed.Weapon[0].Type = -1;
        observed.Weapon[1].Type = -1;
        observed.PositionX = observed.TargetX = StartTile;
        observed.PositionY = observed.TargetY = StartTile;
        observed.Object.Position[0] = (StartTile + 0.5f) * TERRAIN_SCALE;
        observed.Object.Position[1] = (StartTile + 0.5f) * TERRAIN_SCALE;
    }

    ~ObservedPlayerFixture()
    {
        delete[] Models[MODEL_PLAYER].Actions;
        Models[MODEL_PLAYER].Actions = nullptr;
        Models = previousModels;
        Hero = previousHero;
        CharactersClient = previousCharacters;
        gMapManager.WorldActive = previousWorld;
    }

    void ReceiveInstantMove(BYTE x, BYTE y)
    {
        PRECEIVE_MOVE_POSITION packet{};
        packet.KeyL = ObservedKey;
        packet.PositionX = x;
        packet.PositionY = y;
        ReceiveMovePosition(reinterpret_cast<const BYTE*>(&packet));
    }

    void ReceiveWalk(BYTE sourceX, BYTE sourceY, BYTE targetX, BYTE targetY)
    {
        PMOVE_CHARACTER packet{};
        packet.KeyL = ObservedKey;
        packet.SourceX = sourceX;
        packet.SourceY = sourceY;
        packet.TargetX = targetX;
        packet.TargetY = targetY;
        ReceiveMoveCharacter(std::span(reinterpret_cast<const BYTE*>(&packet), sizeof(packet)));
    }
};
} // namespace

TEST_CASE("An instant move of another player glides it to the new tile [network][movement]")
{
    ObservedPlayerFixture fixture;
    fixture.ReceiveInstantMove(StartTile + 1, StartTile);

    CHECK(fixture.observed.JumpTime > 0);
    CHECK(fixture.observed.TargetX == StartTile + 1);
}

TEST_CASE("A walk right after an instant move walks instead of gliding to the walk target [network][movement]")
{
    ObservedPlayerFixture fixture;
    fixture.ReceiveInstantMove(StartTile + 1, StartTile);
    fixture.ReceiveWalk(StartTile + 1, StartTile, StartTile + 6, StartTile);

    CHECK(fixture.observed.JumpTime == 0);
    CHECK(fixture.observed.TargetX == StartTile + 6);
    // Not moved yet: the walk starts from where the character is drawn.
    CHECK(fixture.observed.Object.Position[0] == doctest::Approx((StartTile + 0.5f) * TERRAIN_SCALE));
}
