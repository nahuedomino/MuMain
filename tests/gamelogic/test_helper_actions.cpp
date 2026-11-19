#include "stdafx.h"
#include "doctest.h"

#include "Engine/AI/ZzzAI.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInventory.h"
#include "Engine/Object/ZzzObject.h"
#include "GameLogic/Automation/Pickup.h"
#include "GameLogic/Automation/Repair.h"
#include "GameLogic/Automation/Skill.h"
#include "GameLogic/Skills/SkillManager.h"
#include "MUHelper/MuHelper.h"
#include "Render/Terrain/ZzzLodTerrain.h"
#include "Scenes/SceneCore.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cwchar>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace
{
constexpr ActionSkillType Primary = static_cast<ActionSkillType>(41);
constexpr ActionSkillType Extra = static_cast<ActionSkillType>(42);
constexpr int HeroTile = 103;
constexpr int HeroRow = 100;
constexpr int DropSlot = MAX_ITEMS - 1;
constexpr int OtherDropSlot = MAX_ITEMS - 2;
constexpr auto HelperTick = std::chrono::milliseconds(250);
std::vector<std::pair<ActionSkillType, int>> submittedCasts;
std::map<ActionSkillType, GameLogic::Automation::SkillResult> castResults;
std::map<int, GameLogic::Automation::SkillResult> targetCastResults;
std::vector<int> pickupAttempts;
GameLogic::Automation::PickupResult pickupResult;

struct HelperActionFixture
{
    CHARACTER hero;
    CHARACTER_MACHINE machine{};
    std::array<SKILL_ATTRIBUTE, MAX_SKILLS> attributes{};
    std::unique_ptr<CHARACTER[]> characters = std::make_unique<CHARACTER[]>(MAX_CHARACTERS_CLIENT);
    CHARACTER* previousHero = Hero;
    CHARACTER* previousCharacters = CharactersClient;
    CHARACTER_MACHINE* previousMachine = CharacterMachine;
    CHARACTER_ATTRIBUTE* previousCharacter = CharacterAttribute;
    SKILL_ATTRIBUTE* previousAttributes = SkillAttribute;
    EGameScene previousScene = SceneFlag;
    std::chrono::steady_clock::time_point now;
    std::array<WORD, 9> previousWalls;
    std::array<bool, 2> previousDropLive = {Items[DropSlot].Object.Live, Items[OtherDropSlot].Object.Live};
    std::array<std::array<float, 3>, 2> previousDropPositions;

    HelperActionFixture()
    {
        submittedCasts.clear();
        castResults.clear();
        targetCastResults.clear();
        pickupAttempts.clear();
        pickupResult = GameLogic::Automation::PickupResult::NoPath;
        for (int drop = 0; drop < previousDropLive.size(); ++drop)
            std::copy_n(Items[DropSlot - drop].Object.Position, 3, previousDropPositions[drop].begin());
        InitializeCharacter();
        MUHelper::ConfigData config;
        config.aiSkill = {Primary, Extra, 0};
        config.iHuntingRange = config.iObtainingRange = 8;
        config.bPickAllItems = true;
        config.bReturnToOriginalPosition = true;
        MUHelper::g_MuHelper.Load(config);
        MUHelper::g_MuHelper.Start();
        now = std::chrono::steady_clock::now();
        hero.PositionX = HeroTile;
        hero.Object.Position[0] = (HeroTile + 0.5f) * TERRAIN_SCALE;
        hero.Object.Position[1] = (HeroRow + 0.5f) * TERRAIN_SCALE;
        auto& target = characters[0];
        target.Key = 123;
        target.Object.Live = true;
        target.Object.Kind = KIND_MONSTER;
        target.PositionX = target.TargetX = HeroTile;
        target.PositionY = target.TargetY = HeroRow;
        target.Object.Position[0] = hero.Object.Position[0];
        target.Object.Position[1] = hero.Object.Position[1];
        MUHelper::g_MuHelper.AddTarget(target.Key, false);
        PlaceDrop(DropSlot, HeroTile + 3);
        BlockReturnPath();
    }

    void InitializeCharacter()
    {
        Hero = &hero;
        SceneFlag = MAIN_SCENE;
        CharactersClient = characters.get();
        CharacterMachine = &machine;
        CharacterAttribute = &machine.Character;
        SkillAttribute = attributes.data();
        hero.Class = CLASS_KNIGHT;
        hero.Object.Type = MODEL_PLAYER;
        hero.Object.CurrentAction = PLAYER_STOP_MALE;
        hero.PositionX = HeroTile - 3;
        hero.PositionY = HeroRow;
        machine.Character.Mana = machine.Character.SkillMana = 100;
        machine.Character.Strength = 500;
        machine.Character.Level = 100;
        machine.Character.SkillNumber = 2;
        machine.Character.Skill[0] = Primary;
        machine.Character.Skill[1] = Extra;
        attributes[Primary].Distance = attributes[Extra].Distance = 8;
        gSkillManager.InvalidateSkillAttributeRequirementsCache();
    }

    void BlockReturnPath()
    {
        int wall = 0;
        for (int y = -1; y <= 1; ++y)
            for (int x = -1; x <= 1; ++x)
            {
                auto& terrain = TerrainWall[TERRAIN_INDEX(HeroTile + x, HeroRow + y)];
                previousWalls[wall++] = terrain;
                terrain = x == 0 && y == 0 ? 0 : TW_NOMOVE;
            }
        InitPath();
    }

    ~HelperActionFixture()
    {
        MUHelper::g_MuHelper.Stop();
        MUHelper::g_MuHelper.DeleteAllTargets();
        MUHelper::g_MuHelper.DeleteItem(DropSlot);
        MUHelper::g_MuHelper.DeleteItem(OtherDropSlot);
        for (int drop = 0; drop < previousDropLive.size(); ++drop)
        {
            auto& object = Items[DropSlot - drop].Object;
            object.Live = previousDropLive[drop];
            std::copy(previousDropPositions[drop].begin(), previousDropPositions[drop].end(), object.Position);
        }
        int wall = 0;
        for (int y = -1; y <= 1; ++y)
            for (int x = -1; x <= 1; ++x)
                TerrainWall[TERRAIN_INDEX(HeroTile + x, HeroRow + y)] = previousWalls[wall++];
        Hero = previousHero;
        CharactersClient = previousCharacters;
        CharacterMachine = previousMachine;
        CharacterAttribute = previousCharacter;
        SkillAttribute = previousAttributes;
        SceneFlag = previousScene;
        gSkillManager.InvalidateSkillAttributeRequirementsCache();
    }

    void PlaceDrop(int slot, int tileX)
    {
        Items[slot].Object.Live = true;
        Items[slot].Object.Position[0] = (tileX + 0.5f) * TERRAIN_SCALE;
        Items[slot].Object.Position[1] = (HeroRow + 0.5f) * TERRAIN_SCALE;
        MUHelper::g_MuHelper.AddItem(slot, {tileX, HeroRow});
    }

    void Tick(std::chrono::milliseconds elapsed = HelperTick)
    {
        now += elapsed;
        MUHelper::g_MuHelper.Tick(now);
    }
};
} // namespace

