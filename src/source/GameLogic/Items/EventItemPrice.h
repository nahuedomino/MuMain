#pragma once

#include <cstdint>
#include <optional>

namespace GameLogic::Items
{
std::optional<std::int64_t> GetEventItemBuyingPrice(int itemType, int level);
}
