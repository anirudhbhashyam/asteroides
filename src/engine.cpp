#include "engine.hh"

namespace asteroides {

std::mt19937 Random::s_random_engine;
std::uniform_int_distribution<std::mt19937::result_type> Random::s_distribution;

InputState InputState::poll_hardware() noexcept {
    InputState state{ };

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    state.move_axis.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  state.move_axis.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  state.move_axis.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) state.move_axis.x += 1.0f;

    state.fire_down = IsKeyDown(KEY_SPACE);
    state.fire_pressed = IsKeyPressed(KEY_SPACE);
    state.dash_pressed = IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT);
    state.strafe_lock_down = IsKeyDown(KEY_K) || IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT) ||
                             IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    state.confirm_pressed = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    state.pause_pressed = IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE);
    state.restart_pressed = IsKeyPressed(KEY_R);
    state.menu_pressed = IsKeyPressed(KEY_M);
    state.nav_up_pressed = IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP);
    state.nav_down_pressed = IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN);
    state.nav_left_pressed = IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT);
    state.nav_right_pressed = IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT);
    state.debug_toggle_pressed = IsKeyPressed(KEY_F3) || IsKeyPressed(KEY_TAB);

    return state;
}

Engine::Engine(const EngineConfig& config) noexcept
    : debug_enabled{ config.start_with_debug } {
    Random::init(config.rng_seed);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(config.window_width, config.window_height, config.title);
    SetExitKey(KEY_NULL);
    SetTargetFPS(config.target_fps);
}

Engine::~Engine() noexcept {
    if (IsWindowReady()) {
        CloseWindow();
    }
}

bool Engine::is_running() const noexcept {
    return !WindowShouldClose();
}

void Engine::override_input(const InputState& synthetic_input) noexcept {
    pending_input_override = synthetic_input;
}

void Engine::set_time_scale(f32 scale) noexcept {
    time.time_scale = std::fmaxf(0.0f, scale);
}

void Engine::trigger_hitstop(f32 duration) noexcept {
    time.hitstop_timer = std::fmaxf(time.hitstop_timer, duration);
}

void Engine::begin_frame(std::optional<f32> fixed_dt) noexcept {
    const f32 raw{ fixed_dt.has_value() ? *fixed_dt : GetFrameTime() };
    time.raw_dt = std::fminf(raw, 0.1f);

    if (time.hitstop_timer > 0.0f) {
        time.hitstop_timer = std::fmaxf(0.0f, time.hitstop_timer - time.raw_dt);
        time.dt = 0.0f;
    } else {
        time.dt = time.raw_dt * time.time_scale;
    }

    time.elapsed_time += static_cast<f64>(time.dt);
    ++time.frame_count;

    const InputState hw_input{ InputState::poll_hardware() };
    if (pending_input_override.has_value()) {
        input = *pending_input_override;
        input.debug_toggle_pressed = input.debug_toggle_pressed || hw_input.debug_toggle_pressed;
        pending_input_override.reset();
    } else {
        input = hw_input;
    }

    if (input.debug_toggle_pressed) {
        debug_enabled = !debug_enabled;
    }

    BeginDrawing();
    ClearBackground(BLACK);
}

void Engine::end_frame() const noexcept {
    EndDrawing();
}

void Engine::draw_debug_circle(Vector2 center, f32 radius, Color color) const noexcept {
    if (!debug_enabled) return;
    DrawRing(center, std::fmaxf(0.0f, radius - 1.5f), radius, 0.0f, 360.0f, 36, color);
}

void Engine::draw_debug_kinematics(Vector2 pos, Vector2 vel, f32 rotation_deg, f32 radius) const noexcept {
    if (!debug_enabled) return;

    // Heading tick vector (magenta)
    const f32 rad{ rotation_deg * DEG2RAD };
    const Vector2 heading_dir{ std::sinf(rad), -std::cosf(rad) };
    const Vector2 heading_end{ Vector2Add(pos, Vector2Scale(heading_dir, radius + 14.0f)) };
    DrawLineEx(pos, heading_end, 1.5f, hex_to_color(0xf472b6ff));

    // Velocity vector (lime green)
    if (Vector2LengthSqr(vel) > 1.0f) {
        const Vector2 vel_end{ Vector2Add(pos, Vector2Scale(vel, 0.22f)) };
        DrawLineEx(pos, vel_end, 1.5f, hex_to_color(0x4ade80ff));
    }
}

void Engine::draw_debug_line(Vector2 from, Vector2 to, Color color) const noexcept {
    if (!debug_enabled) return;
    DrawLineEx(from, to, 1.0f, color);
}

