#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

// MU Volta: the schedule of the server's events (mini games, Happy Hour, invasions) for the event window on the H key.
// Request C1 04 FA 02; response C2 <length> FA 02, count, then per event: name length and UTF-8 name, duration in
// minutes (2 bytes LE), start count, starts in seconds from now (4 bytes LE each, negative while the event runs).
namespace GameLogic::Events
{
using Clock = std::chrono::system_clock;

struct ScheduledEvent
{
    std::string name;
    std::chrono::minutes duration{};
    std::vector<Clock::time_point> starts;
};

struct EventSchedule
{
    std::vector<ScheduledEvent> events;
    Clock::time_point receivedAt{};
};

// Parses the server's response; nothing if the packet is malformed.
[[nodiscard]] std::optional<EventSchedule> ParseSchedule(std::span<const std::uint8_t> packet, Clock::time_point now);

// The next start or the running run: the start, and whether it's running at `now`.
struct NextRun
{
    Clock::time_point start;
    bool running = false;
};
[[nodiscard]] std::optional<NextRun> GetNextRun(const ScheduledEvent& event, Clock::time_point now);

// "1h 25m", "12m 05s", "45s".
[[nodiscard]] std::string FormatCountdown(std::chrono::seconds left);

// The local clock time of a start, "21:30".
[[nodiscard]] std::string FormatLocalTime(Clock::time_point time);

// The latest schedule from the server.
EventSchedule& CurrentSchedule();
} // namespace GameLogic::Events
