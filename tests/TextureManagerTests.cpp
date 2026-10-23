#include "raylib_win.h"
#include "view/TextureManager.hpp"

#include <gtest/gtest.h>
#include <raylib.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

std::string loadedExtension;
std::vector<unsigned char> loadedBytes;
std::vector<unsigned int> unloadedTextures;
Rectangle croppedSource{};
int resizedWidth = 0;
int resizedHeight = 0;
int uploadedWidth = 0;
int uploadedHeight = 0;
unsigned int nextTextureId = 1;

std::filesystem::path executableDirectory()
{
    std::vector<wchar_t> path(260);
    const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    return std::filesystem::path(std::wstring(path.data(), length)).parent_path();
}

} // namespace

Image LoadImageFromMemory(const char* fileType, const unsigned char* fileData, int dataSize)
{
    loadedExtension = fileType;
    loadedBytes.assign(fileData, fileData + dataSize);
    return Image{reinterpret_cast<void*>(1), 1774, 887, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
}

Texture2D LoadTextureFromImage(Image image)
{
    uploadedWidth = image.width;
    uploadedHeight = image.height;
    return Texture2D{nextTextureId++, image.width, image.height, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
}

void UnloadImage(Image)
{
}

void ImageCrop(Image* image, Rectangle crop)
{
    croppedSource = crop;
    image->width = static_cast<int>(crop.width);
    image->height = static_cast<int>(crop.height);
}

void ImageResize(Image* image, int width, int height)
{
    resizedWidth = width;
    resizedHeight = height;
    image->width = width;
    image->height = height;
}

void UnloadTexture(Texture2D texture)
{
    unloadedTextures.push_back(texture.id);
}

void SetTextureFilter(Texture2D, int)
{
}

namespace view {
namespace {

class TextureManagerTests : public ::testing::Test {
protected:
    void SetUp() override
    {
        TextureManager::instance().clear();
        loadedExtension.clear();
        loadedBytes.clear();
        unloadedTextures.clear();
        croppedSource = {};
        resizedWidth = 0;
        resizedHeight = 0;
        uploadedWidth = 0;
        uploadedHeight = 0;
        nextTextureId = 1;
    }

    void TearDown() override
    {
        TextureManager::instance().clear();
    }
};

TEST_F(TextureManagerTests, StoresAndGetsTextureByKey)
{
    auto& manager = TextureManager::instance();
    const Texture2D texture{42, 32, 24, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};

    manager.add("units/scout", texture);

    EXPECT_EQ(manager.get("units/scout").id, texture.id);
}

TEST_F(TextureManagerTests, LoadsTextureRelativeToExecutableDirectory)
{
    auto& manager = TextureManager::instance();
    const std::filesystem::path path = executableDirectory() / "texture_manager_test.png";
    {
        std::ofstream file(path, std::ios::binary);
        file << "png test data";
    }

    manager.load("ui/button", "texture_manager_test.png");

    std::filesystem::remove(path);
    EXPECT_EQ(loadedExtension, ".png");
    EXPECT_EQ(loadedBytes, std::vector<unsigned char>({'p', 'n', 'g', ' ', 't', 'e', 's', 't', ' ', 'd', 'a', 't', 'a'}));
    EXPECT_EQ(manager.get("ui/button").id, 1U);
}

TEST_F(TextureManagerTests, ReusesTextureWhenLoadingAnAlreadyRegisteredKey)
{
    auto& manager = TextureManager::instance();
    const std::filesystem::path path = executableDirectory() / "texture_manager_reused_test.png";
    {
        std::ofstream file(path, std::ios::binary);
        file << "png test data";
    }

    manager.load("units/drone", "texture_manager_reused_test.png");
    const unsigned int textureId = manager.get("units/drone").id;
    manager.load("units/drone", "texture_manager_reused_test.png");

    std::filesystem::remove(path);
    EXPECT_EQ(manager.get("units/drone").id, textureId);
    EXPECT_TRUE(unloadedTextures.empty());
}

TEST_F(TextureManagerTests, CropsAndResizesTextureBeforeUploading)
{
    auto& manager = TextureManager::instance();
    const std::filesystem::path path = executableDirectory() / "texture_manager_resized_test.png";
    {
        std::ofstream file(path, std::ios::binary);
        file << "png test data";
    }

    manager.loadResized("terrain/tile", "texture_manager_resized_test.png",
        Rectangle{84.0f, 49.0f, 1606.0f, 789.0f}, 128, 64);

    std::filesystem::remove(path);
    EXPECT_EQ(croppedSource.x, 84.0f);
    EXPECT_EQ(croppedSource.y, 49.0f);
    EXPECT_EQ(croppedSource.width, 1606.0f);
    EXPECT_EQ(croppedSource.height, 789.0f);
    EXPECT_EQ(resizedWidth, 128);
    EXPECT_EQ(resizedHeight, 64);
    EXPECT_EQ(uploadedWidth, 128);
    EXPECT_EQ(uploadedHeight, 64);
    EXPECT_EQ(manager.get("terrain/tile").id, 1U);
    EXPECT_EQ(manager.get("terrain/tile").width, 128);
    EXPECT_EQ(manager.get("terrain/tile").height, 64);
}

TEST_F(TextureManagerTests, ReplacingKeyUnloadsPreviousTexture)
{
    auto& manager = TextureManager::instance();
    manager.add("units/scout", Texture2D{7, 16, 16, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8});

    manager.add("units/scout", Texture2D{8, 32, 32, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8});

    ASSERT_EQ(unloadedTextures.size(), 1U);
    EXPECT_EQ(unloadedTextures.front(), 7U);
    EXPECT_EQ(manager.get("units/scout").id, 8U);
}

TEST_F(TextureManagerTests, MissingKeyThrows)
{
    EXPECT_THROW(TextureManager::instance().get("missing"), std::out_of_range);
}

TEST_F(TextureManagerTests, ClearUnloadsRegisteredTextures)
{
    auto& manager = TextureManager::instance();
    manager.add("units/scout", Texture2D{9, 16, 16, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8});

    manager.clear();

    ASSERT_EQ(unloadedTextures.size(), 1U);
    EXPECT_EQ(unloadedTextures.front(), 9U);
}

} // namespace
} // namespace view
