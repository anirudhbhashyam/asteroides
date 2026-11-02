#include "screen.hh"

namespace asteroides {

void ProgressionState::reset() noexcept {
    score = 0;
    combo_multiplier = 1.0f;
    combo_timer = 0.0f;
    current_wave = 1;
    wave_timer = 0.0f;
    wave_banner_timer = 2.2f;
}

bool ProgressionState::update(f32 dt) noexcept {
    if (combo_timer > 0.0f) {
        combo_timer = std::fmaxf(0.0f, combo_timer - dt);
        if (combo_timer <= 0.0f) {
            combo_multiplier = 1.0f;
        }
    }

    if (wave_banner_timer > 0.0f) {
        wave_banner_timer = std::fmaxf(0.0f, wave_banner_timer - dt);
    }

    wave_timer += dt;
    if (wave_timer >= WAVE_DURATION) {
        wave_timer -= WAVE_DURATION;
        ++current_wave;
        wave_banner_timer = 2.4f;
        return true;
    }
    return false;
}

u64 ProgressionState::add_kill_score(u64 base_points) noexcept {
    const u64 awarded{
        static_cast<u64>(std::llroundf(static_cast<f32>(base_points) * combo_multiplier))
    };
    score += awarded;
    if (score > high_score) {
        high_score = score;
    }

    combo_multiplier = std::fminf(MAX_MULTIPLIER, combo_multiplier + 0.5f);
    combo_timer = COMBO_DURATION;
    return awarded;
}

void ProgressionState::on_player_damaged() noexcept {
    if (combo_multiplier > 1.0f) {
        combo_multiplier = std::fmaxf(1.0f, std::floorf(combo_multiplier * 0.5f * 2.0f) * 0.5f);
        if (combo_multiplier <= 1.0f) {
            combo_timer = 0.0f;
        }
    }
}

ScreenAction ScreenManager::update(
    f32 dt,
    const InputState& input,
    bool player_alive,
    LevelManager& levels
) noexcept {
    constexpr f32 two_pi{ 2.0f * std::numbers::pi_v<f32> };
    pulse_timer = std::fmodf(pulse_timer + 3.5f * dt, two_pi);

    switch (state) {
        case ScreenState::Start: {
            if (input.nav_left_pressed || input.nav_right_pressed ||
                input.nav_up_pressed || input.nav_down_pressed) {
                levels.set_mode(
                    levels.get_mode() == GameMode::Freestyle ? GameMode::Levels : GameMode::Freestyle
                );
            }
            if (input.confirm_pressed) {
                if (levels.get_mode() == GameMode::Levels) {
                    state = ScreenState::LevelSelect;
                    return ScreenAction::None;
                }
                state = ScreenState::Playing;
                return ScreenAction::StartGame;
            }
            break;
        }

        case ScreenState::LevelSelect: {
            const u32 cur{ levels.get_current_level() };
            const u32 max_unlocked{ levels.get_unlocked_levels() };
            if (input.nav_up_pressed || input.nav_left_pressed) {
                if (cur > 1) {
                    levels.set_current_level(cur - 1);
                }
            } else if (input.nav_down_pressed || input.nav_right_pressed) {
                if (cur < max_unlocked && cur < LevelManager::TOTAL_LEVELS) {
                    levels.set_current_level(cur + 1);
                }
            }
            if (input.menu_pressed || input.pause_pressed) {
                state = ScreenState::Start;
                return ScreenAction::None;
            }
            if (input.confirm_pressed && cur <= max_unlocked) {
                state = ScreenState::Playing;
                return ScreenAction::StartGame;
            }
            break;
        }

        case ScreenState::Playing:
            if (!player_alive) {
                state = ScreenState::GameOver;
                return ScreenAction::None;
            }
            if (input.pause_pressed) {
                state = ScreenState::Paused;
                return ScreenAction::None;
            }
            break;

        case ScreenState::Paused:
            if (input.pause_pressed || input.confirm_pressed) {
                state = ScreenState::Playing;
                return ScreenAction::ResumeGame;
            }
            if (input.restart_pressed) {
                state = ScreenState::Playing;
                return ScreenAction::RestartGame;
            }
            if (input.menu_pressed) {
                state = (levels.get_mode() == GameMode::Levels) ? ScreenState::LevelSelect : ScreenState::Start;
                return ScreenAction::ReturnToTitle;
            }
            break;

        case ScreenState::LevelComplete:
            if (input.confirm_pressed) {
                const u32 cur{ levels.get_current_level() };
                if (cur < LevelManager::TOTAL_LEVELS && cur < levels.get_unlocked_levels()) {
                    levels.set_current_level(cur + 1);
                    state = ScreenState::Playing;
                    return ScreenAction::NextLevel;
                }
                state = ScreenState::LevelSelect;
                return ScreenAction::ReturnToTitle;
            }
            if (input.restart_pressed) {
                state = ScreenState::Playing;
                return ScreenAction::RestartGame;
            }
            if (input.menu_pressed) {
                state = ScreenState::LevelSelect;
                return ScreenAction::ReturnToTitle;
            }
            break;

        case ScreenState::GameOver:
            if (input.restart_pressed || input.confirm_pressed) {
                state = ScreenState::Playing;
                return ScreenAction::RestartGame;
            }
            if (input.menu_pressed) {
                state = (levels.get_mode() == GameMode::Levels) ? ScreenState::LevelSelect : ScreenState::Start;
                return ScreenAction::ReturnToTitle;
            }
            break;
    }

    return ScreenAction::None;
}

std::string_view ScreenManager::get_state_name() const noexcept {
    switch (state) {
        case ScreenState::Start:         return "Start";
        case ScreenState::LevelSelect:   return "LevelSelect";
        case ScreenState::Playing:       return "Playing";
        case ScreenState::Paused:        return "Paused";
        case ScreenState::LevelComplete: return "LevelComplete";
        case ScreenState::GameOver:      return "GameOver";
    }
    return "Unknown";
}

void ScreenManager::draw_hud(
    const Spaceship& spaceship,
    const ProgressionState& progression,
    const LevelManager& levels,
    const EnemyField& enemies,
    const PowerupField& powerups,
    const AsteroidField& asteroids,
    Vector2 view_center
) const noexcept {
    const i32 w{ GetScreenWidth() };

    // 1. Top-Left Hull Health Bar + Active Powerup Timers
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

    char hp_buf[32]{ };
    std::snprintf(hp_buf, sizeof(hp_buf), "HULL %d/%d", spaceship.get_health(), spaceship.get_max_health());
    DrawText(hp_buf, bar_x + 6, bar_y + 3, 10, hex_to_color(0xf8fafcff));

    // Phase Dash Cooldown Bar & Strafe-Lock Badge
    i32 pw_y{ bar_y + bar_h + 6 };
    const f32 dash_cd{ spaceship.get_dash_cooldown() };
    const f32 dash_ready_ratio{
        1.0f - std::clamp(dash_cd / Spaceship::DASH_COOLDOWN, 0.0f, 1.0f)
    };
    const Color dash_color{
        dash_cd <= 0.0f ? hex_to_color(0x38bdf8ff) : hex_to_color(0x475569ff)
    };
    DrawRectangle(bar_x, pw_y, bar_w, 10, hex_to_color(0x0f172aff));
    DrawRectangle(bar_x, pw_y, static_cast<i32>(static_cast<f32>(bar_w) * dash_ready_ratio), 10, dash_color);
    DrawRectangleLines(bar_x, pw_y, bar_w, 10, hex_to_color(0x38bdf8ff));
    if (dash_cd <= 0.0f) {
        DrawText(
            spaceship.is_strafe_locked() ? "DASH READY [SHIFT] | LOCK" : "PHASE DASH READY [SHIFT]",
            bar_x + 4,
            pw_y,
            10,
            hex_to_color(0x0f172aff)
        );
    } else {
        char dash_buf[32]{ };
        std::snprintf(dash_buf, sizeof(dash_buf), "DASH COOLDOWN %.1fs", static_cast<f64>(dash_cd));
        DrawText(dash_buf, bar_x + 4, pw_y, 10, hex_to_color(0xe2e8f0ff));
    }
    pw_y += 14;

    // Active Powerup Timer Bars underneath Dash Bar
    constexpr f32 max_pw_duration{ 10.0f };

    const f32 dbl_rem{ spaceship.get_powerup_remaining(PowerupType::DoubleBlaster) };
    if (dbl_rem > 0.0f) {
        const f32 pw_ratio{ std::clamp(dbl_rem / max_pw_duration, 0.0f, 1.0f) };
        DrawRectangle(bar_x, pw_y, bar_w, 10, hex_to_color(0x0f172aff));
        DrawRectangle(bar_x, pw_y, static_cast<i32>(static_cast<f32>(bar_w) * pw_ratio), 10, hex_to_color(0xf59e0bff));
        DrawRectangleLines(bar_x, pw_y, bar_w, 10, hex_to_color(0xfbbf24ff));
        char buf[32]{ };
        std::snprintf(buf, sizeof(buf), "DOUBLE BLASTER %.1fs", static_cast<f64>(dbl_rem));
        DrawText(buf, bar_x + 4, pw_y, 10, hex_to_color(0xfef9c3ff));
        pw_y += 14;
    }

    const f32 shld_rem{ spaceship.get_powerup_remaining(PowerupType::Shield) };
    if (shld_rem > 0.0f) {
        const f32 pw_ratio{ std::clamp(shld_rem / max_pw_duration, 0.0f, 1.0f) };
        DrawRectangle(bar_x, pw_y, bar_w, 10, hex_to_color(0x0f172aff));
        DrawRectangle(bar_x, pw_y, static_cast<i32>(static_cast<f32>(bar_w) * pw_ratio), 10, hex_to_color(0x0284c7ff));
        DrawRectangleLines(bar_x, pw_y, bar_w, 10, hex_to_color(0x38bdf8ff));
        char buf[32]{ };
        std::snprintf(buf, sizeof(buf), "SHIELD HALO %.1fs", static_cast<f64>(shld_rem));
        DrawText(buf, bar_x + 4, pw_y, 10, hex_to_color(0xe0f2feff));
        pw_y += 14;
    }

    // 2. Top-Center Score, Mode/Level Objective & Kill-Chain Multiplier
    char score_buf[96]{ };
    if (levels.get_mode() == GameMode::Levels) {
        const LevelDefinition& def{ levels.get_definition() };
        std::snprintf(
            score_buf,
            sizeof(score_buf),
            "SCORE %06u   |   LVL %u: %s",
            static_cast<u32>(progression.score),
            def.level_number,
            def.title.data()
        );
    } else {
        std::snprintf(
            score_buf,
            sizeof(score_buf),
            "SCORE %06u   |   WAVE %u",
            static_cast<u32>(progression.score),
            progression.current_wave
        );
    }
    _draw_centered_text(score_buf, 14, 17, hex_to_color(0xf8fafcff));

    i32 hud_center_y{ 36 };
    if (levels.get_mode() == GameMode::Levels) {
        char obj_buf[96]{ };
        levels.format_objective_status(obj_buf, sizeof(obj_buf));
        _draw_centered_text(obj_buf, hud_center_y, 13, hex_to_color(0x38bdf8ff));
        hud_center_y += 16;

        constexpr i32 obj_w{ 220 };
        constexpr i32 obj_h{ 6 };
        const i32 obj_x{ (w - obj_w) / 2 };
        const f32 obj_ratio{ levels.get_objective_progress_ratio() };
        DrawRectangle(obj_x, hud_center_y, obj_w, obj_h, hex_to_color(0x0f172aff));
        DrawRectangle(
            obj_x,
            hud_center_y,
            static_cast<i32>(static_cast<f32>(obj_w) * obj_ratio),
            obj_h,
            levels.is_portal_active() ? hex_to_color(0xc084fcff) : hex_to_color(0xfacc15ff)
        );
        DrawRectangleLines(obj_x, hud_center_y, obj_w, obj_h, hex_to_color(0x334155ff));
        hud_center_y += 10;
    }

    if (progression.combo_multiplier > 1.0f && progression.combo_timer > 0.0f) {
        char combo_buf[48]{ };
        std::snprintf(combo_buf, sizeof(combo_buf), "x%.1f COMBO", static_cast<f64>(progression.combo_multiplier));
        _draw_centered_text(combo_buf, hud_center_y, 14, hex_to_color(0xfef08aff));

        constexpr i32 c_w{ 110 };
        constexpr i32 c_h{ 4 };
        const i32 c_x{ (w - c_w) / 2 };
        const f32 c_ratio{ std::clamp(progression.combo_timer / ProgressionState::COMBO_DURATION, 0.0f, 1.0f) };
        DrawRectangle(c_x, hud_center_y + 16, c_w, c_h, hex_to_color(0x1e293bff));
        DrawRectangle(c_x, hud_center_y + 16, static_cast<i32>(static_cast<f32>(c_w) * c_ratio), c_h, hex_to_color(0xfef08aff));
        hud_center_y += 24;
    }

    // Active Death Zone Lockdown Banner
    const DeathZoneState& dz{ levels.get_death_zone() };
    if (dz.active) {
        const f32 pulse{ 0.55f + 0.45f * (0.5f + 0.5f * std::sinf(pulse_timer * 2.5f)) };
        char dz_buf[96]{ };
        if (dz.cause == DeathZoneCause::BattlestarWarden) {
            std::snprintf(
                dz_buf,
                sizeof(dz_buf),
                "! BATTLESTAR DEATH ZONE LOCKDOWN (%.1fs) — BORDERS LETHAL !",
                static_cast<f64>(dz.timer)
            );
            _draw_centered_text(dz_buf, hud_center_y + 4, 15, Fade(hex_to_color(0xc084fcff), pulse));
        } else {
            std::snprintf(
                dz_buf,
                sizeof(dz_buf),
                "! AMBUSH DEATH ZONE ACTIVE (%.1fs) — BORDERS LETHAL !",
                static_cast<f64>(dz.timer)
            );
            _draw_centered_text(dz_buf, hud_center_y + 4, 15, Fade(hex_to_color(0xef4444ff), pulse));
        }
        hud_center_y += 22;
    }

    // Wave / Level announcement banner
    if (progression.wave_banner_timer > 0.0f && state == ScreenState::Playing) {
        const f32 alpha{ std::clamp(progression.wave_banner_timer / 0.8f, 0.0f, 1.0f) };
        char wave_banner[96]{ };
        if (levels.get_mode() == GameMode::Levels) {
            const LevelDefinition& def{ levels.get_definition() };
            std::snprintf(wave_banner, sizeof(wave_banner), "LEVEL %u: %s", def.level_number, def.title.data());
        } else {
            std::snprintf(wave_banner, sizeof(wave_banner), "WAVE %u", progression.current_wave);
        }
        _draw_centered_text(wave_banner, hud_center_y + 10, 24, Fade(hex_to_color(0x38bdf8ff), alpha));
    }

    // 3. Off-Screen Threat, Objective & Loot Edge Indicators
    _draw_edge_indicators(levels, enemies, powerups, view_center);

    // 4. Bottom-Right Circular Tactical Radar
    _draw_radar(spaceship, levels, enemies, powerups, asteroids, view_center);
}

void ScreenManager::_draw_edge_indicators(
    const LevelManager& levels,
    const EnemyField& enemies,
    const PowerupField& powerups,
    Vector2 view_center
) const noexcept {
    const f32 w{ static_cast<f32>(GetScreenWidth()) };
    const f32 h{ static_cast<f32>(GetScreenHeight()) };
    const f32 half_w{ w * 0.5f };
    const f32 half_h{ h * 0.5f };
    constexpr f32 margin{ 22.0f };
    const f32 max_x{ half_w - margin };
    const f32 max_y{ half_h - margin };

    auto draw_edge_marker = [&](Vector2 world_pos, Color color, f32 base_radius, i32 sides = 3) noexcept {
        const Vector2 delta{ Vector2Subtract(world_pos, view_center) };
        if (std::fabsf(delta.x) <= half_w && std::fabsf(delta.y) <= half_h) {
            return; // Already visible on screen
        }

        const f32 dist{ Vector2Length(delta) };
        if (dist < 1.0f) return;

        const f32 tx{ std::fabsf(delta.x) > 0.001f ? max_x / std::fabsf(delta.x) : 10000.0f };
        const f32 ty{ std::fabsf(delta.y) > 0.001f ? max_y / std::fabsf(delta.y) : 10000.0f };
        const f32 t{ std::fminf(tx, ty) };

        const Vector2 screen_pos{
            half_w + delta.x * t,
            half_h + delta.y * t
        };
        const f32 angle_deg{ std::atan2f(delta.y, delta.x) * RAD2DEG };
        const f32 dist_alpha{ std::clamp(1.15f - (dist - half_w) / 900.0f, 0.35f, 0.95f) };

        DrawPoly(screen_pos, sides, base_radius, angle_deg, Fade(color, dist_alpha));
        DrawPolyLinesEx(screen_pos, sides, base_radius + 1.5f, angle_deg, 1.5f, Fade(WHITE, dist_alpha * 0.6f));
    };

    if (levels.get_mode() == GameMode::Levels) {
        for (const ObjectiveEntity& obj : levels.get_entities()) {
            if (!obj.active || (obj.type == ObjectiveEntityType::Beacon && obj.completed)) continue;
            const Color obj_col{
                obj.type == ObjectiveEntityType::Star   ? hex_to_color(0xfacc15ff) :
                obj.type == ObjectiveEntityType::Beacon ? hex_to_color(0x38bdf8ff) :
                                                          hex_to_color(0xc084fcff)
            };
            draw_edge_marker(obj.pos, obj_col, 10.5f, 4);
        }
    }

    for (const EnemySpaceship& enemy : enemies.get_enemies()) {
        if (!enemy.is_alive()) continue;
        if (enemy.get_archetype() == EnemyArchetype::Battlestar) {
            const Vector2 delta{ Vector2Subtract(enemy.get_pos(), view_center) };
            if (std::fabsf(delta.x) > half_w || std::fabsf(delta.y) > half_h) {
                const f32 tx{ std::fabsf(delta.x) > 0.001f ? max_x / std::fabsf(delta.x) : 10000.0f };
                const f32 ty{ std::fabsf(delta.y) > 0.001f ? max_y / std::fabsf(delta.y) : 10000.0f };
                const f32 t{ std::fminf(tx, ty) };
                const Vector2 screen_pos{ half_w + delta.x * t, half_h + delta.y * t };
                const f32 pulse{ 1.0f + 0.2f * std::sinf(pulse_timer * 2.5f) };
                DrawPoly(screen_pos, 4, 12.5f * pulse, 0.0f, hex_to_color(0xa855f7ee));
                DrawPolyLinesEx(screen_pos, 4, 14.0f * pulse, 0.0f, 2.0f, hex_to_color(0xfacc15ff));
                DrawCircleV(screen_pos, 4.0f, hex_to_color(0xa3e635ff));
            }
        } else {
            draw_edge_marker(enemy.get_pos(), hex_to_color(0xef4444ff), 8.5f);
        }
    }

    for (const Powerup& p : powerups.get_powerups()) {
        const Color pw_color{
            p.type == PowerupType::Shield         ? hex_to_color(0x38bdf8ff) :
            p.type == PowerupType::IncreaseHealth ? hex_to_color(0x22c55eff) :
                                                    hex_to_color(0xfbbf24ff)
        };
        draw_edge_marker(p.pos, pw_color, 7.5f);
    }
}

void ScreenManager::_draw_radar(
    const Spaceship& spaceship,
    const LevelManager& levels,
    const EnemyField& enemies,
    const PowerupField& powerups,
    const AsteroidField& asteroids,
    Vector2 view_center
) const noexcept {
    const f32 w{ static_cast<f32>(GetScreenWidth()) };
    const f32 h{ static_cast<f32>(GetScreenHeight()) };
    constexpr f32 radar_radius{ 48.0f };
    constexpr f32 world_range{ 1050.0f };
    constexpr f32 scale{ radar_radius / world_range };
    const Vector2 center{ w - radar_radius - 16.0f, h - radar_radius - 16.0f };

    // Radar background & range rings
    DrawCircleV(center, radar_radius, Fade(hex_to_color(0x0f172aff), 0.78f));
    DrawRing(center, radar_radius * 0.5f - 0.5f, radar_radius * 0.5f + 0.5f, 0.0f, 360.0f, 32, hex_to_color(0x1e293bff));

    const Color rim_col{
        levels.get_death_zone().active ? hex_to_color(0xef4444ff) : hex_to_color(0x334155ff)
    };
    DrawRing(center, radar_radius - 2.0f, radar_radius, 0.0f, 360.0f, 40, rim_col);
    DrawLineEx(
        Vector2{ center.x - radar_radius + 4.0f, center.y },
        Vector2{ center.x + radar_radius - 4.0f, center.y },
        1.0f,
        hex_to_color(0x1e293bff)
    );
    DrawLineEx(
        Vector2{ center.x, center.y - radar_radius + 4.0f },
        Vector2{ center.x, center.y + radar_radius - 4.0f },
        1.0f,
        hex_to_color(0x1e293bff)
    );

    auto to_radar = [&](Vector2 world_pos) noexcept -> Vector2 {
        const Vector2 rel{ Vector2Scale(Vector2Subtract(world_pos, view_center), scale) };
        const f32 len{ Vector2Length(rel) };
        if (len > radar_radius - 3.0f) {
            return Vector2Add(center, Vector2Scale(rel, (radar_radius - 3.0f) / len));
        }
        return Vector2Add(center, rel);
    };

    // Asteroid blips (color-coded by variant)
    for (const Asteroid& ast : asteroids.get_asteroids()) {
        if (Vector2Distance(ast.get_pos(), view_center) <= world_range) {
            const Color blip_color{
                ast.get_variant() == AsteroidVariant::Volatile ? hex_to_color(0xf97316ff) :
                ast.get_variant() == AsteroidVariant::GoldCore ? hex_to_color(0xfacc15ff) :
                                                                 hex_to_color(0x64748bcc)
            };
            DrawCircleV(to_radar(ast.get_pos()), 2.0f, blip_color);
        }
    }

    // Powerup blips (pulsing green)
    for (const Powerup& p : powerups.get_powerups()) {
        DrawCircleV(to_radar(p.pos), 2.8f, hex_to_color(0x22c55eff));
    }

    // Objective blips (Star, Beacon, Portal)
    if (levels.get_mode() == GameMode::Levels) {
        const f32 pulse{ 1.0f + 0.2f * std::sinf(pulse_timer * 2.2f) };
        for (const ObjectiveEntity& obj : levels.get_entities()) {
            if (!obj.active) continue;
            const Vector2 o_blip{ to_radar(obj.pos) };
            if (obj.type == ObjectiveEntityType::Star) {
                DrawPoly(o_blip, 4, 5.5f * pulse, 45.0f, hex_to_color(0xfacc15ff));
                DrawCircleV(o_blip, 2.2f, hex_to_color(0x38bdf8ff));
            } else if (obj.type == ObjectiveEntityType::Beacon) {
                const Color b_col{ obj.completed ? hex_to_color(0x4ade80ff) : hex_to_color(0x38bdf8ff) };
                DrawRing(o_blip, 3.2f, 5.2f * pulse, 0.0f, 360.0f, 20, b_col);
                DrawCircleV(o_blip, 2.0f, WHITE);
            } else if (obj.type == ObjectiveEntityType::Portal) {
                DrawPoly(o_blip, 6, 6.0f * pulse, 0.0f, hex_to_color(0xa855f7ff));
                DrawCircleV(o_blip, 2.5f, hex_to_color(0x38bdf8ff));
            }
        }
    }

    // Enemy blips (bright crimson for standard ships, prominent diamond-cross emblem for Battlestar)
    for (const EnemySpaceship& enemy : enemies.get_enemies()) {
        if (!enemy.is_alive()) continue;
        const Vector2 e_blip{ to_radar(enemy.get_pos()) };
        if (enemy.get_archetype() == EnemyArchetype::Battlestar) {
            const f32 pulse{ 1.0f + 0.18f * std::sinf(pulse_timer * 2.5f) };
            DrawPoly(e_blip, 4, 7.5f * pulse, 0.0f, hex_to_color(0xa855f7ff));
            DrawPolyLinesEx(e_blip, 4, 8.8f * pulse, 0.0f, 1.8f, hex_to_color(0xfacc15ff));
            DrawLineEx(
                Vector2{ e_blip.x - 9.5f * pulse, e_blip.y },
                Vector2{ e_blip.x + 9.5f * pulse, e_blip.y },
                1.5f,
                hex_to_color(0xfacc15ff)
            );
            DrawLineEx(
                Vector2{ e_blip.x, e_blip.y - 9.5f * pulse },
                Vector2{ e_blip.x, e_blip.y + 9.5f * pulse },
                1.5f,
                hex_to_color(0xfacc15ff)
            );
            DrawCircleV(e_blip, 2.5f, hex_to_color(0xa3e635ff));
        } else {
            DrawCircleV(e_blip, 3.2f, hex_to_color(0xef4444ff));
        }
    }

    // Player center blip + heading tick
    if (spaceship.is_alive()) {
        const Vector2 p_blip{ to_radar(spaceship.get_pos()) };
        const f32 rad{ spaceship.get_rotation() * DEG2RAD };
        const Vector2 head_end{
            p_blip.x + std::sinf(rad) * 7.0f,
            p_blip.y - std::cosf(rad) * 7.0f
        };
        DrawLineEx(p_blip, head_end, 1.5f, hex_to_color(0x38bdf8ff));
        DrawCircleV(p_blip, 3.0f, hex_to_color(0xfe166dff));
    }
}

void ScreenManager::draw_overlay(
    const ProgressionState& progression,
    const LevelManager& levels
) const noexcept {
    switch (state) {
        case ScreenState::Start:
            _draw_start(progression, levels);
            break;
        case ScreenState::LevelSelect:
            _draw_level_select(levels);
            break;
        case ScreenState::Paused:
            _draw_paused(levels);
            break;
        case ScreenState::LevelComplete:
            _draw_level_complete(progression, levels);
            break;
        case ScreenState::GameOver:
            _draw_game_over(progression, levels);
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

void ScreenManager::_draw_start(
    const ProgressionState& progression,
    const LevelManager& levels
) const noexcept {
    const i32 w{ GetScreenWidth() };
    const i32 h{ GetScreenHeight() };
    DrawRectangle(0, 0, w, h, Fade(BLACK, 0.50f));

    const f32 pulse_alpha{ 0.55f + 0.45f * (0.5f + 0.5f * std::sinf(pulse_timer)) };

    _draw_centered_text("ASTEROIDS ON STEROIDS", h / 2 - 125, 44, hex_to_color(0xfe166dff));
    _draw_centered_text(
        "WASD: Move | SPACE: Fire | SHIFT: Dash | K/RMB: Strafe Lock | P: Pause",
        h / 2 - 65,
        15,
        hex_to_color(0x94a3b8ff)
    );

    // Mode Selection Cards
    const bool is_freestyle{ levels.get_mode() == GameMode::Freestyle };
    constexpr i32 box_w{ 360 };
    constexpr i32 box_h{ 36 };
    const i32 box_x{ (w - box_w) / 2 };
    const i32 free_y{ h / 2 - 22 };
    const i32 lvl_y{ h / 2 + 24 };

    DrawRectangle(box_x, free_y, box_w, box_h, is_freestyle ? hex_to_color(0x1e293bff) : hex_to_color(0x0f172acc));
    DrawRectangleLines(box_x, free_y, box_w, box_h, is_freestyle ? hex_to_color(0x38bdf8ff) : hex_to_color(0x334155ff));
    _draw_centered_text(
        is_freestyle ? "> FREESTYLE MODE (ENDLESS WAVES) <" : "FREESTYLE MODE (ENDLESS WAVES)",
        free_y + 10,
        16,
        is_freestyle ? hex_to_color(0x38bdf8ff) : hex_to_color(0x94a3b8ff)
    );

    DrawRectangle(box_x, lvl_y, box_w, box_h, !is_freestyle ? hex_to_color(0x1e293bff) : hex_to_color(0x0f172acc));
    DrawRectangleLines(box_x, lvl_y, box_w, box_h, !is_freestyle ? hex_to_color(0xfacc15ff) : hex_to_color(0x334155ff));
    _draw_centered_text(
        !is_freestyle ? "> LEVELS MODE (5-STAGE CAMPAIGN) <" : "LEVELS MODE (5-STAGE CAMPAIGN)",
        lvl_y + 10,
        16,
        !is_freestyle ? hex_to_color(0xfacc15ff) : hex_to_color(0x94a3b8ff)
    );

    if (progression.high_score > 0) {
        char best_buf[64]{ };
        std::snprintf(best_buf, sizeof(best_buf), "HIGH SCORE: %06u", static_cast<u32>(progression.high_score));
        _draw_centered_text(best_buf, h / 2 + 74, 17, hex_to_color(0xfef08aff));
    }

    _draw_centered_text(
        "W/S or ARROWS: Switch Mode   |   ENTER or SPACE: Select",
        h / 2 + 108,
        20,
        Fade(hex_to_color(0xf8fafcff), pulse_alpha)
    );
}

void ScreenManager::_draw_level_select(const LevelManager& levels) const noexcept {
    const i32 w{ GetScreenWidth() };
    const i32 h{ GetScreenHeight() };
    DrawRectangle(0, 0, w, h, Fade(BLACK, 0.65f));

    _draw_centered_text("SELECT CAMPAIGN LEVEL", 48, 34, hex_to_color(0xfacc15ff));
    _draw_centered_text(
        "Complete objectives & survive Death Zone lockdowns to unlock the next sector",
        90,
        15,
        hex_to_color(0x94a3b8ff)
    );

    constexpr i32 card_w{ 620 };
    constexpr i32 card_h{ 64 };
    const i32 card_x{ (w - card_w) / 2 };
    i32 y{ 126 };

    const u32 selected{ levels.get_current_level() };
    const u32 unlocked{ levels.get_unlocked_levels() };

    for (u32 i{ 1 }; i <= LevelManager::TOTAL_LEVELS; ++i) {
        const LevelDefinition& def{ LevelManager::get_definition(i) };
        const bool is_sel{ i == selected };
        const bool is_unl{ i <= unlocked };
        const bool is_done{ levels.is_level_completed_once(i) };

        const Color bg_col{
            is_sel ? hex_to_color(0x1e293bff) :
            is_unl ? hex_to_color(0x0f172aee) :
                     hex_to_color(0x090d16cc)
        };
        const Color border_col{
            is_sel  ? hex_to_color(0x38bdf8ff) :
            is_done ? hex_to_color(0x4ade80ff) :
            is_unl  ? hex_to_color(0x475569ff) :
                      hex_to_color(0x1e293bff)
        };

        DrawRectangle(card_x, y, card_w, card_h, bg_col);
        DrawRectangleLines(card_x, y, card_w, card_h, border_col);

        char header_buf[96]{ };
        std::snprintf(
            header_buf,
            sizeof(header_buf),
            "%sLEVEL %u: %s",
            is_sel ? "> " : "  ",
            def.level_number,
            def.title.data()
        );
        DrawText(
            header_buf,
            card_x + 14,
            y + 10,
            18,
            is_unl ? hex_to_color(0xf8fafcff) : hex_to_color(0x64748bff)
        );

        DrawText(
            def.objective_summary.data(),
            card_x + 28,
            y + 36,
            14,
            is_unl ? hex_to_color(0xbae6fdff) : hex_to_color(0x475569ff)
        );

        const char* status_str{
            is_done ? "COMPLETED *" :
            is_unl  ? "UNLOCKED" :
                      "LOCKED"
        };
        const Color status_col{
            is_done ? hex_to_color(0x4ade80ff) :
            is_unl  ? hex_to_color(0xfacc15ff) :
                      hex_to_color(0x64748bff)
        };
        const i32 st_w{ MeasureText(status_str, 14) };
        DrawText(status_str, card_x + card_w - st_w - 16, y + 12, 14, status_col);

        y += card_h + 12;
    }

    const f32 pulse_alpha{ 0.55f + 0.45f * (0.5f + 0.5f * std::sinf(pulse_timer)) };
    _draw_centered_text(
        "W/S: Select Level   |   ENTER/SPACE: Launch   |   M/ESC: Back",
        h - 46,
        18,
        Fade(hex_to_color(0xf8fafcff), pulse_alpha)
    );
}

void ScreenManager::_draw_paused(const LevelManager& levels) const noexcept {
    const i32 w{ GetScreenWidth() };
    const i32 h{ GetScreenHeight() };
    DrawRectangle(0, 0, w, h, Fade(BLACK, 0.55f));

    _draw_centered_text("PAUSED", h / 2 - 80, 48, hex_to_color(0x38bdf8ff));
    _draw_centered_text("Press P / ESC / ENTER to Resume", h / 2 - 10, 22, hex_to_color(0xf8fafcff));
    _draw_centered_text("Press R to Restart", h / 2 + 26, 20, hex_to_color(0x94a3b8ff));
    _draw_centered_text(
        levels.get_mode() == GameMode::Levels ? "Press M for Level Select" : "Press M for Main Menu",
        h / 2 + 58,
        20,
        hex_to_color(0x94a3b8ff)
    );
}

void ScreenManager::_draw_level_complete(
    const ProgressionState& progression,
    const LevelManager& levels
) const noexcept {
    const i32 w{ GetScreenWidth() };
    const i32 h{ GetScreenHeight() };
    DrawRectangle(0, 0, w, h, Fade(BLACK, 0.60f));

    const f32 pulse_alpha{ 0.55f + 0.45f * (0.5f + 0.5f * std::sinf(pulse_timer)) };
    const LevelDefinition& def{ levels.get_definition() };

    char title_buf[96]{ };
    std::snprintf(title_buf, sizeof(title_buf), "LEVEL %u COMPLETE!", def.level_number);
    _draw_centered_text(title_buf, h / 2 - 96, 44, hex_to_color(0x4ade80ff));
    _draw_centered_text(def.title.data(), h / 2 - 42, 22, hex_to_color(0x38bdf8ff));

    char score_buf[96]{ };
    std::snprintf(
        score_buf,
        sizeof(score_buf),
        "SECTOR SCORE: %06u   |   BEST: %06u",
        static_cast<u32>(progression.score),
        static_cast<u32>(progression.high_score)
    );
    _draw_centered_text(score_buf, h / 2 + 2, 18, hex_to_color(0xfef08aff));

    if (def.level_number < LevelManager::TOTAL_LEVELS) {
        _draw_centered_text(
            "Press ENTER or SPACE for Next Level",
            h / 2 + 44,
            22,
            Fade(hex_to_color(0xf8fafcff), pulse_alpha)
        );
    } else {
        _draw_centered_text(
            "CAMPAIGN VICTORY! Press ENTER for Level Select",
            h / 2 + 44,
            22,
            Fade(hex_to_color(0xfacc15ff), pulse_alpha)
        );
    }
    _draw_centered_text("Press R to Replay Level   |   Press M for Level Select", h / 2 + 80, 18, hex_to_color(0x94a3b8ff));
}

void ScreenManager::_draw_game_over(
    const ProgressionState& progression,
    const LevelManager& levels
) const noexcept {
    const i32 w{ GetScreenWidth() };
    const i32 h{ GetScreenHeight() };
    DrawRectangle(0, 0, w, h, Fade(BLACK, 0.55f));

    const f32 pulse_alpha{ 0.55f + 0.45f * (0.5f + 0.5f * std::sinf(pulse_timer)) };

    _draw_centered_text("GAME OVER", h / 2 - 90, 48, hex_to_color(0xfe166dff));

    char summary[96]{ };
    if (levels.get_mode() == GameMode::Levels) {
        std::snprintf(
            summary,
            sizeof(summary),
            "FINAL SCORE: %06u   |   BEST: %06u   |   LEVEL %u",
            static_cast<u32>(progression.score),
            static_cast<u32>(progression.high_score),
            levels.get_current_level()
        );
    } else {
        std::snprintf(
            summary,
            sizeof(summary),
            "FINAL SCORE: %06u   |   BEST: %06u   |   WAVE %u",
            static_cast<u32>(progression.score),
            static_cast<u32>(progression.high_score),
            progression.current_wave
        );
    }
    _draw_centered_text(summary, h / 2 - 22, 18, hex_to_color(0xfef08aff));

    _draw_centered_text("Press R or ENTER to Restart", h / 2 + 24, 22, Fade(hex_to_color(0xf8fafcff), pulse_alpha));
    _draw_centered_text(
        levels.get_mode() == GameMode::Levels ? "Press M for Level Select" : "Press M for Main Menu",
        h / 2 + 60,
        20,
        hex_to_color(0x94a3b8ff)
    );
}

} // namespace asteroides
