#pragma once

#include <array>
#include <cmath>
#include <numbers>
#include <vector>

#include "blaster.hh"
#include "engine.hh"
#include "powerup.hh"
#include "raylib.h"
#include "raymath.h"
#include "sprite.hh"

namespace asteroides {

struct Asteroid {
    public:
        static constexpr i32 SPRITE_SIZE{ 32 };
        static constexpr u64 PIXEL_COUNT{ static_cast<u64>(SPRITE_SIZE * SPRITE_SIZE) };
        static constexpr u64 VERTEX_COUNT{ 12 };

        static constexpr Color COLOR_EMPTY{ hex_to_color(0x00000000) };
        static constexpr Color COLOR_BODY{ hex_to_color(0x000000ff) };
        static constexpr Color COLOR_RIM{ hex_to_color(0x4e4d50ff) };

        Asteroid() noexcept = default;

        static std::array<Color, PIXEL_COUNT> generate_pixels() noexcept;
        static Asteroid spawn_from_edge(Vector2 view_center, i32 screen_w, i32 screen_h) noexcept;
        static Asteroid spawn_fragment(Vector2 origin, f32 parent_scale, f32 angle_rad) noexcept;

        void update(f32 dt) noexcept;
        void draw() const noexcept;
        [[nodiscard]] bool is_offscreen(Vector2 view_center, i32 screen_w, i32 screen_h, f32 margin = 120.0f) const noexcept;
        [[nodiscard]] bool is_expired() const noexcept;
        [[nodiscard]] bool collides_with(Vector2 point, f32 point_radius) const noexcept;
        [[nodiscard]] bool collides_with(const Asteroid& other) const noexcept;
        static void resolve_physics_collision(Asteroid& a, Asteroid& b) noexcept;
        [[nodiscard]] Vector2 get_pos() const noexcept { return pos; }
        [[nodiscard]] f32 get_scale() const noexcept { return scale; }
        [[nodiscard]] f32 get_radius() const noexcept { return radius; }

    private:
        assets::Sprite sprite{ };
        Vector2 pos{ Vector2Zero() };
        Vector2 vel{ Vector2Zero() };
        f32 scale{ 2.0f };
        f32 radius{ 32.0f };
        f32 rotation{ 0.0f };
        f32 angular_vel{ 0.0f };
        f32 ttl{ -1.0f };
        f32 max_ttl{ 1.0f };
        bool is_fragment{ false };
};

struct AsteroidField {
    public:
        explicit AsteroidField(Vector2 initial_center, u64 count = 12);

        void update(f32 dt, Vector2 view_center, Blaster& blaster, PowerupField& powerups) noexcept;
        void draw() const noexcept;

    private:
        void _spawn_explosion(Vector2 origin, f32 parent_scale) noexcept;

        std::vector<Asteroid> asteroids;
        std::vector<Asteroid> fragments;
};

} // namespace asteroides
