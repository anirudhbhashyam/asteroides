#include "enemy.hh"

namespace asteroides {

EnemySpaceship::EnemySpaceship() {
    strafe_sign = (Random::uint(0, 1) == 0) ? 1.0f : -1.0f;
}

EnemySpaceship EnemySpaceship::spawn_from_edge(Vector2 view_center, i32 screen_w, i32 screen_h) {
    EnemySpaceship enemy{ };
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

EnemySpaceship EnemySpaceship::spawn_at(Vector2 origin, f32 initial_rotation) {
    EnemySpaceship enemy{ };
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

    if (player_alive) {
        const Vector2 to_player{ Vector2Subtract(player_pos, pos) };
        const f32 dist{ Vector2Length(to_player) };
        const Vector2 dir{ dist > 0.001f ? Vector2Scale(to_player, 1.0f / dist) : Vector2{ 0.0f, -1.0f } };

        const f32 target_rotation{ std::atan2f(dir.x, -dir.y) * RAD2DEG };
        const f32 angle_diff{ std::remainderf(target_rotation - rotation, 360.0f) };
        rotation = std::remainderf(rotation + angle_diff * std::fminf(1.0f, turn_speed * dt), 360.0f);

        // Approach player until preferred engagement distance, while strafing laterally
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

        const f32 speed{ Vector2Length(vel) };
        if (speed > max_speed) {
            vel = Vector2Scale(vel, max_speed / speed);
        }

        // Fire at the player when within engagement range and roughly aimed at them
        if (dist < 460.0f && std::fabsf(angle_diff) < 25.0f) {
            blaster.fire(pos, vel, rotation);
        }
    }

    pos = Vector2Add(pos, Vector2Scale(vel, dt));
    blaster.update(dt, view_center);
}

void EnemySpaceship::draw() const noexcept {
    blaster.draw();
    if (!is_alive()) return;

    if (sprite.is_valid()) {
        const Color tint{ hit_flash_timer > 0.0f ? hex_to_color(0xff5555ff) : WHITE };
        sprite.draw(pos, rotation, scale, tint);
    }

    if (health < max_health) {
        constexpr f32 bar_w{ 36.0f };
        constexpr f32 bar_h{ 4.0f };
        const f32 bx{ pos.x - bar_w * 0.5f };
        const f32 by{ pos.y - get_radius() - 12.0f };
        const f32 ratio{ static_cast<f32>(health) / static_cast<f32>(max_health) };

        DrawRectangleRec(Rectangle{ bx, by, bar_w, bar_h }, hex_to_color(0x1e293bff));
        DrawRectangleRec(Rectangle{ bx, by, bar_w * ratio, bar_h }, hex_to_color(0xef4444ff));
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
    const i32 w{ GetScreenWidth() };
    const i32 h{ GetScreenHeight() };

    if (auto_spawn && player_alive && static_cast<u64>(enemies.size()) < MAX_ENEMIES) {
        spawn_timer -= dt;
        if (spawn_timer <= 0.0f) {
            enemies.push_back(EnemySpaceship::spawn_from_edge(view_center, w, h));
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
    spawn_timer = 3.0f;
}

EnemySpaceship& EnemyField::spawn_at(Vector2 origin, f32 initial_rotation) {
    enemies.push_back(EnemySpaceship::spawn_at(origin, initial_rotation));
    return enemies.back();
}

} // namespace asteroides
