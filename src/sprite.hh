#pragma once

#include <filesystem>
#include <print>
#include <string_view>
#include <utility>

#include "engine.hh"
#include "raylib.h"

namespace asteroides::assets {

constexpr inline std::string_view ASSETS_DIR{ "assets" };

struct Sprite {
    public:
        Sprite() noexcept = default;
        explicit Sprite(std::string_view asset_name);
        explicit Sprite(const std::filesystem::path& asset_path);
        Sprite(const Color* pixels, i32 width, i32 height) noexcept;
        ~Sprite() noexcept;

        Sprite(const Sprite&) = delete;
        Sprite& operator=(const Sprite&) = delete;

        Sprite(Sprite&& other) noexcept;
        Sprite& operator=(Sprite&& other) noexcept;

        void draw(Vector2 pos, f32 rotation = 0.0f, f32 scale = 1.0f, Color tint = WHITE) const noexcept;
        void draw(Vector2 pos, Rectangle src, f32 rotation = 0.0f, f32 scale = 1.0f, Color tint = WHITE) const noexcept;
        [[nodiscard]] bool is_valid() const noexcept;
        [[nodiscard]] const Texture2D& get_texture() const noexcept;
        void clean() noexcept;

    private:
        Texture2D texture{ };
};

} // namespace asteroides::assets
