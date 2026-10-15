#include "view/Renderer.hpp"

#include "core/Events.hpp"

#include <raylib.h>

namespace view {

Renderer::Renderer(core::EventBus& bus)
    : menuView_(std::make_unique<MenuView>(bus))
    , settingsView_(std::make_unique<SettingsView>(bus))
    , playingView_(std::make_unique<PlayingView>(bus))
    , debugOverlay_(std::make_unique<DebugOverlay>(bus))
{
    bus.subscribe<events::SceneChanged>([this](const events::SceneChanged& e) { scene_ = e.scene; });
}

void Renderer::render() const
{
    BeginDrawing();
    ClearBackground(Color{20, 20, 28, 255});

    switch (scene_) {
    case model::Scene::MainMenu:
        menuView_->draw();
        break;
    case model::Scene::Settings:
        settingsView_->draw();
        break;
    case model::Scene::Playing:
        playingView_->draw();
        break;
    }
    debugOverlay_->draw();

    EndDrawing();
}

} // namespace view
