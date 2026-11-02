#pragma once

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <string_view>

#include "asteroid.hh"
#include "enemy.hh"
#include "engine.hh"
#include "level.hh"
#include "powerup.hh"
#include "raylib.h"
#include "raymath.h"
#include "spaceship.hh"

namespace asteroides {

enum class ScreenState : u8 {
    Start = 0,
    LevelSelect,
    Playing,
    Paused,
    LevelComplete,
    GameOver
};

enum class ScreenAction : u8 {
    None = 0,
    StartGame,
    ResumeGame,
    RestartGame,
    NextLevel,
    ReturnToTitle
};

enum class Layer : u8 {
    None       = 0,
    Background = 1 << 0,
    Asteroids  = 1 << 1,
    Combat     = 1 << 2,
    Hud        = 1 << 3,
    Overlay    = 1 << 4
};

constexpr Layer operator|(Layer a, Layer b) noexcept {
    return static_cast<Layer>(static_cast<u8>(a) | static_cast<u8>(b));
}

constexpr bool has_layer(Layer mask, Layer flag) noexcept {
    return (static_cast<u8>(mask) & static_cast<u8>(flag)) != 0;
}

struct LayerPolicy {
    Layer update_mask{ Layer::None };
    Layer draw_mask{ Layer::None };
};

constexpr LayerPolicy get_layer_policy(ScreenState state) noexcept {
    using enum Layer;
    switch (state) {
        case ScreenState::Start:
        case ScreenState::LevelSelect:
            return {
                .update_mask = Background | Asteroids | Overlay,
                .draw_mask   = Background | Asteroids | Overlay
            };
        case ScreenState::Playing:
            return {
                .update_mask = Background | Asteroids | Combat | Hud,
                .draw_mask   = Background | Asteroids | Combat | Hud
            };
        case ScreenState::Paused:
            return {
                .update_mask = Overlay,
                .draw_mask   = Background | Asteroids | Combat | Hud | Overlay
            };
        case ScreenState::LevelComplete:
        case ScreenState::GameOver:
            return {
                .update_mask = Background | Asteroids | Overlay,
                .draw_mask   = Background | Asteroids | Combat | Hud | Overlay
            };
    }
    return { };
}

struct ProgressionState {
    static constexpr f32 COMBO_DURATION{ 4.0f };
    static constexpr f32 MAX_MULTIPLIER{ 8.0f };
    static constexpr f32 WAVE_DURATION{ 28.0f };

    u64 score{ 0 };
    u64 high_score{ 0 };
    f32 combo_multiplier{ 1.0f };
    f32 combo_timer{ 0.0f };
    u32 current_wave{ 1 };
    f32 wave_timer{ 0.0f };
    f32 wave_banner_timer{ 2.2f };

    void reset() noexcept;
    [[nodiscard]] bool update(f32 dt) noexcept;
    [[nodiscard]] u64 add_kill_score(u64 base_points) noexcept;
    void on_player_damaged() noexcept;
};

struct ScreenManager {
    public:
        ScreenManager() noexcept = default;

        [[nodiscard]] ScreenAction update(
            f32 dt,
            const InputState& input,
            bool player_alive,
            LevelManager& levels
        ) noexcept;
        void draw_hud(
            const Spaceship& spaceship,
            const ProgressionState& progression,
            const LevelManager& levels,
            const EnemyField& enemies,
            const PowerupField& powerups,
            const AsteroidField& asteroids,
            Vector2 view_center
        ) const noexcept;
        void draw_overlay(const ProgressionState& progression, const LevelManager& levels) const noexcept;

        [[nodiscard]] ScreenState get_state() const noexcept { return state; }
        [[nodiscard]] std::string_view get_state_name() const noexcept;
        [[nodiscard]] LayerPolicy get_policy() const noexcept { return get_layer_policy(state); }
        void set_state(ScreenState new_state) noexcept { state = new_state; }

    private:
        void _draw_centered_text(const char* text, i32 y, i32 font_size, Color color) const noexcept;
        void _draw_start(const ProgressionState& progression, const LevelManager& levels) const noexcept;
        void _draw_level_select(const LevelManager& levels) const noexcept;
        void _draw_paused(const LevelManager& levels) const noexcept;
        void _draw_level_complete(const ProgressionState& progression, const LevelManager& levels) const noexcept;
        void _draw_game_over(const ProgressionState& progression, const LevelManager& levels) const noexcept;
        void _draw_edge_indicators(
            const LevelManager& levels,
            const EnemyField& enemies,
            const PowerupField& powerups,
            Vector2 view_center
        ) const noexcept;
        void _draw_radar(
            const Spaceship& spaceship,
            const LevelManager& levels,
            const EnemyField& enemies,
            const PowerupField& powerups,
            const AsteroidField& asteroids,
            Vector2 view_center
        ) const noexcept;

        ScreenState state{ ScreenState::Start };
        f32 pulse_timer{ 0.0f };
};

} // namespace asteroides
