#pragma once

#include <filesystem>
#include <string_view>

namespace view {

// Directory that contains the running executable. All runtime assets are resolved from here,
// never from the current working directory.
[[nodiscard]] std::filesystem::path executableDirectory();

// Resolves a path inside resource/ next to the executable.
[[nodiscard]] std::filesystem::path resourcePath(std::string_view relative);

} // namespace view
