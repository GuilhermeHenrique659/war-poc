#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/menu/MenuModel.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

class MenuModelTest : public ::testing::Test {
protected:
    MenuModelTest()
        : menu_(bus_)
    {
        bus_.subscribe<events::WindowConfigurationRequested>([this](const auto& event) {
            configurations_.push_back(event);
        });
        bus_.subscribe<events::StartMatchRequested>([this](const auto& event) {
            starts_.push_back(event);
        });
    }

    core::EventBus bus_;
    model::MenuModel menu_;
    std::vector<events::WindowConfigurationRequested> configurations_;
    std::vector<events::StartMatchRequested> starts_;
};

} // namespace

TEST_F(MenuModelTest, AppliesTheSelectedResolutionAndFullscreenModeOnlyOnConfirmation)
{
    // Given the main menu and its default settings.
    menu_.initialize();

    // When the player enters settings, changes resolution and fullscreen, then confirms Apply.
    bus_.publish(events::MenuNavigateRequested{events::MenuDirection::Down});
    bus_.publish(events::MenuConfirmRequested{});
    bus_.publish(events::SettingsAdjustRequested{1});
    bus_.publish(events::SettingsNavigateRequested{events::MenuDirection::Down});
    bus_.publish(events::SettingsAdjustRequested{1});
    bus_.publish(events::SettingsNavigateRequested{events::MenuDirection::Down});
    bus_.publish(events::SettingsNavigateRequested{events::MenuDirection::Down});
    bus_.publish(events::SettingsConfirmRequested{});

    // Then the chosen 1600 by 900 fullscreen configuration is requested once.
    ASSERT_EQ(configurations_.size(), 1U);
    const events::WindowConfigurationRequested configuration = configurations_.front();
    EXPECT_EQ(configuration.width, 1600);
    EXPECT_EQ(configuration.height, 900);
    EXPECT_TRUE(configuration.fullscreen);
}

TEST_F(MenuModelTest, StartsAMatchWithTheDefaultPlayerName)
{
    // Given the initial menu selection on Start.
    menu_.initialize();

    // When the player confirms it.
    bus_.publish(events::MenuConfirmRequested{});

    // Then the match starts with the configured default player name.
    ASSERT_EQ(starts_.size(), 1U);
    EXPECT_EQ(starts_.front().playerName, "Jogador");
}
