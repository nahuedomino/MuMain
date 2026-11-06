#pragma once

namespace Render::Effects
{

[[nodiscard]] constexpr bool ShouldKeepAuraJointAlive(bool hasDefense, bool hasHelpNpc)
{
    return hasDefense || hasHelpNpc;
}

} // namespace Render::Effects
