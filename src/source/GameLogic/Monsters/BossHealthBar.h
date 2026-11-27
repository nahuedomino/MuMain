#pragma once

#include <algorithm>
#include <cstdint>
#include <cwchar>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>

// MU Volta: the health bar over the bosses, seen by every player near them. The server (mu-volta-plugins
// "boss health bars") sends C1 0E FA 03, object id (2 bytes), current health (4 bytes), maximum health (4 bytes),
// all little endian, after hits (at most 4 a second) and every 2 s to everyone who sees a living boss.
namespace GameLogic::BossHealthBar
{
inline constexpr std::size_t PacketSize = 14;
inline constexpr unsigned char SubCode = 0x03;

/// A boss whose last update is older than this has left the view or died; its bar is hidden.
inline constexpr std::uint64_t StaleAfterMs = 6000;

struct Update
{
    std::uint16_t key;
    std::uint32_t current;
    std::uint32_t maximum;
};

/// Reads the boss health packet; nothing for a short packet, another sub code or no maximum.
inline std::optional<Update> ParsePacket(std::span<const unsigned char> packet)
{
    if (packet.size() < PacketSize || packet[0] != 0xC1 || packet[2] != 0xFA || packet[3] != SubCode)
        return std::nullopt;

    const auto u32 = [&packet](std::size_t at) {
        return static_cast<std::uint32_t>(packet[at]) | (static_cast<std::uint32_t>(packet[at + 1]) << 8)
               | (static_cast<std::uint32_t>(packet[at + 2]) << 16) | (static_cast<std::uint32_t>(packet[at + 3]) << 24);
    };
    Update update{ static_cast<std::uint16_t>(packet[4] | (packet[5] << 8)), u32(6), u32(10) };
    if (update.maximum == 0)
        return std::nullopt;
    update.current = std::min(update.current, update.maximum);
    return update;
}

/// The health as a short text that fits over the bar: 850, 8.5k, 35k, 1.2M, 12M. Rounded down, so a boss with a
/// sliver of health never shows the next unit up.
inline std::wstring FormatCompact(std::uint32_t health)
{
    wchar_t text[16];
    const auto scaled = [&text](std::uint32_t value, std::uint32_t unit, const wchar_t* suffix) {
        if (value < 10 * unit)
        {
            const std::uint32_t tenths = value / (unit / 10);
            if (tenths % 10 != 0)
            {
                std::swprintf(text, 16, L"%u.%u%ls", tenths / 10, tenths % 10, suffix);
                return std::wstring(text);
            }
        }
        std::swprintf(text, 16, L"%u%ls", value / unit, suffix);
        return std::wstring(text);
    };

    if (health < 1'000)
    {
        std::swprintf(text, 16, L"%u", health);
        return text;
    }
    if (health < 1'000'000)
        return scaled(health, 1'000, L"k");
    if (health < 1'000'000'000)
        return scaled(health, 1'000'000, L"M");
    return scaled(health, 1'000'000'000, L"G");
}

/// The fill of the bar, 0..1.
inline float Fraction(std::uint32_t current, std::uint32_t maximum)
{
    return maximum == 0 ? 0.f : std::clamp(static_cast<float>(current) / static_cast<float>(maximum), 0.f, 1.f);
}

struct Entry
{
    std::uint32_t current = 0;
    std::uint32_t maximum = 0;
    std::uint64_t receivedAtMs = 0;
    int monsterType = -1; // the monster the id belonged to when the update came
    float shown = 1.f;   // the eased fill drawn
    bool fresh = true;   // no fill drawn yet: start at the reported one
};

/// The last health the server sent for each boss in view, by object id.
class Table
{
public:
    /// Keeps the update; monsterType is the monster with that id in view (-1 if none), so an id that a map change
    /// gives to another monster never shows the old boss's bar.
    void Apply(const Update& update, std::uint64_t nowMs, int monsterType)
    {
        auto& entry = m_entries[update.key];
        if (entry.maximum != update.maximum || entry.monsterType != monsterType)
            entry.fresh = true; // another boss under a reused id
        entry.current = update.current;
        entry.maximum = update.maximum;
        entry.receivedAtMs = nowMs;
        entry.monsterType = monsterType;
    }

    /// The entry of the monster with that id and type while its last update is recent; nullptr otherwise.
    Entry* Find(std::uint16_t key, int monsterType, std::uint64_t nowMs)
    {
        const auto it = m_entries.find(key);
        if (it == m_entries.end() || it->second.monsterType != monsterType
            || nowMs - it->second.receivedAtMs > StaleAfterMs)
            return nullptr;
        return &it->second;
    }

    /// Forgets the bosses not updated for a while.
    void Prune(std::uint64_t nowMs)
    {
        std::erase_if(m_entries, [nowMs](const auto& pair) { return nowMs - pair.second.receivedAtMs > StaleAfterMs; });
    }

    void Clear() { m_entries.clear(); }

    std::size_t Size() const { return m_entries.size(); }

private:
    std::unordered_map<std::uint16_t, Entry> m_entries;
};

/// The table of the running game.
inline Table& Current()
{
    static Table table;
    return table;
}
} // namespace GameLogic::BossHealthBar
