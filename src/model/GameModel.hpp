#pragma once

#include "core/EventBus.hpp"
#include "model/MenuModel.hpp"
#include "model/PlayingModel.hpp"

#include <memory>

namespace model {

// Composition point for scene-specific models. They exchange commands only through the bus.
class GameModel {
public:
    explicit GameModel(core::EventBus& bus);

    // Publishes the initial state; call after every listener has subscribed.
    void initialize();
    void update(float deltaSeconds);

private:
    std::unique_ptr<MenuModel> menu_;
    std::unique_ptr<PlayingModel> playing_;
};

} // namespace model
