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
        constexpr std::int64_t SellPricePerLevel = 5000;
        constexpr int BuyToSellRatio = 3;
        return SellPricePerLevel * BuyToSellRatio * std::max(1, level);
    }
    default:
        return std::nullopt;
    }
}
}
