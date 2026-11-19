#include "stdafx.h"
#include "GameLogic/Automation/Repair.h"

#include <cstdint>

namespace GameLogic::Automation
{
    bool ShouldRepairEquipment(bool repairBanned, bool ammunition, int durability, int maxDurability,
                               int thresholdPercent)
    {
        if (repairBanned || ammunition || maxDurability <= 0)
        {
            return false;
        }

        const int64_t health = (static_cast<int64_t>(durability) * 100 + maxDurability - 1) / maxDurability;
        return health <= thresholdPercent;
    }
}
