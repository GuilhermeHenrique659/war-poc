#include "view/menu/SettingsView.hpp"

#include "core/Events.hpp"
#include "view/menu/SettingsLayout.hpp"
#include "view/ui/Button.hpp"

#include <raylib.h>

#include <array>
#include <string>

namespace view::menu {

namespace {

constexpr int kTitleSize = 42;
constexpr int kRowTextSize = 24;
constexpr std::array<const char*, 5> kRowLabels = {"Resolução", "Modo de tela", "Nome do jogador", "Aplicar", "Voltar"};
constexpr ui::ButtonStyle kSettingsButtonStyle{
    Color{45, 48, 58, 255},
    Color{70, 75, 90, 255},
    Color{36, 36, 44, 255},
    GRAY,
    GOLD,
    RAYWHITE,
    2,
};

} // namespace

SettingsView::SettingsView(core::EventBus& bus)
{
    bus.subscribe<events::SettingsStateChanged>([this](const events::SettingsStateChanged& state) {
        selectedIndex_ = state.selectedIndex;
        width_ = state.width;
        height_ = state.height;
        fullscreen_ = state.fullscreen;
        playerName_ = state.playerName;
    });
}

void SettingsView::draw() const
{
    constexpr const char* title = "Configurações";
    const int titleWidth = MeasureText(title, kTitleSize);
    DrawText(title, (GetScreenWidth() - titleWidth) / 2, 100, kTitleSize, RAYWHITE);

    for (std::size_t i = 0; i < settings_layout::kRowCount; ++i) {
        const Rectangle bounds = settings_layout::rowBounds(i, GetScreenWidth(), GetScreenHeight());
        const bool selected = i == selectedIndex_;

        std::string label = kRowLabels[i];
        if (i == 0) {
            label += "  <  " + std::to_string(width_) + " x " + std::to_string(height_) + "  >";
        } else if (i == 1) {
            label += fullscreen_ ? "  <  Tela cheia  >" : "  <  Janela  >";
        } else if (i == 2) {
            label += "  " + (playerName_.empty() ? std::string{"Jogador"} : playerName_);
        }
        const auto state = selected ? ui::ButtonState::Selected : ui::ButtonState::Normal;
        ui::Button{kSettingsButtonStyle}.draw(bounds, label, state, kRowTextSize);
    }

    constexpr const char* hint = "Setas: navegar/alterar  |  Digite o nome quando selecionado  |  Esc: voltar";
    const int hintWidth = MeasureText(hint, 18);
    DrawText(hint, (GetScreenWidth() - hintWidth) / 2, GetScreenHeight() - 58, 18, LIGHTGRAY);
}

} // namespace view::menu
