#include "core/Game.hpp"

#include "core/Events.hpp"
#include "view/TextureManager.hpp"

#include <raylib.h>

namespace core {

namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
constexpr const char* kWindowTitle = "War POC";

} // namespace

Game::Game()
    : bus_(std::make_unique<EventBus>())
    , window_(std::make_unique<view::Window>(kWindowWidth, kWindowHeight, kWindowTitle))
    , model_(std::make_unique<model::GameModel>(*bus_))
    , renderer_(std::make_unique<view::Renderer>(*bus_))
    , controller_(std::make_unique<controller::InputController>(*bus_))
{
    bus_->subscribe<events::QuitRequested>([this](const events::QuitRequested&) { running_ = false; });
    bus_->subscribe<events::WindowConfigurationRequested>([this](const events::WindowConfigurationRequested& e) {
        window_->applyDisplayConfiguration(e.width, e.height, e.fullscreen);
    });
    model_->initialize();
}

Game::~Game()
{
    controller_.reset();
    renderer_.reset();
    model_.reset();
    view::TextureManager::instance().clear();
}

void Game::run()
{
    while (running_) {
        controller_->update();
        if (!running_) {
            return;
        }
        model_->update(GetFrameTime());
        renderer_->render();
    }
}

} // namespace core
