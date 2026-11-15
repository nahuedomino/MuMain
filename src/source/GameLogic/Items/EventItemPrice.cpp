#include "stdafx.h"
#include "GameLogic/Items/EventItemPrice.h"

#include <algorithm>

namespace GameLogic::Items
{
std::optional<std::int64_t> GetEventItemBuyingPrice(int itemType, int level)
{
    switch (itemType)
    {
    case ITEM_DEVILS_EYE:
    case ITEM_DEVILS_KEY:
    case ITEM_DEVILS_INVITATION:
    case ITEM_SCROLL_OF_ARCHANGEL:
    case ITEM_BLOOD_BONE:
    case ITEM_INVISIBILITY_CLOAK:
    case ITEM_OLD_SCROLL:
    case ITEM_ILLUSION_SORCERER_COVENANT:
    case ITEM_SCROLL_OF_BLOOD:
    {
        // Matches the server's "MU Volta event prices (early game)": 2,500 Zen per level up to +4, then 15,000 at
        // +5 and 5,000 more per level (+7: 25,000, +8: 30,000); level 0 sells like +1. NPCs ask three times that.
        constexpr int BuyToSellRatio = 3;
        const std::int64_t selling = level <= 4 ? 2500LL * std::max(1, level) : 5000LL * (level - 2);
        return selling * BuyToSellRatio;
    }
    default:
        return std::nullopt;
    }
}
}
