#include "screen.hh"

namespace asteroides {

ScreenAction ScreenManager::update(f32 dt, bool player_alive) noexcept {
    constexpr f32 two_pi{ 2.0f * std::numbers::pi_v<f32> };
    pulse_timer = std::fmodf(pulse_timer + 3.5f * dt, two_pi);

    switch (state) {
        case ScreenState::Start:
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                state = ScreenState::Playing;
                return ScreenAction::StartGame;
            }
            break;

        case ScreenState::Playing:
            if (!player_alive) {
                state = ScreenState::GameOver;
                return ScreenAction::None;
            }
            if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
                state = ScreenState::Paused;
                return ScreenAction::None;
            }
            break;

        case ScreenState::Paused:
            if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) {
                state = ScreenState::Playing;
                return ScreenAction::ResumeGame;
            }
            if (IsKeyPressed(KEY_R)) {
                state = ScreenState::Playing;
                return ScreenAction::RestartGame;
            }
            if (IsKeyPressed(KEY_M)) {
                state = ScreenState::Start;
                return ScreenAction::ReturnToTitle;
            }
            break;

        case ScreenState::GameOver:
            if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_ENTER)) {
                state = ScreenState::Playing;
                return ScreenAction::RestartGame;
            }
            if (IsKeyPressed(KEY_M)) {
                state = ScreenState::Start;
                return ScreenAction::ReturnToTitle;
            }
            break;
    }

    return ScreenAction::None;
}

void ScreenManager::draw_hud(const Spaceship& spaceship) const noexcept {
    constexpr i32 bar_x{ 20 };
    constexpr i32 bar_y{ 20 };
    constexpr i32 bar_w{ 180 };
    constexpr i32 bar_h{ 16 };

    const f32 ratio{
        static_cast<f32>(spaceship.get_health()) / static_cast<f32>(spaceship.get_max_health())
    };
    const i32 fill_w{ static_cast<i32>(static_cast<f32>(bar_w) * ratio) };
    const Color fill_color{
        ratio > 0.5f ? hex_to_color(0x22c55eff) :
        ratio > 0.25f ? hex_to_color(0xf59e0bff) :
                        hex_to_color(0xef4444ff)
    };

    DrawRectangle(bar_x - 2, bar_y - 2, bar_w + 4, bar_h + 4, hex_to_color(0x334155ff));
    DrawRectangle(bar_x, bar_y, bar_w, bar_h, hex_to_color(0x0f172aff));
    if (fill_w > 0) {
        DrawRectangle(bar_x, bar_y, fill_w, bar_h, fill_color);
    }
}

void ScreenManager::draw_overlay() const noexcept {
    switch (state) {
        case ScreenState::Start:
            _draw_start();
            break;
        case ScreenState::Paused:
            _draw_paused();
            break;
        case ScreenState::GameOver:
            _draw_game_over();
            break;
        case ScreenState::Playing:
            break;
    }
}

void ScreenManager::_draw_centered_text(const char* text, i32 y, i32 font_size, Color color) const noexcept {
    const i32 w{ GetScreenWidth() };
    const i32 text_w{ MeasureText(text, font_size) };
    DrawText(text, (w - text_w) / 2, y, font_size, color);
}

void ScreenManager::_draw_start() const noexcept {
    const i32 w{ GetScreenWidth() };
    const i32 h{ GetScreenHeight() };
    DrawRectangle(0, 0, w, h, Fade(BLACK, 0.45f));

    const f32 pulse_alpha{ 0.55f + 0.45f * (0.5f + 0.5f * std::sinf(pulse_timer)) };

    _draw_centered_text("ASTEROIDS ON STEROIDS", h / 2 - 90, 44, hex_to_color(0xfe166dff));
    _draw_centered_text("WASD: Move   |   SPACE: Fire   |   P / ESC: Pause", h / 2 - 20, 18, hex_to_color(0x94a3b8ff));
    _draw_centered_text("Press ENTER or SPACE to Start", h / 2 + 45, 24, Fade(hex_to_color(0xf8fafcff), pulse_alpha));
}

void ScreenManager::_draw_paused() const noexcept {
    const i32 w{ GetScreenWidth() };
    const i32 h{ GetScreenHeight() };
    DrawRectangle(0, 0, w, h, Fade(BLACK, 0.55f));

    _draw_centered_text("PAUSED", h / 2 - 80, 48, hex_to_color(0x38bdf8ff));
    _draw_centered_text("Press P / ESC / ENTER to Resume", h / 2 - 10, 22, hex_to_color(0xf8fafcff));
    _draw_centered_text("Press R to Restart", h / 2 + 26, 20, hex_to_color(0x94a3b8ff));
    _draw_centered_text("Press M for Main Menu", h / 2 + 58, 20, hex_to_color(0x94a3b8ff));
}

void ScreenManager::_draw_game_over() const noexcept {
    const i32 w{ GetScreenWidth() };
    const i32 h{ GetScreenHeight() };
    DrawRectangle(0, 0, w, h, Fade(BLACK, 0.55f));

    const f32 pulse_alpha{ 0.55f + 0.45f * (0.5f + 0.5f * std::sinf(pulse_timer)) };

    _draw_centered_text("GAME OVER", h / 2 - 70, 48, hex_to_color(0xfe166dff));
    _draw_centered_text("Press R or ENTER to Restart", h / 2 + 5, 22, Fade(hex_to_color(0xf8fafcff), pulse_alpha));
    _draw_centered_text("Press M for Main Menu", h / 2 + 42, 20, hex_to_color(0x94a3b8ff));
}

} // namespace asteroides
