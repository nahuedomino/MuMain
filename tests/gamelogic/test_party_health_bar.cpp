// Tests of the health bar over party members: fill from the server's 10% steps, color bands,
// the lighter top edge and the easing toward a new value.

#include "stdafx.h"

#include "doctest.h"

#include "GameLogic/Social/PartyHealthBar.h"

namespace Bar = GameLogic::PartyHealthBar;

TEST_CASE("party health bar fill follows the health steps and stays within 0 and 1")
{
    CHECK(Bar::HealthFractionFromSteps(0) == doctest::Approx(0.f));
    CHECK(Bar::HealthFractionFromSteps(7) == doctest::Approx(0.7f));
    CHECK(Bar::HealthFractionFromSteps(10) == doctest::Approx(1.f));
    CHECK(Bar::HealthFractionFromSteps(15) == doctest::Approx(1.f));
    CHECK(Bar::HealthFractionFromSteps(-1) == doctest::Approx(0.f));
}

TEST_CASE("party health bar is green above half, amber down to a quarter and red below")
{
    CHECK(Bar::FillColor(1.f) == Bar::HighHealthColor);
    CHECK(Bar::FillColor(0.6f) == Bar::HighHealthColor);
    CHECK(Bar::FillColor(0.5f) == Bar::MediumHealthColor);
    CHECK(Bar::FillColor(0.3f) == Bar::MediumHealthColor);
    CHECK(Bar::FillColor(0.25f) == Bar::LowHealthColor);
    CHECK(Bar::FillColor(0.f) == Bar::LowHealthColor);
}

TEST_CASE("party health bar highlight mixes toward white and keeps the alpha")
{
    CHECK(Bar::Highlight(0x80000000u, 0.f) == 0x80000000u);
    CHECK(Bar::Highlight(0x80000000u, 1.f) == 0x80FFFFFFu);
    CHECK(Bar::Highlight(0xFF000000u, 0.5f) == 0xFF808080u);
}

TEST_CASE("party health bar eases toward the reported fill without overshooting")
{
    CHECK(Bar::EaseToward(1.f, 0.f, 0.1f, 2.f) == doctest::Approx(0.8f));
    CHECK(Bar::EaseToward(0.f, 1.f, 0.1f, 2.f) == doctest::Approx(0.2f));
    CHECK(Bar::EaseToward(0.5f, 0.6f, 0.1f, 2.f) == doctest::Approx(0.6f));
    CHECK(Bar::EaseToward(0.5f, 0.f, 0.f, 2.f) == doctest::Approx(0.5f));
}
