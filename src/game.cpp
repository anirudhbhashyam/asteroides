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

void Game::update(f32 dt) noexcept {
    const ScreenAction action{ screens.update(dt, spaceship.is_alive()) };
    if (action == ScreenAction::StartGame ||
        action == ScreenAction::RestartGame ||
        action == ScreenAction::ReturnToTitle) {
        reset();
        return;
    }

    const LayerPolicy policy{ screens.get_policy() };
    const Vector2 view_center{ world.get_camera_target() };

    if (has_layer(policy.update_mask, Layer::Combat)) {
        spaceship.update(dt);
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
        for (Asteroid& asteroid : asteroid_list) {
            if (asteroid.collides_with(player_pos, player_radius)) {
                const bool was_invulnerable{ spaceship.is_invulnerable() };
                spaceship.take_damage(20, true);
                if (!was_invulnerable) {
                    particles.emit(player_pos, Spaceship::HIT_SPARK_EFFECT, spaceship.get_vel());
                }
                if (!spaceship.is_alive()) {
                    particles.emit(player_pos, Spaceship::EXPLOSION_EFFECT, spaceship.get_vel());
                }
                asteroids.spawn_explosion(asteroid.get_pos(), asteroid.get_scale());
                asteroid = Asteroid::spawn_from_edge(view_center, w, h);
            }
        }
    }
}

void Game::draw() const noexcept {
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
    world.end_camera();

    if (has_layer(policy.draw_mask, Layer::Hud)) {
        screens.draw_hud(spaceship);
    }

    if (has_layer(policy.draw_mask, Layer::Overlay)) {
        screens.draw_overlay();
    }
}

} // namespace asteroides
