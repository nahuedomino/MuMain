#pragma once

#include <algorithm>
#include <cstdint>

// The health bar drawn over party members' heads: how full it is, its color and how it eases
// toward a new value. The server reports party health in steps of 10%.
namespace GameLogic::PartyHealthBar
{
inline constexpr int MaxHealthSteps = 10;

inline constexpr uint32_t HighHealthColor = 0xFF2EC24Au;
inline constexpr uint32_t MediumHealthColor = 0xFFE0B020u;
inline constexpr uint32_t LowHealthColor = 0xFFD8322Au;

/// The fill of the bar, 0..1, for the health steps (0..10) the server sent.
inline float HealthFractionFromSteps(int healthSteps)
{
    return static_cast<float>(std::clamp(healthSteps, 0, MaxHealthSteps)) / MaxHealthSteps;
}

/// Green above half health, amber down to a quarter, red below.
inline uint32_t FillColor(float healthFraction)
{
    if (healthFraction > 0.5f)
        return HighHealthColor;
    if (healthFraction > 0.25f)
        return MediumHealthColor;
    return LowHealthColor;
}

/// The color mixed toward white by amount (0..1), for the lighter top edge of the fill.
inline uint32_t Highlight(uint32_t argb, float amount)
{
    amount = std::clamp(amount, 0.f, 1.f);
    const auto mix = [amount](uint32_t channel) {
        return static_cast<uint32_t>(channel + (255u - channel) * amount + 0.5f);
    };
    return (argb & 0xFF000000u) | (mix((argb >> 16) & 0xFFu) << 16) | (mix((argb >> 8) & 0xFFu) << 8)
           | mix(argb & 0xFFu);
}

/// Moves the shown fill toward the reported one at speed fractions per second, never past it.
inline float EaseToward(float shownFraction, float reportedFraction, float elapsedSeconds, float speed = 2.f)
{
    const float maxStep = std::max(0.f, elapsedSeconds) * speed;
    const float difference = reportedFraction - shownFraction;
    if (difference > maxStep)
        return shownFraction + maxStep;
    if (difference < -maxStep)
        return shownFraction - maxStep;
    return reportedFraction;
}
} // namespace GameLogic::PartyHealthBar