// Link-time action spies verify helper submissions without a game server; the real primitives have separate tests.
namespace GameLogic::Automation
{
SkillResult CastSkill(ActionSkillType skill, bool, int targetKey, bool, int)
{
    submittedCasts.emplace_back(skill, targetKey);
    if (const auto target = targetCastResults.find(targetKey); target != targetCastResults.end())
        return target->second;
    const auto result = castResults.find(skill);
    return result == castResults.end() ? SkillResult::Cast : result->second;
}

bool IsSelfPositionSkill(ActionSkillType)
{
    return false;
}

PickupResult PickUpItem(int slot, int)
{
    pickupAttempts.push_back(slot);
    return pickupResult;
}
} // namespace GameLogic::Automation

TEST_CASE("Blocked loot and return paths still submit successful cast actions [helper_actions]")
{
    HelperActionFixture fixture;
    constexpr int TicksThroughReturn = 6;
    for (int tick = 0; tick < TicksThroughReturn; ++tick)
        fixture.Tick();
    REQUIRE(submittedCasts.size() == TicksThroughReturn);
    for (int tick = 0; tick < TicksThroughReturn; ++tick)
    {
        CHECK(submittedCasts[tick].first == (tick % 2 == 0 ? Primary : Extra));
        CHECK(submittedCasts[tick].second == 123);
    }
    CHECK(pickupAttempts == std::vector<int>{DropSlot, DropSlot});
}

