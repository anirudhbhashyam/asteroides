#pragma once

#include <cmath>
#include <numbers>
#include <string_view>

#include "engine.hh"
#include "raylib.h"
#include "spaceship.hh"

namespace asteroides {

enum class ScreenState : u8 {
    Start = 0,
    Playing,
    Paused,
    GameOver
};

enum class ScreenAction : u8 {
    None = 0,
    StartGame,
    ResumeGame,
    RestartGame,
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
        case ScreenState::GameOver:
            return {
                .update_mask = Background | Asteroids | Overlay,
                .draw_mask   = Background | Asteroids | Combat | Hud | Overlay
            };
    }
    return { };
}

struct ScreenManager {
    public:
        ScreenManager() noexcept = default;

        [[nodiscard]] ScreenAction update(f32 dt, const InputState& input, bool player_alive) noexcept;
        void draw_hud(const Spaceship& spaceship) const noexcept;
        void draw_overlay() const noexcept;

        [[nodiscard]] ScreenState get_state() const noexcept { return state; }
        [[nodiscard]] std::string_view get_state_name() const noexcept;
        [[nodiscard]] LayerPolicy get_policy() const noexcept { return get_layer_policy(state); }
        void set_state(ScreenState new_state) noexcept { state = new_state; }

    private:
        void _draw_centered_text(const char* text, i32 y, i32 font_size, Color color) const noexcept;
        void _draw_start() const noexcept;
        void _draw_paused() const noexcept;
        void _draw_game_over() const noexcept;

        ScreenState state{ ScreenState::Start };
        f32 pulse_timer{ 0.0f };
};

} // namespace asteroides
