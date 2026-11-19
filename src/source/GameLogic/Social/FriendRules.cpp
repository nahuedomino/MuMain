#include "stdafx.h"
#include "GameLogic/Social/FriendRules.h"

#include <cwctype>

namespace GameLogic::Social
{
    bool IsOwnCharacterName(std::wstring_view name, std::wstring_view ownName)
    {
        if (name.empty() || name.size() != ownName.size())
        {
            return false;
        }

        for (size_t i = 0; i < name.size(); ++i)
        {
            if (std::towlower(name[i]) != std::towlower(ownName[i]))
            {
                return false;
            }
        }

        return true;
    }
}
