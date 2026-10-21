#include "view/Renderer.hpp"

#include "core/Events.hpp"

#include <raylib.h>

namespace view {

Renderer::Renderer(core::EventBus& bus)
    : menuModule_(std::make_unique<menu::MenuModule>(bus))
    , playingView_(std::make_unique<gameplay::PlayingView>(bus))
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
    case model::Scene::Settings:
        menuModule_->draw(scene_);
        break;
    case model::Scene::Playing:
        playingView_->draw();
        break;
    }
    debugOverlay_->draw();

    EndDrawing();
}

} // namespace view
