#pragma once

#include <cmath>
#include <string_view>
#include <vector>

#include "engine.hh"
#include "raylib.h"
#include "raymath.h"
#include "sprite.hh"

namespace asteroides {

struct Blaster {
    public:
        static constexpr std::string_view ASSET_NAME{ "blaster.png" };

        struct Bolt {
            Vector2 pos{ Vector2Zero() };
            Vector2 vel{ Vector2Zero() };
            f32 rotation{ 0.0f };
        };

        explicit Blaster(
            std::string_view asset_name = ASSET_NAME,
            f32 bolt_speed = 700.0f,
            f32 fire_cooldown = 0.15f,
            f32 bolt_scale = 1.0f,
            f32 rotation_offset_deg = -90.0f,
            f32 hit_radius = 4.0f
        );

        void fire(Vector2 origin, Vector2 ship_vel, f32 heading_deg, bool double_shot = false) noexcept;
        void fire_bidirectional(Vector2 origin, Vector2 ship_vel, f32 axis_heading_deg) noexcept;
        void update(f32 dt, Vector2 view_center) noexcept;
        void draw() const noexcept;
        void clear() noexcept;
        [[nodiscard]] f32 get_bolt_radius() const noexcept { return bolt_radius; }
        [[nodiscard]] std::vector<Bolt>& get_bolts() noexcept { return bolts; }
        [[nodiscard]] const std::vector<Bolt>& get_bolts() const noexcept { return bolts; }

    private:
        void _spawn_bolt(Vector2 origin, Vector2 ship_vel, f32 heading_deg) noexcept;

        assets::Sprite sprite;
        std::vector<Bolt> bolts{ };
        f32 speed{ 700.0f };
        f32 cooldown{ 0.15f };
        f32 cooldown_timer{ 0.0f };
        f32 scale{ 1.0f };
        f32 sprite_rotation_offset{ -90.0f };
        f32 bolt_radius{ 4.0f };
        f32 muzzle_offset{ 20.0f };
        f32 double_spread_offset{ 10.0f };
};

} // namespace asteroides
