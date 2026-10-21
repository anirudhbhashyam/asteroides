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
            f32 fire_cooldown = 0.15f
        );

        void fire(Vector2 origin, Vector2 ship_vel, f32 heading_deg) noexcept;
        void update(f32 dt, Vector2 view_center) noexcept;
        void draw() const noexcept;
        void activate_double_blaster(f32 duration) noexcept;
        void clear() noexcept;
        [[nodiscard]] std::vector<Bolt>& get_bolts() noexcept { return bolts; }

    private:
        assets::Sprite sprite;
        std::vector<Bolt> bolts{ };
        f32 speed{ 700.0f };
        f32 cooldown{ 0.15f };
        f32 cooldown_timer{ 0.0f };
        f32 double_blaster_timer{ 0.0f };
        f32 scale{ 1.0f };
        f32 muzzle_offset{ 20.0f };
        f32 double_spread_offset{ 10.0f };
};

} // namespace asteroides
