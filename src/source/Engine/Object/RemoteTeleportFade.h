#pragma once

// MU Volta: when another player's teleport fade-out started, for
// RecoverRemoteTeleportVisibility. Kept beside CHARACTER rather than in it,
// because the recorder tests pin CHARACTER's layout (tests/effects/RecorderFields.h).

#include <chrono>
#include <optional>
#include <unordered_map>

class CHARACTER;

namespace RemoteTeleportFade
{
using TimePoint = std::chrono::steady_clock::time_point;

inline std::unordered_map<const CHARACTER*, TimePoint>& Starts()
{
    static std::unordered_map<const CHARACTER*, TimePoint> starts;
    return starts;
}

inline std::optional<TimePoint> StartedAt(const CHARACTER* character)
{
    const auto& starts = Starts();
    const auto found = starts.find(character);
    return found == starts.end() ? std::nullopt : std::optional<TimePoint>(found->second);
}

inline void Set(const CHARACTER* character, TimePoint startedAt)
{
    Starts()[character] = startedAt;
}

inline void Clear(const CHARACTER* character)
{
    Starts().erase(character);
}
} // namespace RemoteTeleportFade
