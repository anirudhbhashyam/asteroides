#include "game.hh"

namespace asteroides {

Game::Game(u64 star_count, u64 asteroid_count)
    : world{ spaceship.get_pos(), star_count }
    , asteroids{ spaceship.get_pos(), asteroid_count }
    , base_asteroid_count{ asteroid_count } {}

void Game::reset() noexcept {
    progression.reset();
    spaceship.reset();
    world.reset_camera(spaceship.get_pos());
    particles.clear();
    powerups.clear();
    asteroids.set_target_count(base_asteroid_count);
    asteroids.reset(spaceship.get_pos());
    enemies.reset();
    levels.setup_run(spaceship.get_pos(), asteroids, enemies);
}

void Game::update(Engine& engine) noexcept {
    const f32 dt{ engine.get_time().dt };
    const f32 raw_dt{ engine.get_time().raw_dt };
    const InputState& input{ engine.get_input() };

    const ScreenAction action{ screens.update(raw_dt, input, spaceship.is_alive(), levels) };
    if (action == ScreenAction::StartGame ||
        action == ScreenAction::RestartGame ||
        action == ScreenAction::NextLevel ||
        action == ScreenAction::ReturnToTitle) {
        reset();
        return;
    }

    const LayerPolicy policy{ screens.get_policy() };
    const Vector2 view_center{ world.get_camera_target() };

    if (has_layer(policy.update_mask, Layer::Combat)) {
        const u64 bolts_before{ static_cast<u64>(spaceship.get_blaster().get_bolts().size()) };
        spaceship.update(dt, input);
        const u64 bolts_after{ static_cast<u64>(spaceship.get_blaster().get_bolts().size()) };
        if (bolts_after > bolts_before) {
            world.add_trauma(0.045f);
        }
        if (spaceship.consume_just_dashed()) {
            ParticleEffect burst{ Spaceship::DASH_BURST_EFFECT };
            burst.direction_deg = spaceship.get_rotation() + 180.0f;
            particles.emit(spaceship.get_pos(), burst, Vector2Scale(spaceship.get_vel(), 0.15f));
            world.add_trauma(0.11f);
        }

        if (spaceship.is_alive()) {
            const bool wave_advanced{ progression.update(dt) };
            if (levels.get_mode() == GameMode::Freestyle && wave_advanced) {
                const u32 wave{ progression.current_wave };
                asteroids.set_target_count(base_asteroid_count + static_cast<u64>((wave - 1) * 2));
                enemies.set_max_enemies(EnemyField::DEFAULT_MAX_ENEMIES + static_cast<u64>(wave - 1));
                enemies.set_spawn_interval(5.0f - 0.35f * static_cast<f32>(wave - 1));
            }
        }

        _emit_thruster_trails();
    }

    if (has_layer(policy.update_mask, Layer::Background)) {
        const Vector2 target_vel{
            has_layer(policy.update_mask, Layer::Combat) ? spaceship.get_vel() : Vector2Zero()
        };
        world.update(raw_dt, spaceship.get_pos(), target_vel);
    }

    if (has_layer(policy.update_mask, Layer::Asteroids)) {
        asteroids.update(dt, view_center, spaceship.get_blaster(), powerups);
        particles.update(dt > 0.0f ? dt : raw_dt * 0.25f);
    }

    if (has_layer(policy.update_mask, Layer::Combat)) {
        enemies.update(dt, spaceship.get_pos(), spaceship.is_alive(), view_center);
        powerups.update(dt, spaceship);
        _process_events();
        _handle_combat(engine, view_center);

        u64 obj_score{ 0 };
        levels.update(dt, spaceship, enemies, particles, world, engine, obj_score);
        if (obj_score > 0) {
            progression.score += obj_score;
            if (progression.score > progression.high_score) {
                progression.high_score = progression.score;
            }
        }
        if (levels.get_death_zone().enforce_player_bounds(spaceship, particles, world, engine)) {
            progression.on_player_damaged();
        }
        if (levels.consume_level_just_completed()) {
            screens.set_state(ScreenState::LevelComplete);
        }
    }
}

void Game::_emit_thruster_trails() noexcept {
    if (spaceship.is_alive() && spaceship.is_thrusting()) {
        const f32 rad{ spaceship.get_rotation() * DEG2RAD };
        const Vector2 forward{ std::sinf(rad), -std::cosf(rad) };
        const Vector2 nozzle_pos{
            Vector2Subtract(spaceship.get_pos(), Vector2Scale(forward, spaceship.get_radius() * 0.85f))
        };
        ParticleEffect exhaust{ Spaceship::THRUSTER_EFFECT };
        exhaust.direction_deg = spaceship.get_rotation() + 180.0f;
        particles.emit(nozzle_pos, exhaust, Vector2Scale(spaceship.get_vel(), 0.2f));
    }

    for (const EnemySpaceship& enemy : enemies.get_enemies()) {
        if (!enemy.is_alive() || Vector2LengthSqr(enemy.get_vel()) < 100.0f) continue;
        const f32 rad{ enemy.get_rotation() * DEG2RAD };
        const Vector2 forward{ std::sinf(rad), -std::cosf(rad) };
        const Vector2 nozzle_pos{
            Vector2Subtract(enemy.get_pos(), Vector2Scale(forward, enemy.get_radius() * 0.85f))
        };
        ParticleEffect exhaust{ EnemySpaceship::THRUSTER_EFFECT };
        exhaust.direction_deg = enemy.get_rotation() + 180.0f;
        particles.emit(nozzle_pos, exhaust, Vector2Scale(enemy.get_vel(), 0.2f));
    }
}

void Game::_process_events() noexcept {
    for (const ShatterEvent& ev : asteroids.consume_shatter_events()) {
        particles.emit(ev.pos, Asteroid::SHATTER_DUST_EFFECT);

        if (ev.variant == AsteroidVariant::Volatile) {
            particles.emit(ev.pos, EnemySpaceship::EXPLOSION_EFFECT);
            world.add_trauma(0.35f);
            particles.emit_text(ev.pos, "VOLATILE BLAST!", hex_to_color(0xf97316ff), 15);

            // AoE shockwave damages nearby enemies and pushes nearby asteroids
            for (EnemySpaceship& enemy : enemies.get_enemies()) {
                if (!enemy.is_alive()) continue;
                const f32 dist{ Vector2Distance(ev.pos, enemy.get_pos()) };
                if (dist <= Asteroid::VOLATILE_AOE_RADIUS + enemy.get_radius()) {
                    enemy.take_damage(3);
                    const Vector2 push{ Vector2Normalize(Vector2Subtract(enemy.get_pos(), ev.pos)) };
                    enemy.apply_impulse(Vector2Scale(push, 260.0f));
                    if (!enemy.is_alive()) {
                        particles.emit(enemy.get_pos(), EnemySpaceship::EXPLOSION_EFFECT, enemy.get_vel());
                        const u64 aoe_pts{ progression.add_kill_score(300) };
                        char buf[24]{ };
                        std::snprintf(buf, sizeof(buf), "AOE +%u", static_cast<u32>(aoe_pts));
                        particles.emit_text(enemy.get_pos(), buf, hex_to_color(0xef4444ff), 16);
                    }
                }
            }

            for (Asteroid& ast : asteroids.get_asteroids()) {
                const f32 dist{ Vector2Distance(ev.pos, ast.get_pos()) };
                if (dist > 1.0f && dist <= Asteroid::VOLATILE_AOE_RADIUS + ast.get_radius()) {
                    const Vector2 push{ Vector2Normalize(Vector2Subtract(ast.get_pos(), ev.pos)) };
                    ast.apply_impulse(Vector2Scale(push, 200.0f));
                }
            }
        }

        if (ev.by_player) {
            levels.on_asteroid_shattered(ev.pos, ev.variant, true);
            world.add_trauma(0.16f);
            const u64 base_pts{ ev.variant == AsteroidVariant::GoldCore ? 150u : 50u };
            const u64 pts{ progression.add_kill_score(base_pts) };
            char buf[32]{ };
            if (ev.variant == AsteroidVariant::GoldCore) {
                std::snprintf(buf, sizeof(buf), "GOLD +%u", static_cast<u32>(pts));
                particles.emit_text(ev.pos, buf, hex_to_color(0xfacc15ff), 15);
            } else {
                std::snprintf(buf, sizeof(buf), "+%u", static_cast<u32>(pts));
                particles.emit_text(ev.pos, buf, hex_to_color(0xfef08aff), 14);
            }
        }
    }

    for (const CollectedPowerup& col : powerups.consume_collected()) {
        world.add_trauma(0.10f);
        switch (col.type) {
            case PowerupType::IncreaseHealth:
                particles.emit_text(col.pos, "+25% HULL", hex_to_color(0x4ade80ff), 15);
                break;
            case PowerupType::DoubleBlaster:
                particles.emit_text(col.pos, "DOUBLE BLASTER!", hex_to_color(0xfbbf24ff), 15);
                break;
            case PowerupType::Shield:
                particles.emit_text(col.pos, "SHIELD UP!", hex_to_color(0x38bdf8ff), 15);
                break;
            default:
                break;
        }
    }
}

void Game::_handle_combat(Engine& engine, Vector2 view_center) noexcept {
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
                    const bool is_battlestar{ enemy.get_archetype() == EnemyArchetype::Battlestar };
                    levels.on_enemy_killed(enemy.get_pos(), enemy.get_archetype(), particles);
                    particles.emit(enemy.get_pos(), EnemySpaceship::EXPLOSION_EFFECT, enemy.get_vel());
                    if (is_battlestar) {
                        particles.emit(enemy.get_pos(), Spaceship::EXPLOSION_EFFECT, enemy.get_vel());
                        powerups.spawn_guaranteed(enemy.get_pos());
                        world.add_trauma(0.55f);
                        engine.trigger_hitstop(0.045f);
                    } else {
                        powerups.try_spawn(enemy.get_pos());
                        world.add_trauma(0.32f);
                        engine.trigger_hitstop(0.028f);
                    }

                    const u64 pts{ progression.add_kill_score(is_battlestar ? 1000u : 250u) };
                    char buf[32]{ };
                    if (is_battlestar) {
                        std::snprintf(buf, sizeof(buf), "BATTLESTAR +%u", static_cast<u32>(pts));
                        particles.emit_text(enemy.get_pos(), buf, hex_to_color(0xfacc15ff), 17);
                    } else {
                        std::snprintf(buf, sizeof(buf), "+%u", static_cast<u32>(pts));
                        particles.emit_text(enemy.get_pos(), buf, hex_to_color(0xf97316ff), 16);
                    }
                } else {
                    particles.emit(hit_pos, EnemySpaceship::HIT_SPARK_EFFECT, enemy.get_vel());
                    world.add_trauma(0.10f);
                }
                break;
            }
        }

        if (spaceship.is_alive()) {
            auto check_enemy_projectiles = [&](Blaster& enemy_blaster, i32 hit_damage) noexcept {
                const f32 proj_radius{ enemy_blaster.get_bolt_radius() };
                auto& enemy_bolts{ enemy_blaster.get_bolts() };
                for (auto it{ enemy_bolts.begin() }; it != enemy_bolts.end(); ++it) {
                    if (CheckCollisionCircles(player_pos, player_radius, it->pos, proj_radius)) {
                        const Vector2 hit_pos{ it->pos };
                        const bool dashing{ spaceship.is_dashing() };
                        const bool shielded{ spaceship.is_shielded() };
                        const i32 hp_before{ spaceship.get_health() };
                        enemy_bolts.erase(it);
                        if (!dashing) {
                            spaceship.take_damage(hit_damage, false);
                        }
                        particles.emit(hit_pos, Spaceship::HIT_SPARK_EFFECT, spaceship.get_vel());

                        if (dashing) {
                            spaceship.refund_dash_cooldown(0.5f);
                            const u64 parry_pts{ progression.add_kill_score(100) };
                            char buf[32]{ };
                            std::snprintf(buf, sizeof(buf), "PARRY +%u", static_cast<u32>(parry_pts));
                            particles.emit_text(hit_pos, buf, hex_to_color(0x38bdf8ff), 15);
                        } else if (shielded) {
                            world.add_trauma(0.12f);
                            particles.emit_text(hit_pos, "BLOCKED", hex_to_color(0x5eead4ff), 13);
                        } else if (spaceship.get_health() < hp_before) {
                            progression.on_player_damaged();
                            world.add_trauma(0.38f);
                            engine.trigger_hitstop(0.032f);
                            char dmg_buf[16]{ };
                            std::snprintf(dmg_buf, sizeof(dmg_buf), "-%d", hit_damage);
                            particles.emit_text(player_pos, dmg_buf, hex_to_color(0xef4444ff), 15);
                        }

                        if (!spaceship.is_alive()) {
                            particles.emit(player_pos, Spaceship::EXPLOSION_EFFECT, spaceship.get_vel());
                            world.add_trauma(0.65f);
                        }
                        break;
                    }
                }
            };

            check_enemy_projectiles(enemy.get_blaster(), 15);
            check_enemy_projectiles(enemy.get_wave_blaster(), 25);
        }

        // Direct ship-to-ship collision between enemy and player
        if (spaceship.is_alive() && enemy.is_alive() &&
            CheckCollisionCircles(player_pos, player_radius, enemy.get_pos(), enemy.get_radius())) {
            const bool dashing{ spaceship.is_dashing() };
            const bool was_invulnerable{ spaceship.is_invulnerable() };
            const bool shielded{ spaceship.is_shielded() };
            spaceship.take_damage(25, true);
            enemy.take_damage(1);

            const Vector2 push_dir{ Vector2Normalize(Vector2Subtract(player_pos, enemy.get_pos())) };
            spaceship.apply_impulse(Vector2Scale(push_dir, 180.0f));
            enemy.apply_impulse(Vector2Scale(push_dir, -180.0f));

            if (dashing) {
                spaceship.refund_dash_cooldown(0.5f);
                particles.emit_text(player_pos, "PHASE PARRY!", hex_to_color(0x38bdf8ff), 15);
            } else if (!was_invulnerable) {
                progression.on_player_damaged();
                particles.emit(player_pos, Spaceship::HIT_SPARK_EFFECT, spaceship.get_vel());
                particles.emit_text(player_pos, "-25", hex_to_color(0xef4444ff), 15);
                world.add_trauma(0.45f);
                engine.trigger_hitstop(0.038f);
            } else if (shielded) {
                particles.emit_text(player_pos, "SHIELD RAM!", hex_to_color(0x5eead4ff), 14);
                world.add_trauma(0.22f);
            }

            if (!spaceship.is_alive()) {
                particles.emit(player_pos, Spaceship::EXPLOSION_EFFECT, spaceship.get_vel());
                world.add_trauma(0.65f);
            }
            if (!enemy.is_alive()) {
                particles.emit(enemy.get_pos(), EnemySpaceship::EXPLOSION_EFFECT, enemy.get_vel());
                const u64 pts{ progression.add_kill_score(250) };
                char buf[24]{ };
                std::snprintf(buf, sizeof(buf), "+%u", static_cast<u32>(pts));
                particles.emit_text(enemy.get_pos(), buf, hex_to_color(0xf97316ff), 16);
            }
        }

        // Enemy-vs-Asteroid collisions
        if (enemy.is_alive()) {
            for (auto ast_it{ asteroid_list.begin() }; ast_it != asteroid_list.end();) {
                if (ast_it->collides_with(enemy.get_pos(), enemy.get_radius())) {
                    enemy.take_damage(2);
                    asteroids.spawn_explosion(
                        ast_it->get_pos(),
                        ast_it->get_scale(),
                        false,
                        ast_it->get_variant()
                    );
                    if (!enemy.is_alive()) {
                        particles.emit(enemy.get_pos(), EnemySpaceship::EXPLOSION_EFFECT, enemy.get_vel());
                        const u64 crash_pts{ progression.add_kill_score(150) };
                        char buf[24]{ };
                        std::snprintf(buf, sizeof(buf), "CRASH +%u", static_cast<u32>(crash_pts));
                        particles.emit_text(enemy.get_pos(), buf, hex_to_color(0xfbbf24ff), 15);
                    }
                    if (asteroid_list.size() <= 1) {
                        ast_it = asteroid_list.erase(ast_it);
                    } else {
                        *ast_it = Asteroid::spawn_from_edge(view_center, w, h);
                        ++ast_it;
                    }
                    break;
                } else {
                    ++ast_it;
                }
            }
        }
    }

    // 2. Asteroids vs. player ship
    if (spaceship.is_alive()) {
        for (auto ast_it{ asteroid_list.begin() }; ast_it != asteroid_list.end();) {
            if (ast_it->collides_with(player_pos, player_radius)) {
                const bool was_invulnerable{ spaceship.is_invulnerable() };
                const bool shielded{ spaceship.is_shielded() };
                spaceship.take_damage(20, true);
                if (!was_invulnerable) {
                    progression.on_player_damaged();
                    particles.emit(player_pos, Spaceship::HIT_SPARK_EFFECT, spaceship.get_vel());
                    particles.emit_text(player_pos, "-20", hex_to_color(0xef4444ff), 15);
                    world.add_trauma(0.42f);
                    engine.trigger_hitstop(0.035f);
                } else if (shielded) {
                    particles.emit_text(ast_it->get_pos(), "DEFLECTED", hex_to_color(0x5eead4ff), 13);
                    world.add_trauma(0.18f);
                }
                if (!spaceship.is_alive()) {
                    particles.emit(player_pos, Spaceship::EXPLOSION_EFFECT, spaceship.get_vel());
                    world.add_trauma(0.65f);
                }
                asteroids.spawn_explosion(ast_it->get_pos(), ast_it->get_scale(), false);
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
            engine.draw_debug_circle(bolt.pos, enemy.get_blaster().get_bolt_radius(), hex_to_color(0xf97316dd));
        }
        for (const Blaster::Bolt& wave : enemy.get_wave_blaster().get_bolts()) {
            engine.draw_debug_circle(wave.pos, enemy.get_wave_blaster().get_bolt_radius(), hex_to_color(0x2dd4bfdd));
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
        enemy_bolts += static_cast<u64>(enemy.get_wave_blaster().get_bolts().size());
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
            levels.draw_world();
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
        screens.draw_hud(
            spaceship,
            progression,
            levels,
            enemies,
            powerups,
            asteroids,
            world.get_camera_target()
        );
    }

    if (has_layer(policy.draw_mask, Layer::Overlay)) {
        screens.draw_overlay(progression, levels);
    }

    if (engine.is_debug_enabled()) {
        engine.draw_telemetry(_build_telemetry());
    }
}

} // namespace asteroides
