#include "model/GameModel.hpp"

#include "model/components/PlayerComponents.hpp"

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

GameModel::GameModel(core::EventBus& bus)
    : bus_(bus)
    , menuItems_{{"Iniciar", MenuAction::StartGame}, {"Configurações", MenuAction::Settings}, {"Sair", MenuAction::Quit}}
    , world_(std::make_unique<ecs::Registry>())
    , buildings_(std::make_unique<BuildingSystem>(bus_, *world_))
    , units_(std::make_unique<UnitSystem>(bus_, *world_, *buildings_))
    , selection_(std::make_unique<SelectionSystem>(bus_, *world_))
    , merges_(std::make_unique<MergeSystem>(bus_, *world_, *units_, *selection_))
{
    buildings_->setTileBlocker([this](GridPos tile) { return units_->isTileTaken(tile); });

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
    bus_.subscribe<events::ReturnToMenuRequested>([this](const auto& e) { onReturnToMenu(e); });
    subscribeBuildCommands();
    subscribeUnitCommands();
}

void GameModel::subscribeUnitCommands()
{
    bus_.subscribe<events::RecruitRequested>([this](const events::RecruitRequested& e) {
        if (!inMatch()) {
            return;
        }
        if (const auto commandCenter = selection_->selectedCommandCenter(e.player)) {
            units_->recruit(e.player, e.type, *commandCenter);
        }
    });
    bus_.subscribe<events::SelectUnitsRequested>([this](const events::SelectUnitsRequested& e) {
        if (inMatch()) {
            selection_->selectUnits(e.player, e.units, e.additive);
            merges_->refresh(e.player);
        }
    });
    bus_.subscribe<events::SelectBuildingRequested>([this](const events::SelectBuildingRequested& e) {
        if (inMatch()) {
            selection_->selectBuilding(e.player, e.building);
            merges_->refresh(e.player);
        }
    });
    bus_.subscribe<events::SelectionClearRequested>([this](const events::SelectionClearRequested& e) {
        if (inMatch()) {
            selection_->clear(e.player);
            merges_->refresh(e.player);
        }
    });
    bus_.subscribe<events::MoveOrderRequested>([this](const events::MoveOrderRequested& e) {
        if (inMatch()) {
            units_->orderMove(e.player, selection_->selectedUnits(e.player), e.tile);
        }
    });
    bus_.subscribe<events::MergeRequested>([this](const events::MergeRequested& e) {
        if (inMatch()) {
            merges_->merge(e.player);
        }
    });
}

void GameModel::subscribeBuildCommands()
{
    bus_.subscribe<events::BuildSelectRequested>([this](const events::BuildSelectRequested& e) {
        if (inMatch()) {
            buildings_->select(e.player, e.type);
        }
    });
    bus_.subscribe<events::BuildCancelRequested>([this](const events::BuildCancelRequested& e) {
        if (inMatch()) {
            buildings_->cancel(e.player);
        }
    });
    bus_.subscribe<events::BuildCursorMoved>([this](const events::BuildCursorMoved& e) {
        if (inMatch()) {
            buildings_->moveCursor(e.player, e.tile);
        }
    });
    bus_.subscribe<events::BuildPlaceRequested>([this](const events::BuildPlaceRequested& e) {
        if (inMatch()) {
            buildings_->place(e.player, e.tile);
        }
    });
}

void GameModel::initialize()
{
    changeScene(Scene::MainMenu);
    publishMenuState();
}

void GameModel::update(float deltaSeconds)
{
    if (scene_ != Scene::Playing) {
        return;
    }
    // Moving units can make a placement preview (in)valid.
    if (units_->update(deltaSeconds)) {
        buildings_->refreshPreviews();
    }
    // Merge validity depends on positions and free space; it only republishes on change.
    merges_->refreshAll();
}

void GameModel::onMenuNavigate(const events::MenuNavigateRequested& event)
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

void GameModel::onMenuItemHovered(const events::MenuItemHovered& event)
{
    if (scene_ != Scene::MainMenu || event.index >= menuItems_.size()) {
        return;
    }
    if (event.index == selectedIndex_) {
        return;
    }

    selectedIndex_ = event.index;
    publishMenuState();
}

