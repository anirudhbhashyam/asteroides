#include "spaceship.hh"

namespace asteroides {

void Spaceship::draw() const noexcept {
    blaster.draw();
    if (!is_alive()) return;

    if (sprite.is_valid()) {
        if ((thrusting || Vector2LengthSqr(vel) > 900.0f) && flame_sprite.is_valid()) {
            const f32 rad{ rotation * DEG2RAD };
            const Vector2 forward{ std::sinf(rad), -std::cosf(rad) };
            const Vector2 right{ std::cosf(rad), std::sinf(rad) };
            const u32 main_idx{ is_dashing() ? 2u : (flame_frame % FLAME_FRAME_COUNT) };
            const u32 side_idx{ (main_idx + 1u) % FLAME_FRAME_COUNT };

            const Rectangle main_src{
                static_cast<f32>(main_idx) * SPRITE_SIZE,
                0.0f,
                SPRITE_SIZE,
                SPRITE_SIZE
            };
            const Rectangle side_src{
                static_cast<f32>(side_idx) * SPRITE_SIZE,
                0.0f,
                SPRITE_SIZE,
                SPRITE_SIZE
            };

            const f32 speed_factor{ std::clamp(Vector2Length(vel) / 400.0f, 0.35f, 1.0f) };
            const f32 alpha{ thrusting ? 0.95f : (0.55f * speed_factor) };
            const Color flame_tint{
                is_dashing() ? Fade(hex_to_color(0x7dd3fcff), 1.0f) : Fade(WHITE, alpha)
            };

            // Dual side thruster plumes aligned to the purple rear nozzles (dx = ±6px in sprite space)
            const f32 side_scale{ scale * (is_dashing() ? 0.78f : 0.62f) };
            const Vector2 side_back{ Vector2Scale(forward, -14.5f * scale) };
            const Vector2 left_nozzle{
                Vector2Add(pos, Vector2Add(side_back, Vector2Scale(right, -5.5f * scale)))
            };
            const Vector2 right_nozzle{
                Vector2Add(pos, Vector2Add(side_back, Vector2Scale(right, 5.5f * scale)))
            };
            flame_sprite.draw(left_nozzle, side_src, rotation, side_scale, flame_tint);
            flame_sprite.draw(right_nozzle, side_src, rotation, side_scale, flame_tint);

            // Central main rocket propeller plume
            if (thrusting) {
                const f32 main_scale{ scale * (is_dashing() ? 1.15f : 0.92f) };
                const Vector2 main_nozzle{
                    Vector2Subtract(pos, Vector2Scale(forward, 17.0f * scale))
                };
                flame_sprite.draw(main_nozzle, main_src, rotation, main_scale, flame_tint);
            }
        }

        if (is_dashing()) {
            const Vector2 ghost_pos{ Vector2Subtract(pos, Vector2Scale(vel, 0.025f)) };
            sprite.draw(ghost_pos, FRAME_UP, rotation, scale, Fade(hex_to_color(0x38bdf8ff), 0.45f));
        }
        const Color tint{
            hit_flash_timer > 0.0f ? hex_to_color(0xff5555ff) :
            is_dashing()           ? hex_to_color(0x7dd3fcff) :
                                     WHITE
        };
        sprite.draw(pos, FRAME_UP, rotation, scale, tint);
    } else {
        DrawCircleV(pos, radius, color);
    }

    if (!is_shielded()) return;

    const f32 shield_remaining{ get_powerup_remaining(PowerupType::Shield) };
    const f32 pulse{ std::sinf(halo_pulse_timer) };
    const f32 halo_radius{ get_radius() + 8.0f + 1.5f * pulse };
    const f32 expiry_fade{
        shield_remaining < 2.0f
            ? (0.45f + 0.55f * (0.5f + 0.5f * std::sinf(halo_pulse_timer * 3.0f)))
            : 1.0f
    };

    DrawCircleGradient(
        pos,
        halo_radius + 3.0f,
        Fade(hex_to_color(0x5eead4ff), 0.06f * expiry_fade),
        Fade(hex_to_color(0x38bdf8ff), 0.28f * expiry_fade)
    );
    DrawRing(
        pos,
        halo_radius - 2.5f,
        halo_radius + 1.0f,
        0.0f,
        360.0f,
        40,
        Fade(hex_to_color(0x5eead4ff), 0.85f * expiry_fade)
    );
    DrawRing(
        pos,
        halo_radius + 1.0f,
        halo_radius + 3.0f,
        0.0f,
        360.0f,
        40,
        Fade(hex_to_color(0x60a5faff), 0.45f * expiry_fade)
    );
}

void Spaceship::take_damage(i32 amount, bool grant_invulnerability) noexcept {
    if (!is_alive() || is_shielded()) return;
    if (grant_invulnerability && invulnerability_timer > 0.0f) return;

    health = (health > amount) ? (health - amount) : 0;
    hit_flash_timer = 0.18f;
    if (grant_invulnerability) {
        invulnerability_timer = 0.6f;
    }
}

void Spaceship::heal_percent(f32 fraction) noexcept {
    if (!is_alive()) return;
    const i32 heal_amount{ static_cast<i32>(static_cast<f32>(max_health) * fraction) };
    health = std::min(max_health, health + heal_amount);
}

void Spaceship::activate_powerup(PowerupType type, f32 duration) noexcept {
    const u64 idx{ static_cast<u64>(type) };
    if (idx < POWERUP_COUNT) {
        powerup_timers[idx] = std::fmaxf(powerup_timers[idx], duration);
    }
}

bool Spaceship::is_powerup_active(PowerupType type) const noexcept {
    const u64 idx{ static_cast<u64>(type) };
    return (idx < POWERUP_COUNT) && (powerup_timers[idx] > 0.0f);
}

f32 Spaceship::get_powerup_remaining(PowerupType type) const noexcept {
    const u64 idx{ static_cast<u64>(type) };
    return (idx < POWERUP_COUNT) ? powerup_timers[idx] : 0.0f;
}

void Spaceship::apply_impulse(Vector2 impulse) noexcept {
    vel = Vector2Add(vel, impulse);
}

void Spaceship::refund_dash_cooldown(f32 fraction) noexcept {
    dash_cooldown_timer = std::fmaxf(0.0f, dash_cooldown_timer - DASH_COOLDOWN * fraction);
}

void Spaceship::reset() noexcept {
    pos = Vector2{
        static_cast<f32>(GetScreenWidth() / 2),
        static_cast<f32>(GetScreenHeight() / 2)
    };
    vel = Vector2Zero();
    rotation = 0.0f;
    health = max_health;
    invulnerability_timer = 0.0f;
    hit_flash_timer = 0.0f;
    dash_timer = 0.0f;
    dash_cooldown_timer = 0.0f;
    powerup_timers.fill(0.0f);
    halo_pulse_timer = 0.0f;
    flame_anim_timer = 0.0f;
    flame_frame = 0;
    thrusting = false;
    strafe_locked = false;
    just_dashed = false;
    blaster.clear();
}

void Spaceship::update(const f32 dt, const InputState& input) noexcept {
    if (invulnerability_timer > 0.0f) {
        invulnerability_timer = std::fmaxf(0.0f, invulnerability_timer - dt);
    }
    if (hit_flash_timer > 0.0f) {
        hit_flash_timer = std::fmaxf(0.0f, hit_flash_timer - dt);
    }
    if (dash_timer > 0.0f) {
        dash_timer = std::fmaxf(0.0f, dash_timer - dt);
    }
    if (dash_cooldown_timer > 0.0f) {
        dash_cooldown_timer = std::fmaxf(0.0f, dash_cooldown_timer - dt);
    }

    // Common powerup timer update across all active powerup effects
    for (f32& timer : powerup_timers) {
        if (timer > 0.0f) {
            timer = std::fmaxf(0.0f, timer - dt);
        }
    }

    if (is_shielded()) {
        constexpr f32 two_pi{ 2.0f * std::numbers::pi_v<f32> };
        halo_pulse_timer = std::fmodf(halo_pulse_timer + 6.0f * dt, two_pi);
    }

    if (!is_alive()) {
        thrusting = false;
        blaster.update(dt, pos);
        return;
    }

    Vector2 directional_acc{
        input.move_axis.x * acc.x,
        input.move_axis.y * acc.y
    };
    f32 k_drag{ 0.01f };
    bool has_input{ directional_acc.x != 0.0f || directional_acc.y != 0.0f };
    thrusting = has_input || is_dashing();
    strafe_locked = input.strafe_lock_down;

    if (thrusting || Vector2LengthSqr(vel) > 900.0f) {
        const f32 anim_rate{ is_dashing() ? (FLAME_FPS * 1.6f) : FLAME_FPS };
        flame_anim_timer += dt * anim_rate;
        if (flame_anim_timer >= 1.0f) {
            const u32 steps{ static_cast<u32>(flame_anim_timer) };
            flame_anim_timer -= static_cast<f32>(steps);
            flame_frame = (flame_frame + steps) % FLAME_FRAME_COUNT;
        }
    } else {
        flame_anim_timer = 0.0f;
        flame_frame = 0;
    }

    if (has_input) {
        directional_acc = Vector2Multiply(Vector2Normalize(directional_acc), acc);
        if (!strafe_locked) {
            const f32 target_rotation{ std::atan2f(directional_acc.x, -directional_acc.y) * RAD2DEG };
            const f32 angle_diff{ std::remainderf(target_rotation - rotation, 360.0f) };
            rotation = std::remainderf(rotation + angle_diff * std::fminf(1.0f, turn_speed * dt), 360.0f);
        }
    }

    if (input.dash_pressed && dash_cooldown_timer <= 0.0f) {
        dash_timer = DASH_DURATION;
        dash_cooldown_timer = DASH_COOLDOWN;
        invulnerability_timer = std::fmaxf(invulnerability_timer, DASH_INVULN_DURATION);
        just_dashed = true;

        Vector2 dash_dir{ Vector2Zero() };
        if (has_input) {
            dash_dir = Vector2Normalize(input.move_axis);
        } else {
            const f32 rad{ rotation * DEG2RAD };
            dash_dir = Vector2{ std::sinf(rad), -std::cosf(rad) };
        }
        vel = Vector2Add(vel, Vector2Scale(dash_dir, DASH_IMPULSE));
    }
    Vector2 drag_acc{ Vector2Zero() };
    f32 speed{ Vector2Length(vel) };
    if (speed > 0 && dt > 0.0f) {
        f32 max_braking_acc{ speed / dt };
        f32 braking{ has_input ? 0.0f : 600.0f };
        f32 drag_magnitude{ std::fminf(0.5f * k_drag * speed * speed + braking, max_braking_acc) };
        drag_acc = Vector2Scale(Vector2Normalize(vel), -drag_magnitude);
    }
    Vector2 net_acc{ Vector2Add(directional_acc, drag_acc) };
    Vector2 s{
        Vector2Add(
            Vector2Scale(vel, dt),
            Vector2Scale(net_acc, 0.5f * dt * dt)
        )
    };
    pos = Vector2Add(pos, s);
    vel = Vector2Add(vel, Vector2Scale(net_acc, dt));

    if (!has_input && Vector2LengthSqr(vel) < 75.0f) vel = Vector2{ 0.0f, 0.0f };

    if (input.fire_down) {
        blaster.fire(pos, vel, rotation, is_powerup_active(PowerupType::DoubleBlaster));
    }
    blaster.update(dt, pos);
}

} // namespace asteroides
