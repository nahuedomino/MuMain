#include "stdafx.h"
#include "doctest.h"

#include "GameLogic/Social/FriendRules.h"

TEST_CASE("the own character can't be added as a friend, in any letter case [gamelogic][social][friends]")
{
    using GameLogic::Social::IsOwnCharacterName;
    // sven-n/MuMain#887: adding yourself sent the request and showed your own friend request.
    CHECK(IsOwnCharacterName(L"VoltaElf", L"VoltaElf"));
    CHECK(IsOwnCharacterName(L"voltaelf", L"VoltaElf"));
    CHECK_FALSE(IsOwnCharacterName(L"VoltaElf2", L"VoltaElf"));
    CHECK_FALSE(IsOwnCharacterName(L"Volta", L"VoltaElf"));
    CHECK_FALSE(IsOwnCharacterName(L"", L""));
}
