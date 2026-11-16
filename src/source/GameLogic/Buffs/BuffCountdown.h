#pragma once

#include <chrono>
#include <map>
#include <optional>
#include <string>

// MU Volta: the remaining time of the hero's own buffs, as the server reports it (packet C1 07 FA 01), for the
// countdown on the buff icons. OpenMU's effect status packet carries no time; the item effect time
// (BuffTimeControl) only covers consumables.
namespace GameLogic::Buffs
{
class BuffCountdown
{
public:
    using Clock = std::chrono::steady_clock;

    // The value the server sends when the duration is unknown or too long to count down.
    static constexpr unsigned short UnknownSeconds = 0xFFFF;

    // The buff started or was cast again: it lasts `seconds` from `now`.
    void Start(int buff, unsigned short seconds, Clock::time_point now);
    void Stop(int buff);
    void Clear();

    // Whole seconds left, rounded up; nothing when the buff has no countdown.
    [[nodiscard]] std::optional<int> RemainingSeconds(int buff, Clock::time_point now) const;

    // The text on the icon: "45", "9m", "2h" (seconds, minutes, hours left, rounded up).
    [[nodiscard]] static std::string ShortText(int seconds);
    // The tooltip's time: "0:45", "12:05", "1:02:03".
    [[nodiscard]] static std::wstring LongText(int seconds);

private:
    std::map<int, Clock::time_point> m_endsAt;
};

BuffCountdown& HeroBuffCountdown();
} // namespace GameLogic::Buffs
