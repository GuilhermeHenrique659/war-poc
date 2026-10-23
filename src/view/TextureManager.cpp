#if defined(_WIN32)
#include "raylib_win.h"
#endif

#include "view/TextureManager.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace view {

namespace {

[[nodiscard]] std::filesystem::path executableDirectory()
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

[[nodiscard]] std::filesystem::path utf8Path(const std::string& value)
{
    std::u8string encodedPath(value.size(), u8'\0');
    std::transform(value.begin(), value.end(), encodedPath.begin(), [](char character) {
        return static_cast<char8_t>(static_cast<unsigned char>(character));
    });
    return std::filesystem::path(encodedPath);
}

[[nodiscard]] std::string displayPath(const std::filesystem::path& path)
{
    const std::u8string encodedPath = path.u8string();
    return std::string(encodedPath.begin(), encodedPath.end());
}

class ImageResource {
public:
    explicit ImageResource(Image image) noexcept
        : image_(image)
    {
    }

    ~ImageResource()
    {
        if (image_.data != nullptr) {
            UnloadImage(image_);
        }
    }

    ImageResource(const ImageResource&) = delete;
    ImageResource& operator=(const ImageResource&) = delete;

    [[nodiscard]] Image& get() noexcept
    {
        return image_;
    }

    [[nodiscard]] const Image& get() const noexcept
    {
        return image_;
    }

private:
    Image image_;
};

} // namespace

TextureManager::ManagedTexture::ManagedTexture(Texture2D texture) noexcept
    : texture_(texture)
{
}

TextureManager::ManagedTexture::~ManagedTexture()
{
    if (texture_.id != 0) {
        UnloadTexture(texture_);
    }
}

TextureManager::ManagedTexture::ManagedTexture(ManagedTexture&& other) noexcept
    : texture_(std::exchange(other.texture_, Texture2D{}))
{
}

TextureManager::ManagedTexture& TextureManager::ManagedTexture::operator=(ManagedTexture&& other) noexcept
{
    if (this == &other) {
        return *this;
    }

    if (texture_.id != 0) {
        UnloadTexture(texture_);
    }
    texture_ = std::exchange(other.texture_, Texture2D{});
    return *this;
}

const Texture2D& TextureManager::ManagedTexture::get() const noexcept
{
    return texture_;
}

void TextureManager::ManagedTexture::release() noexcept
{
    texture_ = Texture2D{};
}

TextureManager& TextureManager::instance()
{
    static TextureManager manager;
    return manager;
}

TextureManager::~TextureManager()
{
    clear();
}

void TextureManager::add(const std::string& key, Texture2D texture)
{
    ManagedTexture managedTexture{texture};
    if (key.empty()) {
        throw std::invalid_argument("Texture key cannot be empty");
    }
    if (managedTexture.get().id == 0) {
        throw std::invalid_argument("Cannot add an invalid texture");
    }

    store(key, std::move(managedTexture));
}

void TextureManager::load(const std::string& key, const std::string& relativePath)
{
    loadFromFile(key, relativePath, nullptr, 0, 0);
}

void TextureManager::loadResized(
    const std::string& key, const std::string& relativePath, Rectangle source, int width, int height)
{
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Resized texture dimensions must be positive");
    }
    if (source.width <= 0.0f || source.height <= 0.0f) {
        throw std::invalid_argument("Texture source dimensions must be positive");
    }

    loadFromFile(key, relativePath, &source, width, height);
}

void TextureManager::loadFromFile(
    const std::string& key, const std::string& relativePath, Rectangle* source, int width, int height)
{
    if (key.empty()) {
        throw std::invalid_argument("Texture key cannot be empty");
    }

    const std::filesystem::path path = executableDirectory() / utf8Path(relativePath);
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to open texture file: " + displayPath(path));
    }

    const std::streampos fileSize = file.tellg();
    if (fileSize <= 0 || fileSize > std::numeric_limits<int>::max()) {
        throw std::runtime_error("Invalid texture file size: " + displayPath(path));
    }

    std::vector<unsigned char> data(static_cast<std::size_t>(fileSize));
    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()))) {
        throw std::runtime_error("Failed to read texture file: " + displayPath(path));
    }

    const std::string extension = path.extension().string();
    ImageResource image{LoadImageFromMemory(extension.c_str(), data.data(), static_cast<int>(data.size()))};
    if (image.get().data == nullptr) {
        throw std::runtime_error("Failed to decode texture file: " + displayPath(path));
    }

    if (source != nullptr) {
        const Image& decodedImage = image.get();
        if (source->x < 0.0f || source->y < 0.0f
            || source->x + source->width > static_cast<float>(decodedImage.width)
            || source->y + source->height > static_cast<float>(decodedImage.height)) {
            throw std::invalid_argument("Texture source rectangle exceeds image bounds: " + displayPath(path));
        }

        ImageCrop(&image.get(), *source);
        if (image.get().data == nullptr || image.get().width <= 0 || image.get().height <= 0) {
            throw std::runtime_error("Failed to crop texture file: " + displayPath(path));
        }
        ImageResize(&image.get(), width, height);
        if (image.get().data == nullptr || image.get().width != width || image.get().height != height) {
            throw std::runtime_error("Failed to resize texture file: " + displayPath(path));
        }
    }

    ManagedTexture texture{LoadTextureFromImage(image.get())};
    if (texture.get().id == 0) {
        throw std::runtime_error("Failed to upload texture to GPU: " + displayPath(path));
    }
    store(key, std::move(texture));
}

void TextureManager::store(const std::string& key, ManagedTexture texture)
{
    const auto existing = textures_.find(key);
    if (existing != textures_.end() && existing->second.get().id == texture.get().id) {
        texture.release();
        return;
    }

    for (const auto& [existingKey, existingTexture] : textures_) {
        if (existingTexture.get().id == texture.get().id && existingKey != key) {
            texture.release();
            throw std::invalid_argument("Texture is already registered with another key");
        }
    }

    if (existing != textures_.end()) {
        existing->second = std::move(texture);
        return;
    }

    textures_.emplace(key, std::move(texture));
}

const Texture2D& TextureManager::get(const std::string& key) const
{
    const auto texture = textures_.find(key);
    if (texture == textures_.end()) {
        throw std::out_of_range("Texture not found: " + key);
    }
    return texture->second.get();
}

void TextureManager::clear() noexcept
{
    textures_.clear();
}

} // namespace view
