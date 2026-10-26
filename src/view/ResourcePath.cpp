#if defined(_WIN32)
#include "raylib_win.h"
#endif

#include "view/ResourcePath.hpp"

#include <raylib.h>

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace view {

std::filesystem::path executableDirectory()
{
#if defined(_WIN32)
    constexpr std::size_t kInitialPathCapacity = 260;
    constexpr std::size_t kMaximumPathCapacity = 32768;
    std::vector<wchar_t> path(kInitialPathCapacity);

    while (true) {
        const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (length == 0) {
            throw std::runtime_error("Failed to get executable path");
        }
        if (length < path.size()) {
            return std::filesystem::path(std::wstring(path.data(), length)).parent_path();
        }
        if (path.size() >= kMaximumPathCapacity) {
            throw std::runtime_error("Executable path exceeds the supported Windows path length");
        }
        path.resize(std::min(path.size() * 2, kMaximumPathCapacity));
    }
#else
    return std::filesystem::path(GetApplicationDirectory());
#endif
}

std::filesystem::path resourcePath(std::string_view relative)
{
    return executableDirectory() / "resource" / std::filesystem::path(relative);
}

} // namespace view