void GameModel::onMenuConfirm(const events::MenuConfirmRequested& /*event*/)
{
    if (scene_ != Scene::MainMenu || selectedIndex_ >= menuItems_.size()) {
        return;
    }

    switch (menuItems_[selectedIndex_].action) {
    case MenuAction::StartGame:
        startMatch();
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

void GameModel::onSettingsNavigate(const events::SettingsNavigateRequested& event)
{
    if (scene_ != Scene::Settings) {
        return;
    }
    selectedSettingsIndex_ = event.direction == events::MenuDirection::Down
        ? (selectedSettingsIndex_ + 1) % kSettingsRowCount
        : (selectedSettingsIndex_ + kSettingsRowCount - 1) % kSettingsRowCount;
    publishSettingsState();
}

void GameModel::onSettingsAdjust(const events::SettingsAdjustRequested& event)
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

void GameModel::onSettingsNameInput(const events::SettingsNameInputRequested& event)
{
    if (scene_ != Scene::Settings || selectedSettingsIndex_ != kPlayerNameRow || event.utf8Text.empty()) {
        return;
    }
    const std::size_t available = kMaxPlayerNameBytes - std::min(playerName_.size(), kMaxPlayerNameBytes);
    playerName_.append(event.utf8Text.substr(0, available));
    publishSettingsState();
}

void GameModel::onSettingsNameBackspace(const events::SettingsNameBackspaceRequested& /*event*/)
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

void GameModel::onSettingsRowHovered(const events::SettingsRowHovered& event)
{
    if (scene_ != Scene::Settings || event.index >= kSettingsRowCount || event.index == selectedSettingsIndex_) {
        return;
    }
    selectedSettingsIndex_ = event.index;
    publishSettingsState();
}

void GameModel::onSettingsConfirm(const events::SettingsConfirmRequested& /*event*/)
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
        publishMenuState();
    }
}

void GameModel::onSettingsBack(const events::SettingsBackRequested& /*event*/)
{
    if (scene_ != Scene::Settings) {
        return;
    }
    changeScene(Scene::MainMenu);
    publishMenuState();
}

void GameModel::onReturnToMenu(const events::ReturnToMenuRequested& /*event*/)
{
    if (scene_ != Scene::Playing) {
        return;
    }

    resetWorld();
    changeScene(Scene::MainMenu);
    publishMenuState();
}

void GameModel::changeScene(Scene scene)
{
    scene_ = scene;
    bus_.publish(events::SceneChanged{scene_});
}

void GameModel::publishMenuState() const
{
    events::MenuStateChanged state{{}, selectedIndex_};
    state.labels.reserve(menuItems_.size());
    for (const auto& item : menuItems_) {
        state.labels.push_back(item.label);
    }
    bus_.publish(state);
}

void GameModel::publishSettingsState() const
{
    const auto& resolution = kResolutions[resolutionIndex_];
    bus_.publish(events::SettingsStateChanged{selectedSettingsIndex_, resolution.width, resolution.height, fullscreen_, playerName_});
}

void GameModel::resetWorld()
{
    world_->clear();
    bus_.publish(events::WorldReset{});
    buildings_->reset();
    units_->reset();
    selection_->reset();
    merges_->reset();
}

void GameModel::startMatch()
{
    resetWorld();
    createLocalPlayer();
    changeScene(Scene::Playing);
}

void GameModel::createLocalPlayer()
{
    const ecs::Entity entity = world_->create();
    localPlayerId_ = nextPlayerId_++;
    world_->emplace<components::PlayerProfile>(entity, localPlayerId_, playerName_.empty() ? kDefaultPlayerName : playerName_);
    bus_.publish(events::LocalPlayerChanged{localPlayerId_});
    buildings_->initializePlayer(localPlayerId_);
    selection_->initializePlayer(localPlayerId_);
    merges_->initializePlayer(localPlayerId_);
}

} // namespace model