TEST_CASE("Blocked drops retry after cooldown while other loot and casts continue [helper_actions]")
{
    HelperActionFixture fixture;
    fixture.Tick();
    REQUIRE(pickupAttempts == std::vector<int>{DropSlot});
    fixture.PlaceDrop(OtherDropSlot, HeroTile + 4);
    pickupResult = GameLogic::Automation::PickupResult::Requested;
    fixture.Tick();
    CHECK(pickupAttempts == std::vector<int>{DropSlot, OtherDropSlot});
    CHECK(submittedCasts.size() == 2);
    constexpr auto AfterPickupRetry = std::chrono::milliseconds(1100);
    fixture.Tick(AfterPickupRetry);
    CHECK(pickupAttempts == std::vector<int>{DropSlot, OtherDropSlot, DropSlot});
    CHECK(submittedCasts.size() == 3);
}

TEST_CASE("A reused drop slot does not inherit a failed pickup cooldown [helper_actions]")
{
    HelperActionFixture fixture;
    fixture.Tick();
    MUHelper::g_MuHelper.DeleteItem(DropSlot);
    fixture.PlaceDrop(DropSlot, HeroTile + 2);
    fixture.Tick();
    CHECK(pickupAttempts == std::vector<int>{DropSlot, DropSlot});
    CHECK(submittedCasts.size() == 2);
}

TEST_CASE("A requested pickup stays tracked until the server removes the drop [helper_actions]")
{
    HelperActionFixture fixture;
    pickupResult = GameLogic::Automation::PickupResult::Requested;
    fixture.Tick();
    REQUIRE(pickupAttempts == std::vector<int>{DropSlot});
    fixture.Tick();
    CHECK(pickupAttempts.size() == 1);
    constexpr auto AfterRejectedPickup = std::chrono::seconds(3);
    fixture.Tick(AfterRejectedPickup);
    CHECK(pickupAttempts == std::vector<int>{DropSlot, DropSlot});
    MUHelper::g_MuHelper.DeleteItem(DropSlot);
    fixture.Tick(AfterRejectedPickup);
    CHECK(pickupAttempts.size() == 2);
}

TEST_CASE("A buff that cannot be cast does not starve later helper actions [helper_actions]")
{
    HelperActionFixture fixture;
    auto config = MUHelper::g_MuHelper.GetConfig();
    config.aiBuff = {AT_SKILL_ATTACK, 0, 0};
    MUHelper::g_MuHelper.Load(config);
    castResults[AT_SKILL_ATTACK] = GameLogic::Automation::SkillResult::Refused;
    fixture.Tick();
    REQUIRE(submittedCasts.size() == 2);
    CHECK(submittedCasts[0].first == AT_SKILL_ATTACK);
    CHECK(submittedCasts[1].first == Primary);
}

namespace
{
// The helper looks at one of the three buff slots per tick.
constexpr int BuffSlots = 3;

long CountBuffCasts()
{
    return std::count_if(submittedCasts.begin(), submittedCasts.end(),
                         [](const auto& cast) { return cast.first == AT_SKILL_ATTACK; });
}

void KeepAttackBuffActive(HelperActionFixture& fixture, bool bKeepBuffDuration)
{
    auto config = MUHelper::g_MuHelper.GetConfig();
    config.aiBuff = {AT_SKILL_ATTACK, 0, 0};
    config.bBuffDuration = bKeepBuffDuration;
    config.iBuffCastInterval = 30;
    MUHelper::g_MuHelper.Load(config);
    g_CharacterRegisterBuff((&fixture.hero.Object), eBuff_Attack);
}
} // namespace

TEST_CASE("Buff Duration recasts an active buff every casting interval, before it runs out [helper_actions]")
{
    HelperActionFixture fixture;
    KeepAttackBuffActive(fixture, true);
    fixture.Tick(std::chrono::milliseconds(0));
    CHECK(CountBuffCasts() == 1);
    for (int tick = 0; tick < 10; ++tick)
        fixture.Tick(std::chrono::seconds(2));
    CHECK(CountBuffCasts() == 1);
    fixture.Tick(std::chrono::seconds(15)); // 30 s after the first round ended
    for (int slot = 1; slot < BuffSlots; ++slot)
        fixture.Tick(std::chrono::milliseconds(0));
    CHECK(CountBuffCasts() == 2);
    g_CharacterUnRegisterBuff((&fixture.hero.Object), eBuff_Attack);
}

