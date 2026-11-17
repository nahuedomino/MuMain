#include "stdafx.h"
#include "doctest.h"

#include "GameLogic/Events/EventSchedule.h"
#include "UI/Dialogs/EventScheduleWindow.h"

#include <array>
#include <cstring>
#include <vector>

using namespace GameLogic::Events;

namespace
{
std::vector<std::uint8_t> KanturuPacket()
{
    // The server's EventScheduleTest packet: Kanturu, 20 minutes, started 10 minutes ago and again in an hour.
    std::vector<std::uint8_t> packet = {0xC2, 0x00, 0x00, 0xFA, 0x02, 0x01, 0x07, 'K', 'a', 'n', 't', 'u', 'r', 'u', 20, 0, 2};
    for (const std::int32_t seconds : {-600, 3600})
    {
        std::array<std::uint8_t, 4> bytes{};
        std::memcpy(bytes.data(), &seconds, 4);
        packet.insert(packet.end(), bytes.begin(), bytes.end());
    }
    packet[2] = static_cast<std::uint8_t>(packet.size());
    return packet;
}
} // namespace

TEST_CASE("The event schedule packet gives the names, durations and starts [events][schedule]")
{
    const auto now = std::chrono::floor<std::chrono::minutes>(Clock::now());
    const auto packet = KanturuPacket();
    const auto schedule = ParseSchedule(packet, now);

    REQUIRE(schedule.has_value());
    REQUIRE(schedule->events.size() == 1);
    const auto& event = schedule->events[0];
    CHECK(event.name == "Kanturu");
    CHECK(event.duration == std::chrono::minutes(20));
    REQUIRE(event.starts.size() == 2);
    CHECK(event.starts[0] == now - std::chrono::seconds(600));
    CHECK(event.starts[1] == now + std::chrono::seconds(3600));

    // Running now, 10 minutes left; once over, the next start is an hour away.
    auto run = GetNextRun(event, now);
    REQUIRE(run.has_value());
    CHECK(run->running);
    run = GetNextRun(event, now + std::chrono::minutes(11));
    REQUIRE(run.has_value());
    CHECK_FALSE(run->running);
    CHECK(run->start == now + std::chrono::seconds(3600));
    CHECK_FALSE(GetNextRun(event, now + std::chrono::hours(2)).has_value());
}

TEST_CASE("A start the server rounded down to the second is shown on its minute [events][schedule]")
{
    // The packet says 3600 s, but the request reached the server 0.6 s after `now`'s whole minute.
    const auto minute = std::chrono::floor<std::chrono::minutes>(Clock::now());
    const auto received = minute - std::chrono::milliseconds(600);
    const auto packet = KanturuPacket();
    const auto schedule = ParseSchedule(packet, received);

    REQUIRE(schedule.has_value());
    CHECK(schedule->events[0].starts[1] == minute + std::chrono::hours(1));
    CHECK(FormatLocalTime(schedule->events[0].starts[1]) == FormatLocalTime(minute + std::chrono::hours(1)));
}

TEST_CASE("A truncated or foreign packet is not a schedule [events][schedule]")
{
    auto packet = KanturuPacket();
    packet.resize(packet.size() - 2);
    packet[2] = static_cast<std::uint8_t>(packet.size());
    CHECK_FALSE(ParseSchedule(packet, Clock::now()).has_value());
    packet = KanturuPacket();
    packet[4] = 0x01;
    CHECK_FALSE(ParseSchedule(packet, Clock::now()).has_value());
}

TEST_CASE("The countdown text [events][schedule]")
{
    CHECK(FormatCountdown(std::chrono::seconds(45)) == "45s");
    CHECK(FormatCountdown(std::chrono::seconds(725)) == "12m 05s");
    CHECK(FormatCountdown(std::chrono::seconds(5100)) == "1h 25m");
    CHECK(FormatCountdown(std::chrono::seconds(-3)) == "0s");
}

TEST_CASE("The event window counts down the running event first, then the next ones [events][schedule]")
{
    const auto now = std::chrono::floor<std::chrono::minutes>(Clock::now());
    const auto packet = KanturuPacket();
    auto schedule = *ParseSchedule(packet, now);
    schedule.events.push_back({"Blood Castle", std::chrono::minutes(20), {now + std::chrono::minutes(30)}});
    CurrentSchedule() = schedule;

    const auto waiting = mu::ui::window::CEventScheduleWindow::BuildLines(false, now);
    // The window's title says "Events"; while waiting, one line says so.
    REQUIRE(waiting.size() == 1);
    CHECK_FALSE(waiting[0].heading);

    const auto lines = mu::ui::window::CEventScheduleWindow::BuildLines(true, now);
    REQUIRE(lines.size() >= 5);
    CHECK(lines[0].text.find("Kanturu") == 0);
    CHECK(lines[0].text.find("10m 00s") != std::string::npos);
    CHECK(lines[1].text.find("Blood Castle") == 0);
    CHECK(lines[1].text.find("30m 00s") != std::string::npos);
    CHECK(lines[2].halfSpacer);
    CHECK(lines[3].heading);
    CHECK(lines[4].text == "Kanturu: " + FormatLocalTime(now - std::chrono::seconds(600)) + ", " +
                               FormatLocalTime(now + std::chrono::seconds(3600)));

    CurrentSchedule() = {};
}
