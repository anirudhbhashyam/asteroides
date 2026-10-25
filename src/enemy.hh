#pragma once

#include <cmath>
#include <string_view>
#include <vector>

#include "blaster.hh"
#include "engine.hh"
#include "particle.hh"
#include "raylib.h"
#include "raymath.h"
#include "sprite.hh"

namespace asteroides {

struct EnemySpaceship {
    public:
        static constexpr std::string_view ASSET_NAME{ "enemy_spaceship.png" };
        static constexpr std::string_view BLASTER_ASSET_NAME{ "enemy_blaster.png" };
        static constexpr i32 DEFAULT_MAX_HEALTH{ 3 };

        static constexpr ParticleEffect HIT_SPARK_EFFECT{
            .count_min = 8,
            .count_max = 14,
            .speed_min = 60.0f,
            .speed_max = 180.0f,
            .size_min = 2.0f,
            .size_max = 4.0f,
            .ttl_min = 0.2f,
            .ttl_max = 0.45f,
            .drag = 3.5f,
            .direction_deg = 0.0f,
            .spread_deg = 360.0f,
            .shrink = true,
            .start_palette = { {
                hex_to_color(0xfef08aff),
                hex_to_color(0xf97316ff),
                hex_to_color(0xa855f7ff),
                hex_to_color(0x38bdf8ff)
            } },
            .end_color = hex_to_color(0x573984ff)
        };

        static constexpr ParticleEffect EXPLOSION_EFFECT{
            .count_min = 36,
            .count_max = 52,
            .speed_min = 40.0f,
            .speed_max = 280.0f,
            .size_min = 3.0f,
            .size_max = 8.0f,
            .ttl_min = 0.45f,
            .ttl_max = 1.15f,
            .drag = 2.2f,
            .direction_deg = 0.0f,
            .spread_deg = 360.0f,
            .shrink = true,
            .start_palette = { {
                hex_to_color(0xfef9c3ff), // white-hot core
                hex_to_color(0xfb923cff), // bright orange flame
                hex_to_color(0xef4444ff), // fiery crimson
                hex_to_color(0xa855f7ff)  // purple thruster plasma
            } },
            .end_color = hex_to_color(0x3b0764ff)
        };

        EnemySpaceship();

        static EnemySpaceship spawn_from_edge(Vector2 view_center, i32 screen_w, i32 screen_h);
        static EnemySpaceship spawn_at(Vector2 origin, f32 initial_rotation = 0.0f);

        void update(f32 dt, Vector2 player_pos, bool player_alive, Vector2 view_center) noexcept;
        void draw() const noexcept;
        void take_damage(i32 amount) noexcept;
        void apply_impulse(Vector2 impulse) noexcept;

        [[nodiscard]] bool is_alive() const noexcept { return health > 0; }
        [[nodiscard]] bool is_offscreen(Vector2 view_center, i32 screen_w, i32 screen_h, f32 margin = 220.0f) const noexcept;
        [[nodiscard]] bool collides_with(Vector2 point, f32 point_radius) const noexcept;
        [[nodiscard]] Vector2 get_pos() const noexcept { return pos; }
        [[nodiscard]] Vector2 get_vel() const noexcept { return vel; }
        [[nodiscard]] f32 get_rotation() const noexcept { return rotation; }
        [[nodiscard]] f32 get_scale() const noexcept { return scale; }
        [[nodiscard]] f32 get_radius() const noexcept { return radius * scale; }
        [[nodiscard]] f32 get_preferred_range() const noexcept { return preferred_range; }
        [[nodiscard]] i32 get_health() const noexcept { return health; }
        [[nodiscard]] i32 get_max_health() const noexcept { return max_health; }
        [[nodiscard]] Blaster& get_blaster() noexcept { return blaster; }
        [[nodiscard]] const Blaster& get_blaster() const noexcept { return blaster; }

    private:
        assets::Sprite sprite{ ASSET_NAME };
        Blaster blaster{ BLASTER_ASSET_NAME, 430.0f, 1.15f };
        Vector2 pos{ Vector2Zero() };
        Vector2 vel{ Vector2Zero() };
        f32 max_speed{ 210.0f };
        f32 accel{ 340.0f };
        f32 preferred_range{ 220.0f };
        f32 strafe_sign{ 1.0f };
        f32 rotation{ 0.0f };
        f32 turn_speed{ 8.0f };
        f32 scale{ 2.0f };
        f32 radius{ 10.0f };
        i32 max_health{ DEFAULT_MAX_HEALTH };
        i32 health{ DEFAULT_MAX_HEALTH };
        f32 hit_flash_timer{ 0.0f };
};

struct EnemyField {
    public:
        static constexpr u64 MAX_ENEMIES{ 4 };

        EnemyField() noexcept = default;

        void update(f32 dt, Vector2 player_pos, bool player_alive, Vector2 view_center) noexcept;
        void draw() const noexcept;
        void reset() noexcept;
        void set_auto_spawn(bool enabled) noexcept { auto_spawn = enabled; }
        EnemySpaceship& spawn_at(Vector2 origin, f32 initial_rotation = 0.0f);
        [[nodiscard]] std::vector<EnemySpaceship>& get_enemies() noexcept { return enemies; }
        [[nodiscard]] const std::vector<EnemySpaceship>& get_enemies() const noexcept { return enemies; }

    private:
        std::vector<EnemySpaceship> enemies{ };
        f32 spawn_timer{ 3.0f };
        f32 spawn_interval{ 5.0f };
        bool auto_spawn{ true };
};

} // namespace asteroides
