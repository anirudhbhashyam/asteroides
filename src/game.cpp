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
    if (!spaceship.is_alive() && IsKeyPressed(KEY_R)) {
        reset();
        return;
    }

    spaceship.update(dt);
    world.update(dt, spaceship.get_pos(), spaceship.get_vel());

    const Vector2 view_center{ world.get_camera_target() };
    asteroids.update(dt, view_center, spaceship.get_blaster(), powerups);
    enemies.update(dt, spaceship.get_pos(), spaceship.is_alive(), view_center);
    powerups.update(dt, spaceship);
    particles.update(dt);

    _handle_combat(view_center);
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

void Game::_draw_hud() const noexcept {
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

    if (!spaceship.is_alive()) {
        const i32 w{ GetScreenWidth() };
        const i32 h{ GetScreenHeight() };
        DrawRectangle(0, 0, w, h, Fade(BLACK, 0.55f));

        const char* title{ "GAME OVER" };
        const char* prompt{ "Press R to Restart" };
        constexpr i32 title_size{ 48 };
        constexpr i32 prompt_size{ 22 };

        const i32 title_w{ MeasureText(title, title_size) };
        const i32 prompt_w{ MeasureText(prompt, prompt_size) };

        DrawText(title, (w - title_w) / 2, h / 2 - 40, title_size, hex_to_color(0xfe166dff));
        DrawText(prompt, (w - prompt_w) / 2, h / 2 + 24, prompt_size, hex_to_color(0xf8fafcff));
    }
}

void Game::draw() const noexcept {
    world.draw_background();

    world.begin_camera();
        asteroids.draw();
        powerups.draw();
        enemies.draw();
        spaceship.draw();
        particles.draw();
    world.end_camera();

    _draw_hud();
}

} // namespace asteroides
