#include "game.hh"

namespace asteroides {

Game::Game(u64 star_count, u64 asteroid_count)
    : world{ spaceship.get_pos(), star_count }
    , asteroids{ spaceship.get_pos(), asteroid_count } {}

void Game::reset() noexcept {
    spaceship.reset();
    world.reset_camera(spaceship.get_pos());
    particles.clear();
    powerups.clear();
    asteroids.reset(spaceship.get_pos());
    enemies.reset();
}

void Game::update(const Engine& engine) noexcept {
    const f32 dt{ engine.get_time().dt };
    const InputState& input{ engine.get_input() };

    const ScreenAction action{ screens.update(engine.get_time().raw_dt, input, spaceship.is_alive()) };
    if (action == ScreenAction::StartGame ||
        action == ScreenAction::RestartGame ||
        action == ScreenAction::ReturnToTitle) {
        reset();
        return;
    }

    const LayerPolicy policy{ screens.get_policy() };
    const Vector2 view_center{ world.get_camera_target() };

    if (has_layer(policy.update_mask, Layer::Combat)) {
        spaceship.update(dt, input);
    }

    if (has_layer(policy.update_mask, Layer::Background)) {
        const Vector2 target_vel{
            has_layer(policy.update_mask, Layer::Combat) ? spaceship.get_vel() : Vector2Zero()
        };
        world.update(dt, spaceship.get_pos(), target_vel);
    }

    if (has_layer(policy.update_mask, Layer::Asteroids)) {
        asteroids.update(dt, view_center, spaceship.get_blaster(), powerups);
        particles.update(dt);
    }

    if (has_layer(policy.update_mask, Layer::Combat)) {
        enemies.update(dt, spaceship.get_pos(), spaceship.is_alive(), view_center);
        powerups.update(dt, spaceship);
        _handle_combat(view_center);
    }
}

void Game::_handle_combat(Vector2 view_center) noexcept {
    const i32 w{ GetScreenWidth() };
    const i32 h{ GetScreenHeight() };
    constexpr f32 bolt_radius{ 4.0f };

    auto& player_bolts{ spaceship.get_blaster().get_bolts() };
    auto& enemy_list{ enemies.get_enemies() };
    auto& asteroid_list{ asteroids.get_asteroids() };

    const Vector2 player_pos{ spaceship.get_pos() };
    const f32 player_radius{ spaceship.get_radius() };

    // 1. Player bolts vs. enemy ships & Enemy bolts vs. player ship
    for (EnemySpaceship& enemy : enemy_list) {
        if (!enemy.is_alive()) continue;

        for (auto it{ player_bolts.begin() }; it != player_bolts.end(); ++it) {
            if (enemy.collides_with(it->pos, bolt_radius)) {
                const Vector2 hit_pos{ it->pos };
                player_bolts.erase(it);
                enemy.take_damage(1);

                if (!enemy.is_alive()) {
                    particles.emit(enemy.get_pos(), EnemySpaceship::EXPLOSION_EFFECT, enemy.get_vel());
                    powerups.try_spawn(enemy.get_pos());
                } else {
                    particles.emit(hit_pos, EnemySpaceship::HIT_SPARK_EFFECT, enemy.get_vel());
                }
                break;
            }
        }

        if (spaceship.is_alive()) {
            auto& enemy_bolts{ enemy.get_blaster().get_bolts() };
            for (auto it{ enemy_bolts.begin() }; it != enemy_bolts.end(); ++it) {
                if (CheckCollisionCircles(player_pos, player_radius, it->pos, bolt_radius)) {
                    const Vector2 hit_pos{ it->pos };
                    enemy_bolts.erase(it);
                    spaceship.take_damage(15, false);
                    particles.emit(hit_pos, Spaceship::HIT_SPARK_EFFECT, spaceship.get_vel());
                    if (!spaceship.is_alive()) {
                        particles.emit(player_pos, Spaceship::EXPLOSION_EFFECT, spaceship.get_vel());
                    }
                    break;
                }
            }
        }

        // Direct ship-to-ship collision between enemy and player
        if (spaceship.is_alive() && enemy.is_alive() &&
            CheckCollisionCircles(player_pos, player_radius, enemy.get_pos(), enemy.get_radius())) {
            const bool was_invulnerable{ spaceship.is_invulnerable() };
            spaceship.take_damage(25, true);
            enemy.take_damage(1);

            const Vector2 push_dir{ Vector2Normalize(Vector2Subtract(player_pos, enemy.get_pos())) };
            spaceship.apply_impulse(Vector2Scale(push_dir, 180.0f));
            enemy.apply_impulse(Vector2Scale(push_dir, -180.0f));

            if (!was_invulnerable) {
                particles.emit(player_pos, Spaceship::HIT_SPARK_EFFECT, spaceship.get_vel());
            }
            if (!spaceship.is_alive()) {
                particles.emit(player_pos, Spaceship::EXPLOSION_EFFECT, spaceship.get_vel());
            }
            if (!enemy.is_alive()) {
                particles.emit(enemy.get_pos(), EnemySpaceship::EXPLOSION_EFFECT, enemy.get_vel());
            }
        }
    }

    // 2. Asteroids vs. player ship
    if (spaceship.is_alive()) {
        for (auto ast_it{ asteroid_list.begin() }; ast_it != asteroid_list.end();) {
            if (ast_it->collides_with(player_pos, player_radius)) {
                const bool was_invulnerable{ spaceship.is_invulnerable() };
                spaceship.take_damage(20, true);
                if (!was_invulnerable) {
                    particles.emit(player_pos, Spaceship::HIT_SPARK_EFFECT, spaceship.get_vel());
                }
                if (!spaceship.is_alive()) {
                    particles.emit(player_pos, Spaceship::EXPLOSION_EFFECT, spaceship.get_vel());
                }
                asteroids.spawn_explosion(ast_it->get_pos(), ast_it->get_scale());
                if (asteroid_list.size() <= 1) {
                    ast_it = asteroid_list.erase(ast_it);
                } else {
                    *ast_it = Asteroid::spawn_from_edge(view_center, w, h);
                    ++ast_it;
                }
            } else {
                ++ast_it;
            }
        }
    }
}

void Game::_draw_debug_world(const Engine& engine) const noexcept {
    if (!engine.is_debug_enabled()) return;

    constexpr f32 bolt_radius{ 4.0f };

    for (const Asteroid& ast : asteroids.get_asteroids()) {
        engine.draw_debug_circle(ast.get_pos(), ast.get_radius(), hex_to_color(0xfbbf24cc));
        engine.draw_debug_kinematics(ast.get_pos(), ast.get_vel(), ast.get_rotation(), ast.get_radius());
    }

    for (const Asteroid& frag : asteroids.get_fragments()) {
        engine.draw_debug_circle(frag.get_pos(), frag.get_radius(), hex_to_color(0xf59e0b88));
    }

    for (const Powerup& p : powerups.get_powerups()) {
        engine.draw_debug_circle(p.pos, p.radius, hex_to_color(0x22c55edd));
    }

    for (const EnemySpaceship& enemy : enemies.get_enemies()) {
        if (!enemy.is_alive()) continue;
        engine.draw_debug_circle(enemy.get_pos(), enemy.get_radius(), hex_to_color(0xef4444dd));
        engine.draw_debug_circle(enemy.get_pos(), enemy.get_preferred_range(), hex_to_color(0xa855f733));
        engine.draw_debug_kinematics(enemy.get_pos(), enemy.get_vel(), enemy.get_rotation(), enemy.get_radius());
        if (spaceship.is_alive()) {
            engine.draw_debug_line(enemy.get_pos(), spaceship.get_pos(), hex_to_color(0xa855f766));
        }
        for (const Blaster::Bolt& bolt : enemy.get_blaster().get_bolts()) {
            engine.draw_debug_circle(bolt.pos, bolt_radius, hex_to_color(0xf97316dd));
        }
    }

    if (spaceship.is_alive()) {
        const Color ship_hitbox_color{
            spaceship.is_invulnerable() ? hex_to_color(0x5eead4ff) : hex_to_color(0x38bdf8dd)
        };
        engine.draw_debug_circle(spaceship.get_pos(), spaceship.get_radius(), ship_hitbox_color);
        engine.draw_debug_kinematics(
            spaceship.get_pos(),
            spaceship.get_vel(),
            spaceship.get_rotation(),
            spaceship.get_radius()
        );
    }

    for (const Blaster::Bolt& bolt : spaceship.get_blaster().get_bolts()) {
        engine.draw_debug_circle(bolt.pos, bolt_radius, hex_to_color(0x38bdf8dd));
    }
}

TelemetryData Game::_build_telemetry() const noexcept {
    const LayerPolicy policy{ screens.get_policy() };
    u64 enemy_bolts{ 0 };
    for (const EnemySpaceship& enemy : enemies.get_enemies()) {
        enemy_bolts += static_cast<u64>(enemy.get_blaster().get_bolts().size());
    }

    return TelemetryData{
        .screen_name = screens.get_state_name(),
        .update_mask = static_cast<u8>(policy.update_mask),
        .draw_mask = static_cast<u8>(policy.draw_mask),
        .player_pos = spaceship.get_pos(),
        .player_vel = spaceship.get_vel(),
        .player_rotation = spaceship.get_rotation(),
        .player_health = spaceship.get_health(),
        .player_max_health = spaceship.get_max_health(),
        .player_invuln_timer = spaceship.get_invulnerability_timer(),
        .double_blaster_timer = spaceship.get_powerup_remaining(PowerupType::DoubleBlaster),
        .shield_timer = spaceship.get_powerup_remaining(PowerupType::Shield),
        .asteroid_count = static_cast<u64>(asteroids.get_asteroids().size()),
        .fragment_count = static_cast<u64>(asteroids.get_fragments().size()),
        .enemy_count = static_cast<u64>(enemies.get_enemies().size()),
        .player_bolt_count = static_cast<u64>(spaceship.get_blaster().get_bolts().size()),
        .enemy_bolt_count = enemy_bolts,
        .powerup_count = static_cast<u64>(powerups.get_powerups().size()),
        .particle_count = particles.count()
    };
}

void Game::draw(const Engine& engine) const noexcept {
    const LayerPolicy policy{ screens.get_policy() };

    if (has_layer(policy.draw_mask, Layer::Background)) {
        world.draw_background();
    }

    world.begin_camera();
        if (has_layer(policy.draw_mask, Layer::Asteroids)) {
            asteroids.draw();
        }
        if (has_layer(policy.draw_mask, Layer::Combat)) {
            powerups.draw();
            enemies.draw();
            spaceship.draw();
        }
        if (has_layer(policy.draw_mask, Layer::Asteroids)) {
            particles.draw();
        }
        _draw_debug_world(engine);
    world.end_camera();

    if (has_layer(policy.draw_mask, Layer::Hud)) {
        screens.draw_hud(spaceship);
    }

    if (has_layer(policy.draw_mask, Layer::Overlay)) {
        screens.draw_overlay();
    }

    if (engine.is_debug_enabled()) {
        engine.draw_telemetry(_build_telemetry());
    }
}

} // namespace asteroides
