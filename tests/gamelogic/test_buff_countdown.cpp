#include "stdafx.h"
#include "doctest.h"

#include "GameLogic/Buffs/BuffCountdown.h"

using GameLogic::Buffs::BuffCountdown;

TEST_CASE("A buff counts down from the duration the server sent [buffs][countdown]")
{
    BuffCountdown countdown;
    const auto start = BuffCountdown::Clock::now();
    countdown.Start(8, 120, start);

    CHECK(countdown.RemainingSeconds(8, start) == 120);
    CHECK(countdown.RemainingSeconds(8, start + std::chrono::milliseconds(500)) == 120);
    CHECK(countdown.RemainingSeconds(8, start + std::chrono::seconds(119)) == 1);
    // Until the server removes the buff, it shows 0 rather than disappearing early.
    CHECK(countdown.RemainingSeconds(8, start + std::chrono::seconds(200)) == 0);
    CHECK_FALSE(countdown.RemainingSeconds(9, start).has_value());
}

TEST_CASE("A recast restarts the countdown, and a removed or endless buff has none [buffs][countdown]")
{
    BuffCountdown countdown;
    const auto start = BuffCountdown::Clock::now();
    countdown.Start(8, 60, start);
    countdown.Start(8, 60, start + std::chrono::seconds(50));
    CHECK(countdown.RemainingSeconds(8, start + std::chrono::seconds(50)) == 60);

    countdown.Stop(8);
    CHECK_FALSE(countdown.RemainingSeconds(8, start).has_value());

    countdown.Start(8, 60, start);
    countdown.Start(8, BuffCountdown::UnknownSeconds, start);
    CHECK_FALSE(countdown.RemainingSeconds(8, start).has_value());
}

TEST_CASE("The icon and tooltip texts of a countdown [buffs][countdown]")
{
    CHECK(BuffCountdown::ShortText(45) == "45");
    CHECK(BuffCountdown::ShortText(60) == "1m");
    CHECK(BuffCountdown::ShortText(61) == "2m");
    CHECK(BuffCountdown::ShortText(3600) == "1h");
    CHECK(BuffCountdown::ShortText(3601) == "2h");
    CHECK(BuffCountdown::LongText(45) == L"0:45");
    CHECK(BuffCountdown::LongText(725) == L"12:05");
    CHECK(BuffCountdown::LongText(3723) == L"1:02:03");
}
