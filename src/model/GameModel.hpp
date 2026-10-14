#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/BuildingSystem.hpp"
#include "model/Scene.hpp"
#include "model/ecs/Registry.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace model {

enum class MenuAction { StartGame, Quit };

struct MenuItem {
    std::string label;
    MenuAction action;
};

// Owns game state. Reacts to commands from the bus and publishes state changes.
class GameModel {
public:
    explicit GameModel(core::EventBus& bus);

    // Publishes the initial state; call after every listener has subscribed.
    void initialize();
    void update(float deltaSeconds);

private:
    void onMenuNavigate(const events::MenuNavigateRequested& event);
    void onMenuItemHovered(const events::MenuItemHovered& event);
    void onMenuConfirm(const events::MenuConfirmRequested& event);
    void onReturnToMenu(const events::ReturnToMenuRequested& event);

    void subscribeBuildCommands();
    [[nodiscard]] bool inMatch() const { return scene_ == Scene::Playing; }

    void changeScene(Scene scene);
    void publishMenuState() const;
    void resetWorld();
    void startMatch();

    core::EventBus& bus_;
    Scene scene_ = Scene::MainMenu;
    std::vector<MenuItem> menuItems_;
    std::size_t selectedIndex_ = 0;
    std::unique_ptr<ecs::Registry> world_;
    std::unique_ptr<BuildingSystem> buildings_; // Holds a reference to world_; keep it declared after.
};

} // namespace model
