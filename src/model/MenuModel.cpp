#include "model/MenuModel.hpp"

#include <algorithm>

namespace model {

namespace {

struct Resolution {
    int width;
    int height;
};

constexpr Resolution kResolutions[] = {{1024, 576}, {1280, 720}, {1600, 900}, {1920, 1080}};
constexpr std::size_t kSettingsRowCount = 5;
constexpr std::size_t kPlayerNameRow = 2;
constexpr std::size_t kApplyRow = 3;
constexpr std::size_t kBackRow = 4;
constexpr std::size_t kMaxPlayerNameBytes = 32;
constexpr const char* kDefaultPlayerName = "Jogador";

} // namespace

MenuModel::MenuModel(core::EventBus& bus)
    : bus_(bus)
    , menuItems_{{"Iniciar", MenuAction::StartGame}, {"Configurações", MenuAction::Settings}, {"Sair", MenuAction::Quit}}
{
    bus_.subscribe<events::SceneChanged>([this](const auto& e) { onSceneChanged(e); });
    bus_.subscribe<events::MenuNavigateRequested>([this](const auto& e) { onMenuNavigate(e); });
    bus_.subscribe<events::MenuItemHovered>([this](const auto& e) { onMenuItemHovered(e); });
    bus_.subscribe<events::MenuConfirmRequested>([this](const auto& e) { onMenuConfirm(e); });
    bus_.subscribe<events::SettingsNavigateRequested>([this](const auto& e) { onSettingsNavigate(e); });
    bus_.subscribe<events::SettingsAdjustRequested>([this](const auto& e) { onSettingsAdjust(e); });
    bus_.subscribe<events::SettingsNameInputRequested>([this](const auto& e) { onSettingsNameInput(e); });
    bus_.subscribe<events::SettingsNameBackspaceRequested>([this](const auto& e) { onSettingsNameBackspace(e); });
    bus_.subscribe<events::SettingsRowHovered>([this](const auto& e) { onSettingsRowHovered(e); });
    bus_.subscribe<events::SettingsConfirmRequested>([this](const auto& e) { onSettingsConfirm(e); });
    bus_.subscribe<events::SettingsBackRequested>([this](const auto& e) { onSettingsBack(e); });
}

void MenuModel::initialize()
{
    changeScene(Scene::MainMenu);
}

void MenuModel::onSceneChanged(const events::SceneChanged& event)
{
    scene_ = event.scene;
    if (scene_ == Scene::MainMenu) {
        publishMenuState();
    }
}

void MenuModel::onMenuNavigate(const events::MenuNavigateRequested& event)
{
    if (scene_ != Scene::MainMenu || menuItems_.empty()) {
        return;
    }
    const std::size_t count = menuItems_.size();
    selectedIndex_ = event.direction == events::MenuDirection::Down
        ? (selectedIndex_ + 1) % count
        : (selectedIndex_ + count - 1) % count;
    publishMenuState();
}

void MenuModel::onMenuItemHovered(const events::MenuItemHovered& event)
{
    if (scene_ != Scene::MainMenu || event.index >= menuItems_.size() || event.index == selectedIndex_) {
        return;
    }
    selectedIndex_ = event.index;
    publishMenuState();
}

void MenuModel::onMenuConfirm(const events::MenuConfirmRequested& /*event*/)
{
    if (scene_ != Scene::MainMenu || selectedIndex_ >= menuItems_.size()) {
        return;
    }
    switch (menuItems_[selectedIndex_].action) {
    case MenuAction::StartGame:
        bus_.publish(events::StartMatchRequested{playerName_.empty() ? kDefaultPlayerName : playerName_});
        return;
    case MenuAction::Settings:
        selectedSettingsIndex_ = 0;
        changeScene(Scene::Settings);
        publishSettingsState();
        return;
    case MenuAction::Quit:
        bus_.publish(events::QuitRequested{});
        return;
    }
}

void MenuModel::onSettingsNavigate(const events::SettingsNavigateRequested& event)
{
    if (scene_ != Scene::Settings) {
        return;
    }
    selectedSettingsIndex_ = event.direction == events::MenuDirection::Down
        ? (selectedSettingsIndex_ + 1) % kSettingsRowCount
        : (selectedSettingsIndex_ + kSettingsRowCount - 1) % kSettingsRowCount;
    publishSettingsState();
}

void MenuModel::onSettingsAdjust(const events::SettingsAdjustRequested& event)
{
    if (scene_ != Scene::Settings || event.delta == 0) {
        return;
    }
    if (selectedSettingsIndex_ == 0) {
        const auto count = std::size(kResolutions);
        const auto current = static_cast<int>(resolutionIndex_);
        const auto next = (current + event.delta + static_cast<int>(count)) % static_cast<int>(count);
        resolutionIndex_ = static_cast<std::size_t>(next);
    } else if (selectedSettingsIndex_ == 1) {
        fullscreen_ = !fullscreen_;
    }
    publishSettingsState();
}

void MenuModel::onSettingsNameInput(const events::SettingsNameInputRequested& event)
{
    if (scene_ != Scene::Settings || selectedSettingsIndex_ != kPlayerNameRow || event.utf8Text.empty()) {
        return;
    }
    const std::size_t available = kMaxPlayerNameBytes - std::min(playerName_.size(), kMaxPlayerNameBytes);
    playerName_.append(event.utf8Text.substr(0, available));
    publishSettingsState();
}

void MenuModel::onSettingsNameBackspace(const events::SettingsNameBackspaceRequested& /*event*/)
{
    if (scene_ != Scene::Settings || selectedSettingsIndex_ != kPlayerNameRow || playerName_.empty()) {
        return;
    }
    const auto begin = playerName_.rbegin();
    const auto character = std::find_if(begin, playerName_.rend(), [](unsigned char byte) {
        return (byte & 0xC0U) != 0x80U;
    });
    playerName_.erase(playerName_.size() - static_cast<std::size_t>(std::distance(begin, character)) - 1);
    publishSettingsState();
}

void MenuModel::onSettingsRowHovered(const events::SettingsRowHovered& event)
{
    if (scene_ != Scene::Settings || event.index >= kSettingsRowCount || event.index == selectedSettingsIndex_) {
        return;
    }
    selectedSettingsIndex_ = event.index;
    publishSettingsState();
}

void MenuModel::onSettingsConfirm(const events::SettingsConfirmRequested& /*event*/)
{
    if (scene_ != Scene::Settings) {
        return;
    }
    if (selectedSettingsIndex_ == kApplyRow) {
        const auto& resolution = kResolutions[resolutionIndex_];
        bus_.publish(events::WindowConfigurationRequested{resolution.width, resolution.height, fullscreen_});
        return;
    }
    if (selectedSettingsIndex_ == kBackRow) {
        changeScene(Scene::MainMenu);
    }
}

void MenuModel::onSettingsBack(const events::SettingsBackRequested& /*event*/)
{
    if (scene_ == Scene::Settings) {
        changeScene(Scene::MainMenu);
    }
}

void MenuModel::changeScene(Scene scene)
{
    scene_ = scene;
    bus_.publish(events::SceneChanged{scene_});
}

void MenuModel::publishMenuState() const
{
    events::MenuStateChanged state{{}, selectedIndex_};
    state.labels.reserve(menuItems_.size());
    for (const auto& item : menuItems_) {
        state.labels.push_back(item.label);
    }
    bus_.publish(state);
}

void MenuModel::publishSettingsState() const
{
    const auto& resolution = kResolutions[resolutionIndex_];
    bus_.publish(events::SettingsStateChanged{selectedSettingsIndex_, resolution.width, resolution.height, fullscreen_, playerName_});
}

} // namespace model
