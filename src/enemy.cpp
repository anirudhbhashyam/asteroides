#include "enemy.hh"

namespace asteroides {

EnemySpaceship::EnemySpaceship(EnemyArchetype type) {
    strafe_sign = (Random::uint(0, 1) == 0) ? 1.0f : -1.0f;
    _configure_archetype(type);
}

void EnemySpaceship::_configure_archetype(EnemyArchetype type) noexcept {
    archetype = type;
    switch (type) {
        case EnemyArchetype::Fighter:
            sprite = assets::Sprite{ ASSET_NAME };
            scale = 2.0f;
            max_health = 3;
            health = 3;
            max_speed = 210.0f;
            accel = 340.0f;
            turn_speed = 8.0f;
            preferred_range = 220.0f;
            base_tint = WHITE;
            blaster = Blaster{ BLASTER_ASSET_NAME, 430.0f, 1.15f };
            break;

        case EnemyArchetype::Kamikaze:
            sprite = assets::Sprite{ ASSET_NAME };
            scale = 1.45f;
            max_health = 1;
            health = 1;
            max_speed = 325.0f;
            accel = 520.0f;
            turn_speed = 5.2f;
            preferred_range = 0.0f;
            base_tint = hex_to_color(0xf97316ff);
            break;

        case EnemyArchetype::Sniper:
            sprite = assets::Sprite{ ASSET_NAME };
            scale = 1.85f;
            max_health = 2;
            health = 2;
            max_speed = 175.0f;
            accel = 280.0f;
            turn_speed = 6.5f;
            preferred_range = 340.0f;
            base_tint = hex_to_color(0xc084fcff);
            blaster = Blaster{ BLASTER_ASSET_NAME, 960.0f, 0.25f };
            break;

        case EnemyArchetype::Battlestar:
            sprite = assets::Sprite{ BATTLESTAR_ASSET_NAME };
            scale = 2.8f;
            max_health = 10;
            health = 10;
            max_speed = 115.0f;
            accel = 190.0f;
            turn_speed = 7.5f;
            preferred_range = 240.0f;
            base_tint = WHITE;
            blaster = Blaster{ BLASTER_ASSET_NAME, 440.0f, 0.45f, 1.15f, -90.0f, 4.0f };
            wave_blaster = Blaster{ WAVE_ATTACK_ASSET_NAME, 330.0f, 0.85f, 2.2f, 0.0f, 15.0f };
            battlestar_mode = BattlestarAttackMode::PointyNormal;
            mode_switch_timer = 2.4f;
            break;
    }
}

EnemySpaceship EnemySpaceship::spawn_from_edge(
    Vector2 view_center,
    i32 screen_w,
    i32 screen_h,
    EnemyArchetype type
) {
    EnemySpaceship enemy{ type };
    const f32 w{ static_cast<f32>(screen_w) };
    const f32 h{ static_cast<f32>(screen_h) };
    const f32 left{ view_center.x - w * 0.5f };
    const f32 right{ view_center.x + w * 0.5f };
    const f32 top{ view_center.y - h * 0.5f };
    const f32 bottom{ view_center.y + h * 0.5f };
    const f32 pad{ enemy.get_radius() + 32.0f };

    switch (Random::uint(0, 3)) {
        case 0:
            enemy.pos = Vector2{ Random::real(left, right), top - pad };
            break;
        case 1:
            enemy.pos = Vector2{ Random::real(left, right), bottom + pad };
            break;
        case 2:
            enemy.pos = Vector2{ left - pad, Random::real(top, bottom) };
            break;
        default:
            enemy.pos = Vector2{ right + pad, Random::real(top, bottom) };
            break;
    }

    const Vector2 to_center{ Vector2Subtract(view_center, enemy.pos) };
    enemy.rotation = std::atan2f(to_center.x, -to_center.y) * RAD2DEG;
    return enemy;
}

EnemySpaceship EnemySpaceship::spawn_at(Vector2 origin, f32 initial_rotation, EnemyArchetype type) {
    EnemySpaceship enemy{ type };
    enemy.pos = origin;
    enemy.rotation = initial_rotation;
    return enemy;
}

void EnemySpaceship::take_damage(i32 amount) noexcept {
    if (!is_alive()) return;
    health = (health > amount) ? (health - amount) : 0;
    hit_flash_timer = 0.15f;
}

void EnemySpaceship::apply_impulse(Vector2 impulse) noexcept {
    vel = Vector2Add(vel, impulse);
}

void EnemySpaceship::update(f32 dt, Vector2 player_pos, bool player_alive, Vector2 view_center) noexcept {
    if (hit_flash_timer > 0.0f) {
        hit_flash_timer = std::fmaxf(0.0f, hit_flash_timer - dt);
    }

    if (!player_alive) {
        telegraph_timer = 0.0f;
    }

    const Vector2 to_player{ Vector2Subtract(player_pos, pos) };
    const f32 dist{ Vector2Length(to_player) };
    const Vector2 dir{ dist > 0.001f ? Vector2Scale(to_player, 1.0f / dist) : Vector2{ 0.0f, -1.0f } };

    const f32 player_angle_deg{ std::atan2f(dir.x, -dir.y) * RAD2DEG };

    if (archetype == EnemyArchetype::Battlestar) {
        mode_switch_timer -= dt;
        if (mode_switch_timer <= 0.0f) {
            battlestar_mode = (battlestar_mode == BattlestarAttackMode::PointyNormal)
                                    ? BattlestarAttackMode::FlatWave
                                    : BattlestarAttackMode::PointyNormal;
            mode_switch_timer = 2.4f;
        }

        // At rotation = 0:
        //   - Flat ends face UP (rotation + 0 deg) and DOWN (rotation + 180 deg)
        //   - Pointy ends face RIGHT (rotation + 90 deg) and LEFT (rotation - 90 deg)
        f32 angle_diff{ 0.0f };
        if (battlestar_mode == BattlestarAttackMode::PointyNormal) {
            const f32 diff_a{ std::remainderf((player_angle_deg - 90.0f) - rotation, 360.0f) };
            const f32 diff_b{ std::remainderf((player_angle_deg + 90.0f) - rotation, 360.0f) };
            angle_diff = (std::fabsf(diff_a) <= std::fabsf(diff_b)) ? diff_a : diff_b;
        } else {
            const f32 diff_a{ std::remainderf(player_angle_deg - rotation, 360.0f) };
            const f32 diff_b{ std::remainderf((player_angle_deg + 180.0f) - rotation, 360.0f) };
            angle_diff = (std::fabsf(diff_a) <= std::fabsf(diff_b)) ? diff_a : diff_b;
        }

        rotation = std::remainderf(rotation + angle_diff * std::fminf(1.0f, turn_speed * dt), 360.0f);

        const Vector2 perp{ -dir.y * strafe_sign, dir.x * strafe_sign };
        f32 radial_weight{ 0.0f };
        if (dist > preferred_range + 45.0f) {
            radial_weight = 1.0f;
        } else if (dist < preferred_range - 45.0f) {
            radial_weight = -0.65f;
        }
        const Vector2 desired_dir{
            Vector2Normalize(Vector2Add(Vector2Scale(dir, radial_weight), Vector2Scale(perp, 0.55f)))
        };
        vel = Vector2Add(vel, Vector2Scale(desired_dir, accel * dt));

        if (dist < 540.0f && std::fabsf(angle_diff) < 22.0f) {
            if (battlestar_mode == BattlestarAttackMode::PointyNormal) {
                blaster.fire_bidirectional(pos, vel, rotation + 90.0f);
            } else {
                wave_blaster.fire_bidirectional(pos, vel, rotation);
            }
        }
    } else {
        const f32 angle_diff{ std::remainderf(player_angle_deg - rotation, 360.0f) };

        // Sniper locks rotation during final 0.25s of telegraph before firing
        const bool aim_locked{
            archetype == EnemyArchetype::Sniper && telegraph_timer >= (TELEGRAPH_DURATION - 0.25f)
        };
        if (!aim_locked) {
            rotation = std::remainderf(rotation + angle_diff * std::fminf(1.0f, turn_speed * dt), 360.0f);
        }

        if (archetype == EnemyArchetype::Kamikaze) {
            vel = Vector2Add(vel, Vector2Scale(dir, accel * dt));
        } else {
            const Vector2 perp{ -dir.y * strafe_sign, dir.x * strafe_sign };
            f32 radial_weight{ 0.0f };
            if (dist > preferred_range + 45.0f) {
                radial_weight = 1.0f;
            } else if (dist < preferred_range - 45.0f) {
                radial_weight = -0.65f;
            }

            const Vector2 desired_dir{
                Vector2Normalize(Vector2Add(Vector2Scale(dir, radial_weight), Vector2Scale(perp, 0.75f)))
            };
            vel = Vector2Add(vel, Vector2Scale(desired_dir, accel * dt));
        }

        if (archetype == EnemyArchetype::Fighter) {
            if (dist < 460.0f && std::fabsf(angle_diff) < 25.0f) {
                blaster.fire(pos, vel, rotation);
            }
        } else if (archetype == EnemyArchetype::Sniper) {
            if (telegraph_timer < 0.0f) {
                telegraph_timer = std::fminf(0.0f, telegraph_timer + dt);
            } else if (dist < 540.0f && (telegraph_timer > 0.0f || std::fabsf(angle_diff) < 28.0f)) {
                telegraph_timer += dt;
                if (telegraph_timer >= TELEGRAPH_DURATION) {
                    blaster.fire(pos, vel, rotation);
                    telegraph_timer = -0.95f;
                }
            } else {
                telegraph_timer = std::fmaxf(0.0f, telegraph_timer - dt * 2.0f);
            }
        }
    }

    const f32 speed{ Vector2Length(vel) };
    if (speed > max_speed) {
        vel = Vector2Scale(vel, max_speed / speed);
    }

    pos = Vector2Add(pos, Vector2Scale(vel, dt));
    blaster.update(dt, view_center);
    wave_blaster.update(dt, view_center);
}

void EnemySpaceship::draw() const noexcept {
    blaster.draw();
    wave_blaster.draw();
    if (!is_alive()) return;

    // Draw Sniper telegraph laser sight line
    if (is_telegraphing()) {
        const f32 progress{ std::clamp(telegraph_timer / TELEGRAPH_DURATION, 0.0f, 1.0f) };
        const f32 rad{ rotation * DEG2RAD };
        const Vector2 forward{ std::sinf(rad), -std::cosf(rad) };
        const Vector2 beam_start{ Vector2Add(pos, Vector2Scale(forward, get_radius())) };
        const Vector2 beam_end{ Vector2Add(pos, Vector2Scale(forward, 560.0f)) };
        const bool locked{ telegraph_timer >= (TELEGRAPH_DURATION - 0.25f) };
        const Color beam_color{
            locked ? hex_to_color(0xef4444ff) : hex_to_color(0x38bdf8ff)
        };
        const f32 beam_width{ locked ? 2.5f : (3.5f - 2.2f * progress) };
        DrawLineEx(beam_start, beam_end, beam_width, Fade(beam_color, 0.35f + 0.55f * progress));
    }

    if (sprite.is_valid()) {
        const Color tint{ hit_flash_timer > 0.0f ? hex_to_color(0xff5555ff) : base_tint };
        sprite.draw(pos, rotation, scale, tint);
    }

    if (health < max_health || archetype == EnemyArchetype::Battlestar) {
        const f32 bar_w{ archetype == EnemyArchetype::Battlestar ? 56.0f : 36.0f };
        const f32 bar_h{ archetype == EnemyArchetype::Battlestar ? 6.0f : 4.0f };
        const f32 bx{ pos.x - bar_w * 0.5f };
        const f32 by{ pos.y - get_radius() - 14.0f };
        const f32 ratio{ static_cast<f32>(health) / static_cast<f32>(max_health) };
        const Color fill_col{
            archetype == EnemyArchetype::Battlestar ? hex_to_color(0xa855f7ff) : hex_to_color(0xef4444ff)
        };

        DrawRectangleRec(Rectangle{ bx - 1.0f, by - 1.0f, bar_w + 2.0f, bar_h + 2.0f }, hex_to_color(0x0f172aff));
        DrawRectangleRec(Rectangle{ bx, by, bar_w, bar_h }, hex_to_color(0x1e293bff));
        DrawRectangleRec(Rectangle{ bx, by, bar_w * ratio, bar_h }, fill_col);
    }
}

bool EnemySpaceship::is_offscreen(Vector2 view_center, i32 screen_w, i32 screen_h, f32 margin) const noexcept {
    const f32 half_w{ static_cast<f32>(screen_w) * 0.5f + get_radius() + margin };
    const f32 half_h{ static_cast<f32>(screen_h) * 0.5f + get_radius() + margin };
    return (std::fabsf(pos.x - view_center.x) > half_w) ||
           (std::fabsf(pos.y - view_center.y) > half_h);
}

bool EnemySpaceship::collides_with(Vector2 point, f32 point_radius) const noexcept {
    return CheckCollisionCircles(pos, get_radius(), point, point_radius);
}

void EnemyField::update(f32 dt, Vector2 player_pos, bool player_alive, Vector2 view_center) noexcept {
    const i32 w{ VIRTUAL_WIDTH };
    const i32 h{ VIRTUAL_HEIGHT };

    if (auto_spawn && player_alive && static_cast<u64>(enemies.size()) < max_enemies) {
        spawn_timer -= dt;
        if (spawn_timer <= 0.0f) {
            bool has_battlestar{ false };
            for (const EnemySpaceship& e : enemies) {
                if (e.is_alive() && e.get_archetype() == EnemyArchetype::Battlestar) {
                    has_battlestar = true;
                    break;
                }
            }

            const f32 roll{ Random::real() };
            const EnemyArchetype chosen{
                (!has_battlestar && roll < 0.18f) ? EnemyArchetype::Battlestar :
                roll < 0.58f                      ? EnemyArchetype::Fighter :
                roll < 0.82f                      ? EnemyArchetype::Kamikaze :
                                                    EnemyArchetype::Sniper
            };
            enemies.emplace_back(EnemySpaceship::spawn_from_edge(view_center, w, h, chosen));
            spawn_timer = Random::real(spawn_interval * 0.75f, spawn_interval * 1.25f);
        }
    }

    for (EnemySpaceship& enemy : enemies) {
        enemy.update(dt, player_pos, player_alive, view_center);
    }

    std::erase_if(enemies, [&](const EnemySpaceship& enemy) noexcept {
        return !enemy.is_alive() || enemy.is_offscreen(view_center, w, h);
    });
}

void EnemyField::draw() const noexcept {
    for (const EnemySpaceship& enemy : enemies) {
        enemy.draw();
    }
}

void EnemyField::reset() noexcept {
    enemies.clear();
    max_enemies = DEFAULT_MAX_ENEMIES;
    spawn_interval = 5.0f;
    spawn_timer = 3.0f;
}

EnemySpaceship& EnemyField::spawn_at(Vector2 origin, f32 initial_rotation, EnemyArchetype type) {
    enemies.emplace_back(EnemySpaceship::spawn_at(origin, initial_rotation, type));
    return enemies.back();
}

} // namespace asteroides
