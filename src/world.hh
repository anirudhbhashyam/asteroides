#pragma once

#include <array>
#include <cmath>
#include <numbers>
#include <vector>

#include "asteroid.hh"
#include "engine.hh"
#include "powerup.hh"
#include "raylib.h"
#include "spaceship.hh"

namespace asteroides {

struct Star {
    Vector2 pos{ Vector2Zero() };
    f32 depth{ 0.2f };
    f32 size{ 1.0f };
    f32 base_alpha{ 0.6f };
    f32 twinkle_phase{ 0.0f };
    f32 twinkle_speed{ 2.0f };
    Color color{ WHITE };
};

struct World {
    public:
        static constexpr Color BACKGROUND_COLOR{ hex_to_color(0x0e1219ff) };

        explicit World(u64 star_count = 120, u64 asteroid_count = 12);

        void update(f32 dt) noexcept;
        void draw() const noexcept;

    private:
        std::vector<Star> stars{ };
        Spaceship spaceship{ };
        PowerupField powerups{ };
        AsteroidField asteroids;
        Camera2D camera{ };
        f32 camera_smooth_speed{ 6.0f };
        Vector2 ambient_drift{ -14.0f, 20.0f };
};

} // namespace asteroides
