#pragma once

#include <cmath>
#include <string_view>

#include "blaster.hh"
#include "engine.hh"
#include "raylib.h"
#include "raymath.h"
#include "sprite.hh"

namespace asteroides {

struct Spaceship {
    public:
        static constexpr std::string_view ASSET_NAME{ "spaceship.png" };
        static constexpr f32 SPRITE_SIZE{ 32.0f };
        static constexpr Rectangle FRAME_UP{ 0.0f, 0.0f, SPRITE_SIZE, SPRITE_SIZE };

        void draw() const noexcept;
        void update(const f32 dt) noexcept;
        [[nodiscard]] Blaster& get_blaster() noexcept { return blaster; }
        [[nodiscard]] Vector2 get_pos() const noexcept { return pos; }
        [[nodiscard]] Vector2 get_vel() const noexcept { return vel; }
        [[nodiscard]] f32 get_radius() const noexcept { return radius * scale; }

    private:
        assets::Sprite sprite{ ASSET_NAME };
        Blaster blaster{ };
        Vector2 pos{ static_cast<f32>(GetScreenWidth() / 2), static_cast<f32>(GetScreenHeight() / 2) };
        Vector2 vel{ Vector2Zero() };
        Vector2 acc{ 500.0f, 500.0f };
        f32 rotation{ 0.0f };
        f32 turn_speed{ 15.0f };
        f32 scale{ 2.0f };
        Color color{ hex_to_color(0xfe166dff) };
        f32 radius{ 10.0f };
};

} // namespace asteroides
