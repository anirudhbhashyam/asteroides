#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <numbers>
#include <optional>
#include <random>
#include <string_view>

#include "raylib.h"
#include "raymath.h"

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using f32 = float;
using f64 = double;

namespace asteroides {

consteval Color hex_to_color(u32 hex) {
    // RRGGBBAA
    return {
        .r = static_cast<u8>((hex >> 8 * 3) & 0xFF),
        .g = static_cast<u8>((hex >> 8 * 2) & 0xFF),
        .b = static_cast<u8>((hex >> 8 * 1) & 0xFF),
        .a = static_cast<u8>((hex >> 8 * 0) & 0xFF)
    };
}

class Random {
    public:
        static void init(u32 seed = 0) noexcept {
            if (seed == 0) {
                s_random_engine.seed(std::random_device{ }());
            } else {
                s_random_engine.seed(seed);
            }
        }

        static u32 uint() noexcept {
            return s_distribution(s_random_engine);
        }

        static u32 uint(u32 min, u32 max) noexcept {
            return min + (s_distribution(s_random_engine) % (max - min + 1));
        }

        static f32 real() noexcept {
            return static_cast<f32>(s_distribution(s_random_engine)) /
                   static_cast<f32>(std::numeric_limits<u32>::max());
        }

        static f32 real(f32 min, f32 max) noexcept {
            return real() * (max - min) + min;
        }

        static Vector2 vec2() noexcept {
            return Vector2{ real(), real() };
        }

        static Vector2 vec2(f32 min, f32 max) noexcept {
            return Vector2{ real(min, max), real(min, max) };
        }

    private:
        static std::mt19937 s_random_engine;
        static std::uniform_int_distribution<std::mt19937::result_type> s_distribution;
};

struct TimeState {
    f32 raw_dt{ 0.0f };
    f32 dt{ 0.0f };
    f32 time_scale{ 1.0f };
    f32 hitstop_timer{ 0.0f };
    f64 elapsed_time{ 0.0 };
    u64 frame_count{ 0 };
};

struct InputState {
    Vector2 move_axis{ 0.0f, 0.0f };
    bool fire_down{ false };
    bool fire_pressed{ false };
    bool confirm_pressed{ false };
    bool pause_pressed{ false };
    bool restart_pressed{ false };
    bool menu_pressed{ false };
    bool debug_toggle_pressed{ false };

    [[nodiscard]] static InputState poll_hardware() noexcept;
};

struct TelemetryData {
    std::string_view screen_name{ "Unknown" };
    u8 update_mask{ 0 };
    u8 draw_mask{ 0 };
    Vector2 player_pos{ 0.0f, 0.0f };
    Vector2 player_vel{ 0.0f, 0.0f };
    f32 player_rotation{ 0.0f };
    i32 player_health{ 0 };
    i32 player_max_health{ 100 };
    f32 player_invuln_timer{ 0.0f };
    f32 double_blaster_timer{ 0.0f };
    f32 shield_timer{ 0.0f };
    u64 asteroid_count{ 0 };
    u64 fragment_count{ 0 };
    u64 enemy_count{ 0 };
    u64 player_bolt_count{ 0 };
    u64 enemy_bolt_count{ 0 };
    u64 powerup_count{ 0 };
    u64 particle_count{ 0 };
};

struct EngineConfig {
    i32 window_width{ 800 };
    i32 window_height{ 600 };
    i32 target_fps{ 60 };
    const char* title{ "Asteroids on steroids" };
    u32 rng_seed{ 0 };
    bool start_with_debug{ false };
};

struct Engine {
    public:
        explicit Engine(const EngineConfig& config = EngineConfig{ }) noexcept;
        ~Engine() noexcept;

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;
        Engine(Engine&&) = delete;
        Engine& operator=(Engine&&) = delete;

        [[nodiscard]] bool is_running() const noexcept;
        void begin_frame(std::optional<f32> fixed_dt = std::nullopt) noexcept;
        void end_frame() const noexcept;

        void override_input(const InputState& synthetic_input) noexcept;
        void set_time_scale(f32 scale) noexcept;
        void trigger_hitstop(f32 duration) noexcept;
        void set_debug_enabled(bool enabled) noexcept { debug_enabled = enabled; }

        [[nodiscard]] bool is_debug_enabled() const noexcept { return debug_enabled; }
        [[nodiscard]] const TimeState& get_time() const noexcept { return time; }
        [[nodiscard]] const InputState& get_input() const noexcept { return input; }
        [[nodiscard]] i32 get_width() const noexcept { return GetScreenWidth(); }
        [[nodiscard]] i32 get_height() const noexcept { return GetScreenHeight(); }

        void draw_debug_circle(Vector2 center, f32 radius, Color color) const noexcept;
        void draw_debug_kinematics(Vector2 pos, Vector2 vel, f32 rotation_deg, f32 radius) const noexcept;
        void draw_debug_line(Vector2 from, Vector2 to, Color color) const noexcept;
        void draw_telemetry(const TelemetryData& data) const noexcept;

    private:
        TimeState time{ };
        InputState input{ };
        std::optional<InputState> pending_input_override{ std::nullopt };
        bool debug_enabled{ false };
};

} // namespace asteroides
