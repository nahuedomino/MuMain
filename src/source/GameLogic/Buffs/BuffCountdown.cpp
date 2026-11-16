#include "stdafx.h"
#include "GameLogic/Buffs/BuffCountdown.h"

#include <cstdio>

namespace GameLogic::Buffs
{
void BuffCountdown::Start(int buff, unsigned short seconds, Clock::time_point now)
{
    if (seconds == UnknownSeconds || seconds == 0)
    {
        m_endsAt.erase(buff);
        return;
    }

    m_endsAt[buff] = now + std::chrono::seconds(seconds);
}

void BuffCountdown::Stop(int buff)
{
    m_endsAt.erase(buff);
}

void BuffCountdown::Clear()
{
    m_endsAt.clear();
}

std::optional<int> BuffCountdown::RemainingSeconds(int buff, Clock::time_point now) const
{
    const auto entry = m_endsAt.find(buff);
    if (entry == m_endsAt.end())
    {
        return std::nullopt;
    }

    // A buff the server hasn't removed yet shows 0 rather than disappearing early.
    const auto left = std::chrono::ceil<std::chrono::seconds>(entry->second - now).count();
    return left > 0 ? static_cast<int>(left) : 0;
}

std::string BuffCountdown::ShortText(int seconds)
{
    if (seconds < 60)
    {
        return std::to_string(seconds < 0 ? 0 : seconds);
    }

    if (seconds < 3600)
    {
        return std::to_string((seconds + 59) / 60) + "m";
    }

    return std::to_string((seconds + 3599) / 3600) + "h";
}

std::wstring BuffCountdown::LongText(int seconds)
{
    if (seconds < 0)
    {
        seconds = 0;
    }

    wchar_t text[32] = {};
    if (seconds >= 3600)
    {
        std::swprintf(text, 32, L"%d:%02d:%02d", seconds / 3600, (seconds / 60) % 60, seconds % 60);
    }
    else
    {
        std::swprintf(text, 32, L"%d:%02d", seconds / 60, seconds % 60);
    }

    return text;
}

BuffCountdown& HeroBuffCountdown()
{
    static BuffCountdown countdown;
    return countdown;
}
} // namespace GameLogic::Buffs