void Engine::draw_telemetry(const TelemetryData& data) const noexcept {
    if (!debug_enabled) return;

    const i32 screen_w{ GetScreenWidth() };
    constexpr i32 panel_w{ 252 };
    constexpr i32 panel_h{ 226 };
    const i32 panel_x{ screen_w - panel_w - 12 };
    constexpr i32 panel_y{ 12 };

    DrawRectangle(panel_x, panel_y, panel_w, panel_h, Fade(hex_to_color(0x0f172aff), 0.82f));
    DrawRectangleLines(panel_x, panel_y, panel_w, panel_h, hex_to_color(0x38bdf8ff));

    i32 y{ panel_y + 8 };
    const i32 x{ panel_x + 10 };
    constexpr i32 step{ 16 };
    constexpr i32 font_size{ 10 };

    char buf[128]{ };

    std::snprintf(buf, sizeof(buf), "ENGINE TELEMETRY [F3/TAB]");
    DrawText(buf, x, y, font_size, hex_to_color(0x38bdf8ff));
    y += step;

    std::snprintf(
        buf,
        sizeof(buf),
        "FPS: %d | Frame: %u | Scale: %.1fx",
        GetFPS(),
        static_cast<u32>(time.frame_count),
        static_cast<f64>(time.time_scale)
    );
    DrawText(buf, x, y, font_size, hex_to_color(0xe2e8f0ff));
    y += step;

    std::snprintf(
        buf,
        sizeof(buf),
        "Screen: %s (U:0x%02X D:0x%02X)",
        data.screen_name.data(),
        static_cast<u32>(data.update_mask),
        static_cast<u32>(data.draw_mask)
    );
    DrawText(buf, x, y, font_size, hex_to_color(0xfef08aff));
    y += step;

    std::snprintf(
        buf,
        sizeof(buf),
        "Ship Pos: (%.0f, %.0f) Rot: %.0f",
        static_cast<f64>(data.player_pos.x),
        static_cast<f64>(data.player_pos.y),
        static_cast<f64>(data.player_rotation)
    );
    DrawText(buf, x, y, font_size, hex_to_color(0xe2e8f0ff));
    y += step;

    const f32 speed{ Vector2Length(data.player_vel) };
    std::snprintf(
        buf,
        sizeof(buf),
        "Ship Vel: (%.0f, %.0f) | Spd: %.0f",
        static_cast<f64>(data.player_vel.x),
        static_cast<f64>(data.player_vel.y),
        static_cast<f64>(speed)
    );
    DrawText(buf, x, y, font_size, hex_to_color(0x4ade80ff));
    y += step;

    std::snprintf(
        buf,
        sizeof(buf),
        "HP: %d/%d | Invuln: %.2fs",
        data.player_health,
        data.player_max_health,
        static_cast<f64>(data.player_invuln_timer)
    );
    DrawText(buf, x, y, font_size, hex_to_color(0xf472b6ff));
    y += step;

    std::snprintf(
        buf,
        sizeof(buf),
        "Timers -> Dbl: %.1fs | Shld: %.1fs",
        static_cast<f64>(data.double_blaster_timer),
        static_cast<f64>(data.shield_timer)
    );
    DrawText(buf, x, y, font_size, hex_to_color(0x5eead4ff));
    y += step;

    std::snprintf(
        buf,
        sizeof(buf),
        "Asteroids: %u | Frags: %u",
        static_cast<u32>(data.asteroid_count),
        static_cast<u32>(data.fragment_count)
    );
    DrawText(buf, x, y, font_size, hex_to_color(0xfbbf24ff));
    y += step;

    std::snprintf(
        buf,
        sizeof(buf),
        "Enemies: %u | Powerups: %u",
        static_cast<u32>(data.enemy_count),
        static_cast<u32>(data.powerup_count)
    );
    DrawText(buf, x, y, font_size, hex_to_color(0xf87171ff));
    y += step;

    std::snprintf(
        buf,
        sizeof(buf),
        "Bolts (P/E): %u / %u",
        static_cast<u32>(data.player_bolt_count),
        static_cast<u32>(data.enemy_bolt_count)
    );
    DrawText(buf, x, y, font_size, hex_to_color(0xc084fcff));
    y += step;

    std::snprintf(
        buf,
        sizeof(buf),
        "Particles: %u",
        static_cast<u32>(data.particle_count)
    );
    DrawText(buf, x, y, font_size, hex_to_color(0x94a3b8ff));
}

} // namespace asteroides
