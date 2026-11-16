#include "stdafx.h"
#include "GameLogic/Events/EventSchedule.h"

#include <cstdio>
#include <ctime>

namespace GameLogic::Events
{
namespace
{
class Reader
{
public:
    explicit Reader(std::span<const std::uint8_t> data) : m_data(data) {}

    bool Byte(std::uint8_t& value)
    {
        if (m_offset + 1 > m_data.size())
            return false;
        value = m_data[m_offset++];
        return true;
    }

    bool UInt16(std::uint16_t& value)
    {
        if (m_offset + 2 > m_data.size())
            return false;
        value = static_cast<std::uint16_t>(m_data[m_offset] | (m_data[m_offset + 1] << 8));
        m_offset += 2;
        return true;
    }

    bool Int32(std::int32_t& value)
    {
        if (m_offset + 4 > m_data.size())
            return false;
        const std::uint32_t raw = static_cast<std::uint32_t>(m_data[m_offset]) | (static_cast<std::uint32_t>(m_data[m_offset + 1]) << 8) |
                                  (static_cast<std::uint32_t>(m_data[m_offset + 2]) << 16) |
                                  (static_cast<std::uint32_t>(m_data[m_offset + 3]) << 24);
        value = static_cast<std::int32_t>(raw);
        m_offset += 4;
        return true;
    }

    bool Text(std::size_t length, std::string& value)
    {
        if (m_offset + length > m_data.size())
            return false;
        value.assign(reinterpret_cast<const char*>(m_data.data() + m_offset), length);
        m_offset += length;
        return true;
    }

private:
    std::span<const std::uint8_t> m_data;
    std::size_t m_offset = 0;
};
} // namespace

std::optional<EventSchedule> ParseSchedule(std::span<const std::uint8_t> packet, Clock::time_point now)
{
    if (packet.size() < 6 || packet[0] != 0xC2 || packet[3] != 0xFA || packet[4] != 0x02)
        return std::nullopt;

    const std::size_t length = (static_cast<std::size_t>(packet[1]) << 8) | packet[2];
    if (length > packet.size())
        return std::nullopt;

    Reader reader(packet.subspan(5, length - 5));
    std::uint8_t count = 0;
    if (!reader.Byte(count))
        return std::nullopt;

    EventSchedule schedule;
    schedule.receivedAt = now;
    for (int i = 0; i < count; ++i)
    {
        ScheduledEvent event;
        std::uint8_t nameLength = 0;
        std::uint16_t minutes = 0;
        std::uint8_t startCount = 0;
        if (!reader.Byte(nameLength) || !reader.Text(nameLength, event.name) || !reader.UInt16(minutes) || !reader.Byte(startCount))
            return std::nullopt;

        event.duration = std::chrono::minutes(minutes);
        for (int s = 0; s < startCount; ++s)
        {
            std::int32_t seconds = 0;
            if (!reader.Int32(seconds))
                return std::nullopt;
            event.starts.push_back(now + std::chrono::seconds(seconds));
        }

        schedule.events.push_back(std::move(event));
    }

    return schedule;
}

std::optional<NextRun> GetNextRun(const ScheduledEvent& event, Clock::time_point now)
{
    for (const auto start : event.starts)
    {
        if (start <= now && now < start + event.duration)
            return NextRun{start, true};
        if (start > now)
            return NextRun{start, false};
    }

    return std::nullopt;
}

std::string FormatCountdown(std::chrono::seconds left)
{
    long long total = left.count() < 0 ? 0 : left.count();
    const long long hours = total / 3600;
    const long long minutes = (total / 60) % 60;
    const long long seconds = total % 60;
    char text[32] = {};
    if (hours > 0)
        std::snprintf(text, sizeof(text), "%lldh %02lldm", hours, minutes);
    else if (minutes > 0)
        std::snprintf(text, sizeof(text), "%lldm %02llds", minutes, seconds);
    else
        std::snprintf(text, sizeof(text), "%llds", seconds);
    return text;
}

std::string FormatLocalTime(Clock::time_point time)
{
    const std::time_t value = Clock::to_time_t(time);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &value);
#else
    localtime_r(&value, &local);
#endif
    char text[8] = {};
    std::snprintf(text, sizeof(text), "%02d:%02d", local.tm_hour, local.tm_min);
    return text;
}

EventSchedule& CurrentSchedule()
{
    static EventSchedule schedule;
    return schedule;
}
} // namespace GameLogic::Events
