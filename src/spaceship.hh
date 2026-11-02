#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <string_view>
#include <utility>

#include "blaster.hh"
#include "engine.hh"
#include "particle.hh"
#include "raylib.h"
#include "raymath.h"
#include "sprite.hh"

namespace asteroides {

enum class PowerupType : u8 {
    DoubleBlaster = 0,
    IncreaseHealth,
    Shield,
    Count
};

struct Spaceship {
    public:
        static constexpr std::string_view ASSET_NAME{ "spaceship.png" };
        static constexpr std::string_view FLAME_ASSET_NAME{ "flame.png" };
        static constexpr f32 SPRITE_SIZE{ 32.0f };
        static constexpr u32 FLAME_FRAME_COUNT{ 4 };
        static constexpr f32 FLAME_FPS{ 18.0f };
        static constexpr Rectangle FRAME_UP{ 0.0f, 0.0f, SPRITE_SIZE, SPRITE_SIZE };
        static constexpr i32 DEFAULT_MAX_HEALTH{ 100 };
        static constexpr u64 POWERUP_COUNT{ static_cast<u64>(PowerupType::Count) };
        static constexpr f32 DASH_DURATION{ 0.18f };
        static constexpr f32 DASH_INVULN_DURATION{ 0.22f };
        static constexpr f32 DASH_COOLDOWN{ 2.5f };
        static constexpr f32 DASH_IMPULSE{ 650.0f };

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

        static constexpr ParticleEffect THRUSTER_EFFECT{
            .count_min = 1,
            .count_max = 2,
            .speed_min = 70.0f,
            .speed_max = 150.0f,
            .size_min = 2.0f,
            .size_max = 4.5f,
            .ttl_min = 0.14f,
            .ttl_max = 0.28f,
            .drag = 4.0f,
            .direction_deg = 180.0f,
            .spread_deg = 32.0f,
            .shrink = true,
            .start_palette = { {
                hex_to_color(0x38bdf8ff),
                hex_to_color(0xfe166dff),
                hex_to_color(0xfef08aff),
                hex_to_color(0xf8fafcff)
            } },
            .end_color = hex_to_color(0x1e1b4bff)
        };

        static constexpr ParticleEffect DASH_BURST_EFFECT{
            .count_min = 18,
            .count_max = 26,
            .speed_min = 80.0f,
            .speed_max = 240.0f,
            .size_min = 2.5f,
            .size_max = 5.5f,
            .ttl_min = 0.22f,
            .ttl_max = 0.45f,
            .drag = 3.2f,
            .direction_deg = 180.0f,
            .spread_deg = 75.0f,
            .shrink = true,
            .start_palette = { {
                hex_to_color(0x38bdf8ff),
                hex_to_color(0x5eead4ff),
                hex_to_color(0xfe166dff),
                hex_to_color(0xf8fafcff)
            } },
            .end_color = hex_to_color(0x0f172aff)
        };

        void draw() const noexcept;
        void update(const f32 dt, const InputState& input) noexcept;
        void take_damage(i32 amount, bool grant_invulnerability = false) noexcept;
        void heal_percent(f32 fraction) noexcept;
        void activate_powerup(PowerupType type, f32 duration) noexcept;
        void apply_impulse(Vector2 impulse) noexcept;
        void reset() noexcept;

        void set_pos(Vector2 new_pos) noexcept { pos = new_pos; }
        void set_vel(Vector2 new_vel) noexcept { vel = new_vel; }
        void set_rotation(f32 new_rotation) noexcept { rotation = new_rotation; }
        void set_health(i32 new_health) noexcept { health = std::clamp(new_health, 0, max_health); }
        void refund_dash_cooldown(f32 fraction = 0.5f) noexcept;
        [[nodiscard]] bool consume_just_dashed() noexcept { return std::exchange(just_dashed, false); }

        [[nodiscard]] Blaster& get_blaster() noexcept { return blaster; }
        [[nodiscard]] const Blaster& get_blaster() const noexcept { return blaster; }
        [[nodiscard]] Vector2 get_pos() const noexcept { return pos; }
        [[nodiscard]] Vector2 get_vel() const noexcept { return vel; }
        [[nodiscard]] f32 get_rotation() const noexcept { return rotation; }
        [[nodiscard]] f32 get_radius() const noexcept { return radius * scale; }
        [[nodiscard]] i32 get_health() const noexcept { return health; }
        [[nodiscard]] i32 get_max_health() const noexcept { return max_health; }
        [[nodiscard]] f32 get_invulnerability_timer() const noexcept { return invulnerability_timer; }
        [[nodiscard]] f32 get_dash_cooldown() const noexcept { return dash_cooldown_timer; }
        [[nodiscard]] bool is_alive() const noexcept { return health > 0; }
        [[nodiscard]] bool is_thrusting() const noexcept { return thrusting; }
        [[nodiscard]] u32 get_flame_frame() const noexcept { return flame_frame; }
        [[nodiscard]] bool is_dashing() const noexcept { return dash_timer > 0.0f; }
        [[nodiscard]] bool is_strafe_locked() const noexcept { return strafe_locked; }
        [[nodiscard]] bool is_powerup_active(PowerupType type) const noexcept;
        [[nodiscard]] f32 get_powerup_remaining(PowerupType type) const noexcept;
        [[nodiscard]] bool is_shielded() const noexcept { return is_powerup_active(PowerupType::Shield); }
        [[nodiscard]] bool is_invulnerable() const noexcept { return invulnerability_timer > 0.0f || is_shielded(); }

    private:
        assets::Sprite sprite{ ASSET_NAME };
        assets::Sprite flame_sprite{ FLAME_ASSET_NAME };
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
        f32 dash_timer{ 0.0f };
        f32 dash_cooldown_timer{ 0.0f };
        std::array<f32, POWERUP_COUNT> powerup_timers{ };
        f32 halo_pulse_timer{ 0.0f };
        f32 flame_anim_timer{ 0.0f };
        u32 flame_frame{ 0 };
        bool thrusting{ false };
        bool strafe_locked{ false };
        bool just_dashed{ false };
};

} // namespace asteroides
