#include "stdafx.h"
#include <doctest.h>

#include "Engine/Object/ZzzInfomation.h"
#include "GameLogic/Items/EventItemPrice.h"

TEST_CASE("MU Volta event items sell for 5000 Zen per level [items][economy]")
{
    const int types[] = {
        ITEM_DEVILS_EYE, ITEM_DEVILS_KEY, ITEM_DEVILS_INVITATION,
        ITEM_SCROLL_OF_ARCHANGEL, ITEM_BLOOD_BONE, ITEM_INVISIBILITY_CLOAK,
        ITEM_OLD_SCROLL, ITEM_ILLUSION_SORCERER_COVENANT, ITEM_SCROLL_OF_BLOOD
    };
    for (const int type : types)
    {
        for (int level = 0; level <= 8; ++level)
        {
            CAPTURE(type);
            CAPTURE(level);
            ITEM item{};
            item.Type = static_cast<short>(type);
            item.Level = level;
            item.Durability = 1;
            const std::int64_t selling = 5000LL * std::max(1, level);
            CHECK(ItemValue(&item, 1) == selling);
            CHECK(ItemValue(&item, 0) == selling * 3);
        }
    }
}

TEST_CASE("MU Volta event prices do not change unrelated items [items][economy]")
{
    CHECK_FALSE(GameLogic::Items::GetEventItemBuyingPrice(ITEM_JEWEL_OF_BLESS, 0).has_value());
    CHECK_FALSE(GameLogic::Items::GetEventItemBuyingPrice(ITEM_LARGE_HEALING_POTION, 0).has_value());
    CHECK_FALSE(GameLogic::Items::GetEventItemBuyingPrice(ITEM_SWORD, 7).has_value());
}
