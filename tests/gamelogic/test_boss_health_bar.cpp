// Tests of the MU Volta boss health bar: the packet, the short health text and the table of bosses in view.

#include "stdafx.h"

#include "doctest.h"

#include "GameLogic/Monsters/BossHealthBar.h"

#include <array>

namespace Boss = GameLogic::BossHealthBar;

namespace
{
// C1 0E FA 03, id 0x1234, 35,000 of 300,000 (the plugin test's bytes).
constexpr std::array<unsigned char, 14> Packet{ 0xC1, 0x0E, 0xFA, 0x03, 0x34, 0x12, 0xB8, 0x88,
                                                0x00, 0x00, 0xE0, 0x93, 0x04, 0x00 };
} // namespace

TEST_CASE("boss health packet gives the id and both health values")
{
    const auto update = Boss::ParsePacket(Packet);
    REQUIRE(update.has_value());
    CHECK(update->key == 0x1234);
    CHECK(update->current == 35000u);
    CHECK(update->maximum == 300000u);
}

TEST_CASE("boss health packet is ignored when short, of another sub code or without a maximum")
{
    CHECK_FALSE(Boss::ParsePacket(std::span(Packet).first(13)).has_value());

    auto other = Packet;
    other[3] = 0x02;
    CHECK_FALSE(Boss::ParsePacket(other).has_value());

    auto noMaximum = Packet;
    noMaximum[10] = noMaximum[11] = noMaximum[12] = noMaximum[13] = 0;
    CHECK_FALSE(Boss::ParsePacket(noMaximum).has_value());
}

TEST_CASE("boss health packet caps the current health at the maximum")
{
    auto over = Packet;
    over[6] = over[7] = over[8] = 0xFF;
    over[9] = 0x00;
    CHECK(Boss::ParsePacket(over)->current == 300000u);
}

TEST_CASE("boss health text stays short and rounds down")
{
    CHECK(Boss::FormatCompact(0) == L"0");
    CHECK(Boss::FormatCompact(850) == L"850");
    CHECK(Boss::FormatCompact(999) == L"999");
    CHECK(Boss::FormatCompact(1000) == L"1k");
    CHECK(Boss::FormatCompact(8550) == L"8.5k");
    CHECK(Boss::FormatCompact(9999) == L"9.9k");
    CHECK(Boss::FormatCompact(35000) == L"35k");
    CHECK(Boss::FormatCompact(35999) == L"35k");
    CHECK(Boss::FormatCompact(999999) == L"999k");
    CHECK(Boss::FormatCompact(1000000) == L"1M");
    CHECK(Boss::FormatCompact(1250000) == L"1.2M");
    CHECK(Boss::FormatCompact(12000000) == L"12M");
    CHECK(Boss::FormatCompact(4000000000u) == L"4G");
}

TEST_CASE("boss health fill is the share of the maximum")
{
    CHECK(Boss::Fraction(35000, 300000) == doctest::Approx(0.11667f).epsilon(0.001));
    CHECK(Boss::Fraction(0, 100) == doctest::Approx(0.f));
    CHECK(Boss::Fraction(100, 100) == doctest::Approx(1.f));
    CHECK(Boss::Fraction(5, 0) == doctest::Approx(0.f));
}

TEST_CASE("boss health table finds a boss while its updates are recent and of the same monster")
{
    Boss::Table table;
    table.Apply({ 7, 500, 1000 }, 10'000, 38);

    REQUIRE(table.Find(7, 38, 10'000) != nullptr);
    CHECK(table.Find(7, 38, 10'000)->current == 500u);
    CHECK(table.Find(7, 38, 10'000 + Boss::StaleAfterMs) != nullptr);
    CHECK(table.Find(7, 38, 10'001 + Boss::StaleAfterMs) == nullptr);
    CHECK(table.Find(7, 25, 10'000) == nullptr); // the id now belongs to another monster
    CHECK(table.Find(8, 38, 10'000) == nullptr);

    table.Prune(10'001 + Boss::StaleAfterMs);
    CHECK(table.Size() == 0);
}

TEST_CASE("boss health table starts the fill over for another boss under the same id")
{
    Boss::Table table;
    table.Apply({ 7, 500, 1000 }, 0, 38);
    auto* entry = table.Find(7, 38, 0);
    REQUIRE(entry != nullptr);
    entry->fresh = false;

    table.Apply({ 7, 400, 1000 }, 100, 38);
    CHECK_FALSE(table.Find(7, 38, 100)->fresh);

    table.Apply({ 7, 60000, 60000 }, 200, 25);
    CHECK(table.Find(7, 25, 200)->fresh);
}
