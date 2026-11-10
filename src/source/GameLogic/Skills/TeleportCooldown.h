#pragma once

#include <algorithm>
#include <chrono>
#include <optional>

namespace GameLogic::Skills
{
class TeleportCooldown
{
public:
    using Clock = std::chrono::steady_clock;
    static constexpr auto Recovery = std::chrono::milliseconds(2000);

    void Begin() { m_inFlight = true; }

    void Land(Clock::time_point now = Clock::now())
    {
        if (m_inFlight)
            m_readyAt = now + Recovery;
        m_inFlight = false;
    }

    void Reset()
    {
        m_inFlight = false;
        m_readyAt.reset();
    }

    bool IsReady(Clock::time_point now = Clock::now()) const
    {
        return !m_inFlight && (!m_readyAt || now >= *m_readyAt);
    }

    float Fraction(Clock::time_point now = Clock::now()) const
    {
        if (m_inFlight)
            return 1.f;
        if (!m_readyAt)
            return 0.f;
        return std::clamp(std::chrono::duration<float>(*m_readyAt - now).count()
                              / std::chrono::duration<float>(Recovery).count(),
                          0.f, 1.f);
    }

private:
    bool m_inFlight = false;
    std::optional<Clock::time_point> m_readyAt;
};
}