TEST_CASE("Without Buff Duration an active buff is left until it runs out [helper_actions]")
{
    HelperActionFixture fixture;
    KeepAttackBuffActive(fixture, false);
    for (int tick = 0; tick < 20; ++tick)
        fixture.Tick(std::chrono::seconds(5));
    CHECK(CountBuffCasts() == 0);
    g_CharacterUnRegisterBuff((&fixture.hero.Object), eBuff_Attack);
    for (int slot = 0; slot < BuffSlots; ++slot)
        fixture.Tick();
    CHECK(CountBuffCasts() == 1);
}

TEST_CASE("A buff short of mana holds the attacks for a while [helper_actions]")
{
    HelperActionFixture fixture;
    auto config = MUHelper::g_MuHelper.GetConfig();
    config.aiBuff = {AT_SKILL_ATTACK, 0, 0};
    MUHelper::g_MuHelper.Load(config);
    fixture.attributes[AT_SKILL_ATTACK].Mana = 40;
    fixture.machine.Character.ManaMax = 100;
    fixture.machine.Character.Mana = 10;
    castResults[AT_SKILL_ATTACK] = GameLogic::Automation::SkillResult::NotReady;
    const auto attacks = [] { return std::count_if(submittedCasts.begin(), submittedCasts.end(),
                                                   [](const auto& cast) { return cast.first != AT_SKILL_ATTACK; }); };

    // Support first: every attack would spend the mana the buff waits for.
    for (int tick = 0; tick < 8; ++tick)
        fixture.Tick();
    CHECK(attacks() == 0);
    CHECK(submittedCasts.size() == 8);

    // Mana that never comes back doesn't stall the helper: after the wait, it fights.
    fixture.Tick(std::chrono::seconds(5));
    CHECK(attacks() > 0);
}

TEST_CASE("A buff that is not ready with enough mana does not hold the attacks [helper_actions]")
{
    HelperActionFixture fixture;
    auto config = MUHelper::g_MuHelper.GetConfig();
    config.aiBuff = {AT_SKILL_ATTACK, 0, 0};
    MUHelper::g_MuHelper.Load(config);
    fixture.attributes[AT_SKILL_ATTACK].Mana = 40;
    fixture.machine.Character.ManaMax = 100;
    fixture.machine.Character.Mana = 100;
    castResults[AT_SKILL_ATTACK] = GameLogic::Automation::SkillResult::NotReady;
    fixture.Tick();
    REQUIRE(submittedCasts.size() == 2);
    CHECK(submittedCasts[0].first == AT_SKILL_ATTACK);
    CHECK(submittedCasts[1].first == Primary);
}

TEST_CASE("Starting the helper finds monsters that were already in view [helper_actions]")
{
    HelperActionFixture fixture;
    MUHelper::g_MuHelper.Stop();
    MUHelper::g_MuHelper.DeleteAllTargets();
    MUHelper::g_MuHelper.Start();
    fixture.Tick();
    REQUIRE_FALSE(submittedCasts.empty());
    CHECK(submittedCasts.back().second == 123);
}

TEST_CASE("Helper seconds follow the clock rather than the number of ticks [helper_actions]")
{
    HelperActionFixture fixture;
    auto config = MUHelper::g_MuHelper.GetConfig();
    config.aiSkillCondition[1] = MUHelper::ON_TIMER;
    config.aiSkillInterval[1] = 2;
    MUHelper::g_MuHelper.Load(config);
    fixture.Tick(std::chrono::milliseconds(0));
    for (int tick = 0; tick < 10; ++tick)
        fixture.Tick(std::chrono::milliseconds(150));
    CHECK(std::count(submittedCasts.begin(), submittedCasts.end(), std::pair{Extra, 123}) == 0);
    fixture.Tick(std::chrono::milliseconds(600));
    CHECK(std::count(submittedCasts.begin(), submittedCasts.end(), std::pair{Extra, 123}) == 1);
    fixture.Tick();
    CHECK(std::count(submittedCasts.begin(), submittedCasts.end(), std::pair{Extra, 123}) == 1);
}

TEST_CASE("Ending the session stops the helper and forgets its world [helper_actions]")
{
    HelperActionFixture fixture;
    MUHelper::g_MuHelper.EndSession();
    CHECK_FALSE(MUHelper::g_MuHelper.IsActive());
    MUHelper::g_MuHelper.Start();
    MUHelper::g_MuHelper.DeleteAllTargets();
    fixture.Tick();
    CHECK(pickupAttempts.empty());
    CHECK(submittedCasts.empty());
}

