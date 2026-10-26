#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/Scene.hpp"
#include "model/menu/MenuAction.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace model {

// Owns menu and settings state. Match state belongs to PlayingModel.
class MenuModel {
public:
    explicit MenuModel(core::EventBus& bus);

    void initialize();

private:
    void onSceneChanged(const events::SceneChanged& event);
    void onMenuNavigate(const events::MenuNavigateRequested& event);
    void onMenuItemHovered(const events::MenuItemHovered& event);
    void onMenuConfirm(const events::MenuConfirmRequested& event);
    void onSettingsNavigate(const events::SettingsNavigateRequested& event);
    void onSettingsAdjust(const events::SettingsAdjustRequested& event);
    void onSettingsNameInput(const events::SettingsNameInputRequested& event);
    void onSettingsNameBackspace(const events::SettingsNameBackspaceRequested& event);
    void onSettingsRowHovered(const events::SettingsRowHovered& event);
    void onSettingsConfirm(const events::SettingsConfirmRequested& event);
    void onSettingsBack(const events::SettingsBackRequested& event);

    void changeScene(Scene scene);
    void publishMenuState() const;
    void publishSettingsState() const;

    core::EventBus& bus_;
    Scene scene_ = Scene::MainMenu;
    std::vector<MenuAction> menuItems_;
    std::size_t selectedIndex_ = 0;
    std::size_t selectedSettingsIndex_ = 0;
    std::size_t resolutionIndex_ = 1;
    bool fullscreen_ = false;
    std::string playerName_ = "Jogador";
};

} // namespace model
