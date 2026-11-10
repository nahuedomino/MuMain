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

    bool Request(Clock::time_point now = Clock::now())
    {
        if (!IsReady(now))
            return false;
        m_requestedAt = now;
        return true;
    }

    bool Begin(Clock::time_point now = Clock::now())
    {
        if (m_inFlight || (m_readyAt && now < *m_readyAt))
            return false;
        m_requestedAt.reset();
        m_inFlight = true;
        return true;
    }

    bool Land(Clock::time_point now = Clock::now())
    {
        m_requestedAt.reset();
        const bool landed = m_inFlight;
        if (m_inFlight)
            m_readyAt = now + Recovery;
        m_inFlight = false;
        return landed;
    }

    void Reset()
    {
        m_inFlight = false;
        m_requestedAt.reset();
        m_readyAt.reset();
    }

    bool IsReady(Clock::time_point now = Clock::now()) const
    {
        // A missing rejection/confirmation must not permanently disable the skill.
        const bool pending = m_requestedAt && now - *m_requestedAt < std::chrono::seconds(2);
        return !pending && !m_inFlight && (!m_readyAt || now >= *m_readyAt);
    }

    bool RequestTimedOut(Clock::time_point now = Clock::now()) const
    {
        return m_requestedAt && now - *m_requestedAt >= std::chrono::seconds(2);
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
    std::optional<Clock::time_point> m_requestedAt;
    std::optional<Clock::time_point> m_readyAt;
};
}
