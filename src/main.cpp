#include "core/Game.hpp"

#include <memory>

int main()
{
    const auto game = std::make_unique<core::Game>();
    game->run();
    return 0;
}