TEST_CASE("Refused pickups back off and are given up after three requests [helper_actions]")
{
    HelperActionFixture fixture;
    pickupResult = GameLogic::Automation::PickupResult::Requested;
    fixture.Tick();
    fixture.Tick(std::chrono::seconds(3));
    REQUIRE(pickupAttempts.size() == 2);
    fixture.Tick(std::chrono::seconds(3));
    CHECK(pickupAttempts.size() == 2);
    fixture.Tick(std::chrono::seconds(3));
    CHECK(pickupAttempts.size() == 3);
    fixture.Tick(std::chrono::seconds(30));
    CHECK(pickupAttempts.size() == 3);
}

TEST_CASE("A monster standing out of hunting range is attacked again once back in range [helper_actions]")
{
    HelperActionFixture fixture;
    constexpr int FarTile = HeroTile - 20;
    fixture.hero.PositionX = FarTile;
    fixture.Tick();
    CHECK(submittedCasts.empty());
    fixture.hero.PositionX = HeroTile;
    fixture.Tick();
    REQUIRE_FALSE(submittedCasts.empty());
    CHECK(submittedCasts.back().second == 123);
}

TEST_CASE("An unreachable monster waits out a cooldown while a reachable one is attacked [helper_actions]")
{
    HelperActionFixture fixture;
    auto& reachable = fixture.characters[1];
    reachable.Key = 124;
    reachable.Object.Live = true;
    reachable.Object.Kind = KIND_MONSTER;
    reachable.PositionX = reachable.TargetX = HeroTile + 2;
    reachable.PositionY = reachable.TargetY = HeroRow;
    MUHelper::g_MuHelper.AddTarget(reachable.Key, false);
    targetCastResults[123] = GameLogic::Automation::SkillResult::NoPath;
    fixture.Tick();
    REQUIRE(submittedCasts.size() == 1);
    CHECK(submittedCasts[0].second == 123);
    MUHelper::g_MuHelper.AddTarget(123, false);
    fixture.Tick();
    REQUIRE(submittedCasts.size() == 2);
    CHECK(submittedCasts[1].second == 124);
    MUHelper::g_MuHelper.DeleteTarget(124);
    MUHelper::g_MuHelper.AddTarget(123, false);
    fixture.Tick(std::chrono::seconds(2));
    REQUIRE(submittedCasts.size() == 3);
    CHECK(submittedCasts[2].second == 123);
}

TEST_CASE("A drop already walked to is picked up before the return home [helper_actions]")
{
    HelperActionFixture fixture;
    pickupResult = GameLogic::Automation::PickupResult::Approaching;
    fixture.Tick();
    REQUIRE(pickupAttempts == std::vector<int>{DropSlot});
    // The return delay has expired while a walk is still under way, so a regroup would wait on it.
    fixture.hero.Movement = true;
    fixture.hero.Path.PathNum = 2;
    fixture.hero.Path.CurrentPath = 0;
    pickupResult = GameLogic::Automation::PickupResult::Requested;
    fixture.Tick(std::chrono::seconds(1));
    CHECK(pickupAttempts == std::vector<int>{DropSlot, DropSlot});
}

namespace
{
constexpr int AllyKey = 200;
constexpr int OtherAllyKey = 201;

// A party of the hero and two allies standing next to it, the first ally wounded.
struct HealingPartyFixture : HelperActionFixture
{
    const int previousPartyNumber = PartyNumber;
    const std::array<PARTY_t, 3> previousParty = {Party[0], Party[1], Party[2]};

    HealingPartyFixture()
    {
        machine.Character.SkillNumber = 3;
        machine.Character.Skill[2] = AT_SKILL_HEALING;
        auto config = MUHelper::g_MuHelper.GetConfig();
        config.bAutoHeal = config.bAutoHealParty = true;
        config.iHealThreshold = config.iHealPartyThreshold = 50;
        MUHelper::g_MuHelper.Load(config);
        PartyNumber = 3;
        std::wcscpy(Party[0].Name, L"Hero"); // Not in view through CharactersClient: skipped.
        AddAlly(1, L"Ally", AllyKey, 2);
        AddAlly(2, L"OtherAlly", OtherAllyKey, 10);
    }

