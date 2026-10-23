#pragma once

#include <raylib.h>

#include <string>
#include <unordered_map>

namespace view {

class TextureManager {
public:
    static TextureManager& instance();

    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    TextureManager(TextureManager&&) = delete;
    TextureManager& operator=(TextureManager&&) = delete;
    ~TextureManager();

    void add(const std::string& key, Texture2D texture);
    void load(const std::string& key, const std::string& relativePath);
    void loadResized(
        const std::string& key, const std::string& relativePath, Rectangle source, int width, int height);
    const Texture2D& get(const std::string& key) const;
    void clear() noexcept;

private:
    class ManagedTexture {
    public:
        explicit ManagedTexture(Texture2D texture) noexcept;
        ~ManagedTexture();

        ManagedTexture(const ManagedTexture&) = delete;
        ManagedTexture& operator=(const ManagedTexture&) = delete;
        ManagedTexture(ManagedTexture&& other) noexcept;
        ManagedTexture& operator=(ManagedTexture&& other) noexcept;

        const Texture2D& get() const noexcept;
        void release() noexcept;

    private:
        Texture2D texture_;
    };

    TextureManager() = default;
    void loadFromFile(const std::string& key, const std::string& relativePath, Rectangle* source, int width, int height);
    void store(const std::string& key, ManagedTexture texture);

    std::unordered_map<std::string, ManagedTexture> textures_;
};

} // namespace view
