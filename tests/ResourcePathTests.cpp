#include "raylib_win.h"
#include "view/ResourcePath.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

namespace {

std::filesystem::path testExecutableDirectory()
{
    std::vector<wchar_t> path(260);
    const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    return std::filesystem::path(std::wstring(path.data(), length)).parent_path();
}

} // namespace

TEST(ResourcePathTests, ExecutableDirectoryIsTheDirectoryOfTheRunningTestBinary)
{
    // When the executable directory is asked for.
    const std::filesystem::path directory = view::executableDirectory();

    // Then it is absolute and matches the running binary.
    EXPECT_TRUE(directory.is_absolute());
    EXPECT_EQ(directory, testExecutableDirectory());
}

TEST(ResourcePathTests, ResourcePathIsResolvedFromTheExecutableDirectory)
{
    // When a resource path is built.
    const std::filesystem::path path = view::resourcePath("texts/pt_br.txt");

    // Then it points at resource/ next to the executable.
    EXPECT_TRUE(path.is_absolute());
    EXPECT_EQ(path, testExecutableDirectory() / "resource" / "texts" / "pt_br.txt");
}
