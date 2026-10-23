#include "view/StaticSpriteViewModel.hpp"

#include "view/TextureManager.hpp"

#include <stdexcept>

namespace view {

StaticSpriteViewModel::StaticSpriteViewModel(
    const std::string& key, const std::string& path, Rectangle crop, int width, int height)
    : texture_{}
    , source_{}
{
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Static sprite dimensions must be positive");
    }

    TextureManager& textures = TextureManager::instance();
    textures.loadResized(key, path, crop, width, height);
    texture_ = textures.get(key);
    SetTextureFilter(texture_, TEXTURE_FILTER_POINT);
    source_ = Rectangle{0.0F, 0.0F, static_cast<float>(texture_.width), static_cast<float>(texture_.height)};
}

void StaticSpriteViewModel::drawAt(Rectangle destination, Color tint) const
{
    DrawTexturePro(texture_, source_, destination, Vector2{0.0F, 0.0F}, 0.0F, tint);
}

} // namespace view
