#pragma once

namespace model {

// Main menu entries. Lives in its own header so events can carry the action
// without depending on MenuModel.
enum class MenuAction { StartGame, Settings, Quit };

} // namespace model
