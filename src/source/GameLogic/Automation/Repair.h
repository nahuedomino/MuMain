#pragma once

namespace GameLogic::Automation
{
    // Whether the helper asks the server to repair an equipped item: not when the item data bans repairing it, not
    // for arrows and bolts (their durability is the number of shots; sven-n/MuMain#913), and only when its health,
    // durability in percent of the maximum rounded up, is at or below the threshold.
    [[nodiscard]] bool ShouldRepairEquipment(bool repairBanned, bool ammunition, int durability, int maxDurability,
                                             int thresholdPercent);
}
