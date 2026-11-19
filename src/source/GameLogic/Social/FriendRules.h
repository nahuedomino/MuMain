#pragma once

#include <string_view>

namespace GameLogic::Social
{
    // Whether a name typed or selected to add as a friend is the player's own character (case doesn't matter, as for
    // character names on the server). Neither the client nor OpenMU refused it (sven-n/MuMain#887).
    [[nodiscard]] bool IsOwnCharacterName(std::wstring_view name, std::wstring_view ownName);
}
