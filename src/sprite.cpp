#include "sprite.hh"

namespace asteroides::assets {

Sprite::Sprite(std::string_view asset_name)
    : Sprite(std::filesystem::path{ ASSETS_DIR } / asset_name) {}

Sprite::Sprite(const std::filesystem::path& asset_path) {
    if (!std::filesystem::exists(asset_path)) {
        std::println(stderr, "Error: Sprite asset not found at '{}'", asset_path.string());
        return;
    }
    texture = LoadTexture(asset_path.c_str());
    if (texture.id != 0) {
        SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    }
}

Sprite::Sprite(const Color* pixels, i32 width, i32 height) noexcept {
    Image image{
        .data = const_cast<Color*>(pixels),
        .width = width,
        .height = height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };
    texture = LoadTextureFromImage(image);
    if (texture.id != 0) {
        SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    }
}

void Sprite::clean() noexcept {
    if (texture.id != 0) {
        UnloadTexture(texture);
        texture = Texture2D{ };
    }
}

Sprite::~Sprite() noexcept {
    clean();
}

Sprite::Sprite(Sprite&& other) noexcept
    : texture{ std::exchange(other.texture, Texture2D{ }) }
{}

Sprite& Sprite::operator=(Sprite&& other) noexcept {
    if (this != &other) {
        clean();
        texture = std::exchange(other.texture, Texture2D{ });
    }
    return *this;
}

void Sprite::draw(Vector2 pos, f32 rotation, f32 scale, Color tint) const noexcept {
    if (texture.id == 0) return;

    const Rectangle src{
        0.0f,
        0.0f,
        static_cast<f32>(texture.width),
        static_cast<f32>(texture.height)
    };
    draw(pos, src, rotation, scale, tint);
}

void Sprite::draw(Vector2 pos, Rectangle src, f32 rotation, f32 scale, Color tint) const noexcept {
    if (texture.id == 0) return;

    const f32 dst_w{ src.width * scale };
    const f32 dst_h{ src.height * scale };
    const Rectangle dst{ pos.x, pos.y, dst_w, dst_h };
    const Vector2 origin{ dst_w * 0.5f, dst_h * 0.5f };

    DrawTexturePro(texture, src, dst, origin, rotation, tint);
}

bool Sprite::is_valid() const noexcept {
    return IsTextureValid(texture);
}

const Texture2D& Sprite::get_texture() const noexcept {
    return texture;
}

} // namespace asteroides::assets
