#pragma once

#include <cmath>
#include <string_view>

#include "blaster.hh"
#include "engine.hh"
#include "particle.hh"
#include "raylib.h"
#include "raymath.h"
#include "sprite.hh"

namespace asteroides {

struct Spaceship {
    public:
        static constexpr std::string_view ASSET_NAME{ "spaceship.png" };
        static constexpr f32 SPRITE_SIZE{ 32.0f };
        static constexpr Rectangle FRAME_UP{ 0.0f, 0.0f, SPRITE_SIZE, SPRITE_SIZE };
        static constexpr i32 DEFAULT_MAX_HEALTH{ 100 };

        static constexpr ParticleEffect HIT_SPARK_EFFECT{
            .count_min = 10,
            .count_max = 18,
            .speed_min = 60.0f,
            .speed_max = 200.0f,
            .size_min = 2.0f,
            .size_max = 5.0f,
            .ttl_min = 0.2f,
            .ttl_max = 0.5f,
            .drag = 3.0f,
            .direction_deg = 0.0f,
            .spread_deg = 360.0f,
            .shrink = true,
            .start_palette = { {
                hex_to_color(0x3c9bd3ff),
                hex_to_color(0xfef08aff),
                hex_to_color(0xfe166dff),
                hex_to_color(0xf8fafcff)
            } },
            .end_color = hex_to_color(0x1e293bff)
        };

        static constexpr ParticleEffect EXPLOSION_EFFECT{
            .count_min = 48,
            .count_max = 68,
            .speed_min = 50.0f,
            .speed_max = 320.0f,
            .size_min = 3.0f,
            .size_max = 9.0f,
            .ttl_min = 0.55f,
            .ttl_max = 1.35f,
            .drag = 2.0f,
            .direction_deg = 0.0f,
            .spread_deg = 360.0f,
            .shrink = true,
            .start_palette = { {
                hex_to_color(0xfef9c3ff),
                hex_to_color(0x3c9bd3ff),
                hex_to_color(0xfe166dff),
                hex_to_color(0xf97316ff)
            } },
            .end_color = hex_to_color(0x4c0519ff)
        };

        void draw() const noexcept;
        void update(const f32 dt) noexcept;
        void take_damage(i32 amount, bool grant_invulnerability = false) noexcept;
        void apply_impulse(Vector2 impulse) noexcept;
        void reset() noexcept;

        [[nodiscard]] Blaster& get_blaster() noexcept { return blaster; }
        [[nodiscard]] Vector2 get_pos() const noexcept { return pos; }
        [[nodiscard]] Vector2 get_vel() const noexcept { return vel; }
        [[nodiscard]] f32 get_radius() const noexcept { return radius * scale; }
        [[nodiscard]] i32 get_health() const noexcept { return health; }
        [[nodiscard]] i32 get_max_health() const noexcept { return max_health; }
        [[nodiscard]] bool is_alive() const noexcept { return health > 0; }
        [[nodiscard]] bool is_invulnerable() const noexcept { return invulnerability_timer > 0.0f; }

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
        i32 max_health{ DEFAULT_MAX_HEALTH };
        i32 health{ DEFAULT_MAX_HEALTH };
        f32 invulnerability_timer{ 0.0f };
        f32 hit_flash_timer{ 0.0f };
};

} // namespace asteroides
