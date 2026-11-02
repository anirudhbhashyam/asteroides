#pragma once

#include <array>
#include <cmath>
#include <numbers>
#include <optional>
#include <vector>

#include "engine.hh"
#include "raylib.h"
#include "raymath.h"

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

        explicit World(Vector2 initial_target, u64 star_count = 120);

        void update(f32 dt, Vector2 target_pos, Vector2 target_vel) noexcept;
        void draw_background() const noexcept;
        void begin_camera() const noexcept;
        void end_camera() const noexcept;
        void reset_camera(Vector2 target_pos) noexcept;
        void set_camera_lock(std::optional<Vector2> locked_target) noexcept { locked_camera_target = locked_target; }
        [[nodiscard]] bool is_camera_locked() const noexcept { return locked_camera_target.has_value(); }
        void add_trauma(f32 amount) noexcept;

        [[nodiscard]] Vector2 get_camera_target() const noexcept { return camera.target; }
        [[nodiscard]] f32 get_trauma() const noexcept { return trauma; }

    private:
        std::vector<Star> stars{ };
        Camera2D camera{ };
        std::optional<Vector2> locked_camera_target{ std::nullopt };
        f32 camera_smooth_speed{ 6.0f };
        f32 trauma{ 0.0f };
        Vector2 ambient_drift{ -14.0f, 20.0f };
};

} // namespace asteroides
