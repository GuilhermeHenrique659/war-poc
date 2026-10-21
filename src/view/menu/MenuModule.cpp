#include "view/menu/MenuModule.hpp"

namespace view::menu {

MenuModule::MenuModule(core::EventBus& bus)
    : menuView_(bus)
    , settingsView_(bus)
{
}

void MenuModule::draw(model::Scene scene) const
{
    switch (scene) {
    case model::Scene::MainMenu:
        menuView_.draw();
        break;
    case model::Scene::Settings:
        settingsView_.draw();
        break;
    case model::Scene::Playing:
        break;
    }
}

} // namespace view::menu