    ~HealingPartyFixture()
    {
        std::copy(previousParty.begin(), previousParty.end(), Party);
        PartyNumber = previousPartyNumber;
    }

    void AddAlly(int slot, const wchar_t* name, int key, BYTE stepHP)
    {
        std::wcscpy(Party[slot].Name, name);
        Party[slot].stepHP = stepHP;
        auto& ally = characters[slot];
        std::wcscpy(ally.ID, name);
        ally.Key = key;
        ally.Object.Live = true;
        ally.Object.Kind = KIND_PLAYER;
        ally.PositionX = HeroTile;
        ally.PositionY = HeroRow;
        std::copy_n(hero.Object.Position, 3, ally.Object.Position);
    }
};
} // namespace

TEST_CASE("A heal held up by a swing is cast on the same member before the next attack [helper_actions]")
{
    HealingPartyFixture fixture;
    fixture.hero.PositionX = HeroTile;
    fixture.Tick(); // The hero's slot: nothing to heal, so it attacks.
    REQUIRE_FALSE(submittedCasts.empty());
    REQUIRE(submittedCasts.back().first == Primary);
    targetCastResults[AllyKey] = GameLogic::Automation::SkillResult::Busy;
    fixture.Tick(); // The wounded ally's slot, while the swing is still on.
    REQUIRE(submittedCasts.back() == std::pair{AT_SKILL_HEALING, AllyKey});
    targetCastResults.erase(AllyKey);
    const auto castsBefore = submittedCasts.size();
    fixture.Tick(); // The swing is over.
    REQUIRE(submittedCasts.size() > castsBefore);
    CHECK(submittedCasts[castsBefore] == std::pair{AT_SKILL_HEALING, AllyKey});
}

TEST_CASE("A member who joins the party is buffed right away, not at the next casting interval [helper_actions]")
{
    HealingPartyFixture fixture;
    auto config = MUHelper::g_MuHelper.GetConfig();
    config.bAutoHeal = config.bAutoHealParty = false;
    config.aiBuff = {AT_SKILL_ATTACK, 0, 0};
    config.bSupportParty = config.bBuffDurationParty = true;
    config.iBuffCastInterval = 30;
    MUHelper::g_MuHelper.Load(config);
    PartyNumber = 2; // The hero and Ally; OtherAlly joins later.
    g_CharacterRegisterBuff((&fixture.characters[1].Object), eBuff_Attack);
    g_CharacterRegisterBuff((&fixture.characters[2].Object), eBuff_Attack);
    const auto castsOn = [](int key) { return std::count(submittedCasts.begin(), submittedCasts.end(), std::pair{AT_SKILL_ATTACK, key}); };

    // The first round buffs everyone; the next one is 30 s away.
    for (int tick = 0; tick < 12; ++tick)
        fixture.Tick();
    CHECK(castsOn(AllyKey) == 1);
    for (int tick = 0; tick < 12; ++tick)
        fixture.Tick(std::chrono::seconds(1));
    CHECK(castsOn(AllyKey) == 1);

    // OtherAlly joins with its buff still running: it's buffed within the next rotation, not 18 s later.
    PartyNumber = 3;
    for (int tick = 0; tick < 9; ++tick)
        fixture.Tick();
    CHECK(castsOn(OtherAllyKey) == 1);

    g_CharacterUnRegisterBuff((&fixture.characters[1].Object), eBuff_Attack);
    g_CharacterUnRegisterBuff((&fixture.characters[2].Object), eBuff_Attack);
}

TEST_CASE("the helper repairs worn equipment, but never arrows or bolts [gamelogic][helper][repair]")
{
    using GameLogic::Automation::ShouldRepairEquipment;
    // A worn weapon at 20 of 100 with a 50 % threshold is repaired; a quiver with 20 of 100 arrows is not (#913).
    CHECK(ShouldRepairEquipment(false, false, 20, 100, 50));
    CHECK_FALSE(ShouldRepairEquipment(false, true, 20, 100, 50));
    CHECK_FALSE(ShouldRepairEquipment(true, false, 20, 100, 50));
    // Health is rounded up: 50 of 99 is 51 %, above a 50 % threshold.
    CHECK_FALSE(ShouldRepairEquipment(false, false, 50, 99, 50));
    CHECK(ShouldRepairEquipment(false, false, 49, 99, 50));
    CHECK_FALSE(ShouldRepairEquipment(false, false, 0, 0, 50));
}
