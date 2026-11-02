#include "level.hh"

namespace asteroides {

void DeathZoneState::reset() noexcept {
    active = false;
    cause = DeathZoneCause::None;
    center = Vector2Zero();
    half_extents = Vector2{
        static_cast<f32>(GetScreenWidth()) * 0.5f,
        static_cast<f32>(GetScreenHeight()) * 0.5f
    };
    timer = 0.0f;
    max_duration = 12.0f;
    cooldown_timer = 0.0f;
    wander_check_timer = 2.5f;
    pulse_timer = 0.0f;
}

void DeathZoneState::activate(
    Vector2 lock_center,
    f32 screen_w,
    f32 screen_h,
    f32 duration,
    DeathZoneCause why
) noexcept {
    active = true;
    cause = why;
    center = lock_center;
    half_extents = Vector2{ screen_w * 0.5f, screen_h * 0.5f };
    max_duration = std::fmaxf(1.0f, duration);
    timer = max_duration;
    pulse_timer = 0.0f;
}

void DeathZoneState::deactivate() noexcept {
    active = false;
    cause = DeathZoneCause::None;
    timer = 0.0f;
    cooldown_timer = 10.0f;
}

bool DeathZoneState::update(f32 dt, bool warden_alive) noexcept {
    pulse_timer = std::fmod(pulse_timer + 5.0f * dt, TWO_PI);
    cooldown_timer = std::max(0.0f, cooldown_timer - dt);

    if (!active) return false;

    if (cause == DeathZoneCause::BattlestarWarden && !warden_alive) {
        deactivate();
        return true;
    }

    timer -= dt;
    if (timer <= 0.0f) {
        deactivate();
        return true;
    }
    return false;
}

bool DeathZoneState::enforce_player_bounds(
    Spaceship& spaceship,
    ParticleSystem& particles,
    World& world,
    Engine& engine
) noexcept {
    if (!active || !spaceship.is_alive()) return false;

    const f32 r{ spaceship.get_radius() };
    const f32 min_x{ center.x - half_extents.x + BORDER_INSET + r };
    const f32 max_x{ center.x + half_extents.x - BORDER_INSET - r };
    const f32 min_y{ center.y - half_extents.y + BORDER_INSET + r };
    const f32 max_y{ center.y + half_extents.y - BORDER_INSET - r };

    Vector2 pos{ spaceship.get_pos() };
    Vector2 vel{ spaceship.get_vel() };
    bool touched{ false };
    Vector2 inward{ Vector2Zero() };

    if (pos.x < min_x) {
        pos.x = min_x;
        vel.x = std::fabsf(vel.x) * 0.65f + 220.0f;
        inward.x += 1.0f;
        touched = true;
    } else if (pos.x > max_x) {
        pos.x = max_x;
        vel.x = -std::fabsf(vel.x) * 0.65f - 220.0f;
        inward.x -= 1.0f;
        touched = true;
    }

    if (pos.y < min_y) {
        pos.y = min_y;
        vel.y = std::fabsf(vel.y) * 0.65f + 220.0f;
        inward.y += 1.0f;
        touched = true;
    } else if (pos.y > max_y) {
        pos.y = max_y;
        vel.y = -std::fabsf(vel.y) * 0.65f - 220.0f;
        inward.y -= 1.0f;
        touched = true;
    }

    if (!touched) return false;

    spaceship.set_pos(pos);
    spaceship.set_vel(vel);

    const bool dashing{ spaceship.is_dashing() };
    const bool was_invuln{ spaceship.is_invulnerable() };
    const bool shielded{ spaceship.is_shielded() };

    if (dashing) {
        particles.emit(pos, Spaceship::HIT_SPARK_EFFECT, vel);
        particles.emit_text(pos, "BORDER PHASE!", hex_to_color(0x38bdf8ff), 14);
        world.add_trauma(0.14f);
        return false;
    }

    spaceship.take_damage(BORDER_DAMAGE, true);
    particles.emit(pos, Spaceship::HIT_SPARK_EFFECT, vel);

    if (!was_invuln) {
        particles.emit_text(pos, "-15 DEATH ZONE!", hex_to_color(0xef4444ff), 15);
        world.add_trauma(0.38f);
        engine.trigger_hitstop(0.03f);
        if (!spaceship.is_alive()) {
            particles.emit(pos, Spaceship::EXPLOSION_EFFECT, vel);
            particles.emit_explosion(pos, ExplosionType::Player, 3.0f, 0.52f, vel);
            world.add_trauma(0.65f);
        }
        return true;
    } else if (shielded) {
        particles.emit_text(pos, "SHIELD SHOCK!", hex_to_color(0x5eead4ff), 13);
        world.add_trauma(0.18f);
    }

    (void)inward;
    return false;
}

void DeathZoneState::draw_world() const noexcept {
    if (!active) return;

    const f32 pulse{ 0.55f + 0.45f * (0.5f + 0.5f * std::sinf(pulse_timer)) };
    const f32 left{ center.x - half_extents.x + BORDER_INSET * 0.5f };
    const f32 top{ center.y - half_extents.y + BORDER_INSET * 0.5f };
    const f32 w{ (half_extents.x - BORDER_INSET * 0.5f) * 2.0f };
    const f32 h{ (half_extents.y - BORDER_INSET * 0.5f) * 2.0f };

    const Rectangle outer_rect{ left, top, w, h };
    const Rectangle inner_rect{ left + 6.0f, top + 6.0f, w - 12.0f, h - 12.0f };

    const Color primary_col{
        cause == DeathZoneCause::BattlestarWarden ? hex_to_color(0xa855f7ff) : hex_to_color(0xef4444ff)
    };
    const Color accent_col{
        cause == DeathZoneCause::BattlestarWarden ? hex_to_color(0xfacc15ff) : hex_to_color(0xf97316ff)
    };

    DrawRectangleLinesEx(outer_rect, 4.5f, Fade(primary_col, 0.75f * pulse + 0.2f));
    DrawRectangleLinesEx(inner_rect, 2.0f, Fade(accent_col, 0.65f * pulse));

    // Draw corner containment pylons
    constexpr std::array<Vector2, 4> corners_dir{ {
        { -1.0f, -1.0f },
        {  1.0f, -1.0f },
        {  1.0f,  1.0f },
        { -1.0f,  1.0f }
    } };
    for (const Vector2& dir : corners_dir) {
        const Vector2 c_pos{
            center.x + dir.x * (half_extents.x - BORDER_INSET),
            center.y + dir.y * (half_extents.y - BORDER_INSET)
        };
        DrawPoly(c_pos, 4, 10.0f, 45.0f, Fade(primary_col, 0.85f));
        DrawCircleV(c_pos, 4.0f, Fade(accent_col, pulse));
    }
}

ObjectiveEntity& LevelManager::spawn_objective(
    ObjectiveEntityType type,
    Vector2 pos,
    f32 charge_time
) noexcept {
    const f32 r{
        type == ObjectiveEntityType::Star   ? 24.0f :
        type == ObjectiveEntityType::Beacon ? 28.0f :
                                              32.0f
    };
    entities.emplace_back(
        type,
        pos,
        r,
        0.0f,
        Random::real(0.0f, TWO_PI),
        0.0f,
        std::max(0.5f, charge_time),
        145.0f,
        true,
        false
    );
    return entities.back();
}

void LevelManager::setup_run(
    Vector2 player_pos,
    AsteroidField& asteroids,
    EnemyField& enemies
) noexcept {
    entities.clear();
    death_zone.reset();
    stars_collected = 0;
    stars_spawned_from_asteroids = 0;
    beacons_charged = 0;
    boss_spawned = false;
    boss_defeated = false;
    portal_spawned = false;
    level_just_completed = false;
    origin_pos = player_pos;

    if (mode == GameMode::Freestyle) {
        return;
    }

    const LevelDefinition& def{ get_definition() };
    asteroids.set_target_count(def.asteroid_count);
    asteroids.reset(player_pos);
    enemies.reset();
    enemies.set_max_enemies(def.max_enemies);
    enemies.set_spawn_interval(def.spawn_interval);

    switch (def.level_number) {
        case 1: {
            // 3 Astral Stars placed around the sector at medium range (520..760 px)
            constexpr std::array<Vector2, 3> offsets{ {
                {  540.0f, -340.0f },
                { -600.0f,  280.0f },
                {  180.0f,  640.0f }
            } };
            for (const Vector2& off : offsets) {
                spawn_objective(ObjectiveEntityType::Star, Vector2Add(player_pos, off));
            }
            break;
        }
        case 2: {
            // 2 Subspace Beacons to charge
            spawn_objective(
                ObjectiveEntityType::Beacon,
                Vector2Add(player_pos, Vector2{ -520.0f, -320.0f }),
                3.8f
            );
            spawn_objective(
                ObjectiveEntityType::Beacon,
                Vector2Add(player_pos, Vector2{ 560.0f, 340.0f }),
                3.8f
            );
            break;
        }
        case 3: {
            // Seed extra GoldCore and Volatile asteroids near the player and spawn 1 initial Star
            asteroids.spawn_custom(
                Vector2Add(player_pos, Vector2{ 320.0f, -210.0f }),
                Vector2{ -18.0f, 14.0f },
                2.3f,
                AsteroidVariant::GoldCore
            );
            asteroids.spawn_custom(
                Vector2Add(player_pos, Vector2{ -340.0f, -180.0f }),
                Vector2{ 20.0f, 12.0f },
                2.3f,
                AsteroidVariant::Volatile
            );
            asteroids.spawn_custom(
                Vector2Add(player_pos, Vector2{ 260.0f, 280.0f }),
                Vector2{ -15.0f, -20.0f },
                2.3f,
                AsteroidVariant::GoldCore
            );
            break;
        }
        case 4: {
            // 3 Deep-Space Stars far from origin (triggers Ambush Death Zone when venturing out)
            constexpr std::array<Vector2, 3> deep_offsets{ {
                {  920.0f, -480.0f },
                { -960.0f,  420.0f },
                {  240.0f, -980.0f }
            } };
            for (const Vector2& off : deep_offsets) {
                spawn_objective(ObjectiveEntityType::Star, Vector2Add(player_pos, off));
            }
            break;
        }
        case 5: {
            // 1 Nexus Command Beacon near the player; charging it summons the Dreadnought Battlestar!
            spawn_objective(
                ObjectiveEntityType::Beacon,
                Vector2Add(player_pos, Vector2{ 0.0f, -360.0f }),
                3.5f
            );
            break;
        }
        default:
            break;
    }
}

void LevelManager::on_asteroid_shattered(Vector2 pos, AsteroidVariant variant, bool by_player) noexcept {
    if (mode != GameMode::Levels || !by_player) return;
    const LevelDefinition& def{ get_definition() };
    if (!def.stars_from_asteroids) return;

    const u32 total_existing{ stars_collected + stars_spawned_from_asteroids };
    if (total_existing >= def.target_stars) return;

    // GoldCore/Volatile asteroids always unearth a Star Core; standard asteroids have a 55% chance
    if (variant != AsteroidVariant::Standard || Random::real() < 0.55f) {
        spawn_objective(ObjectiveEntityType::Star, pos);
        ++stars_spawned_from_asteroids;
    }
}

void LevelManager::on_enemy_killed(
    Vector2 pos,
    EnemyArchetype archetype,
    ParticleSystem& particles
) noexcept {
    if (mode != GameMode::Levels) return;
    const LevelDefinition& def{ get_definition() };
    if (def.requires_battlestar_boss && archetype == EnemyArchetype::Battlestar && !boss_defeated) {
        boss_defeated = true;
        spawn_objective(ObjectiveEntityType::Star, pos);
        particles.emit_text(pos, "NOVA STAR DROPPED!", hex_to_color(0xfef08aff), 17);
    }
}

void LevelManager::update(
    f32 dt,
    Spaceship& spaceship,
    EnemyField& enemies,
    ParticleSystem& particles,
    World& world,
    Engine& engine,
    u64& score_out
) noexcept {
    const f32 screen_w{ static_cast<f32>(GetScreenWidth()) };
    const f32 screen_h{ static_cast<f32>(GetScreenHeight()) };
    const Vector2 player_pos{ spaceship.get_pos() };
    const f32 player_radius{ spaceship.get_radius() };

    // 1. Check if a Battlestar is alive and close enough to trigger a Warden Death Zone
    bool warden_alive{ false };
    Vector2 warden_pos{ Vector2Zero() };
    for (const EnemySpaceship& enemy : enemies.get_enemies()) {
        if (enemy.is_alive() && enemy.get_archetype() == EnemyArchetype::Battlestar) {
            warden_alive = true;
            warden_pos = enemy.get_pos();
            break;
        }
    }

    if (!death_zone.active && death_zone.cooldown_timer <= 0.0f && spaceship.is_alive()) {
        if (warden_alive && Vector2Distance(player_pos, warden_pos) <= 360.0f) {
            death_zone.activate(world.get_camera_target(), screen_w, screen_h, 14.0f, DeathZoneCause::BattlestarWarden);
            world.set_camera_lock(death_zone.center);
            world.add_trauma(0.32f);
            particles.emit_text(player_pos, "BATTLESTAR LOCKDOWN!", hex_to_color(0xa855f7ff), 17);
        } else {
            // Check deep-space wander trigger
            const f32 dist_from_origin{ Vector2Distance(player_pos, origin_pos) };
            if (dist_from_origin > DeathZoneState::DEEP_SPACE_THRESHOLD) {
                death_zone.wander_check_timer -= dt;
                if (death_zone.wander_check_timer <= 0.0f) {
                    death_zone.wander_check_timer = 2.8f;
                    if (Random::real() < 0.65f) {
                        death_zone.activate(
                            world.get_camera_target(),
                            screen_w,
                            screen_h,
                            10.0f,
                            DeathZoneCause::DeepSpaceAmbush
                        );
                        world.set_camera_lock(death_zone.center);
                        world.add_trauma(0.28f);
                        particles.emit_text(player_pos, "DEEP-SPACE AMBUSH LOCKDOWN!", hex_to_color(0xef4444ff), 16);

                        // Spawn an ambush enemy inside the locked perimeter
                        const Vector2 ambush_pos{
                            death_zone.center.x + (Random::uint(0, 1) == 0 ? -260.0f : 260.0f),
                            death_zone.center.y - 180.0f
                        };
                        enemies.spawn_at(ambush_pos, 180.0f, EnemyArchetype::Kamikaze);
                    }
                }
            } else {
                death_zone.wander_check_timer = 2.0f;
            }
        }
    }

    if (death_zone.update(dt, warden_alive)) {
        world.set_camera_lock(std::nullopt);
        particles.emit_text(player_pos, "LOCKDOWN LIFTED!", hex_to_color(0x4ade80ff), 16);
    } else if (death_zone.active) {
        world.set_camera_lock(death_zone.center);
    }

    // 2. Update Objective Entities in Levels Mode
    if (mode != GameMode::Levels) return;

    const LevelDefinition& def{ get_definition() };

    for (ObjectiveEntity& obj : entities) {
        if (!obj.active) continue;
        obj.pulse_timer = std::fmodf(obj.pulse_timer + 3.8f * dt, TWO_PI);

        if (obj.type == ObjectiveEntityType::Star) {
            obj.rotation = std::fmodf(obj.rotation + 42.0f * dt, 360.0f);
            if (spaceship.is_alive() &&
                CheckCollisionCircles(player_pos, player_radius, obj.pos, obj.radius)) {
                obj.active = false;
                obj.completed = true;
                ++stars_collected;
                score_out += 500;
                particles.emit(obj.pos, STAR_COLLECT_EFFECT);
                world.add_trauma(0.20f);
                char buf[32]{ };
                std::snprintf(
                    buf,
                    sizeof(buf),
                    "STAR %u/%u (+500)",
                    stars_collected,
                    def.target_stars
                );
                particles.emit_text(obj.pos, buf, hex_to_color(0xfef08aff), 16);
            }
        } else if (obj.type == ObjectiveEntityType::Beacon) {
            if (!obj.completed && spaceship.is_alive()) {
                const f32 dist{ Vector2Distance(player_pos, obj.pos) };
                if (dist <= obj.proximity_radius) {
                    obj.charge_progress = std::fminf(1.0f, obj.charge_progress + dt / obj.charge_duration);
                    if (obj.charge_progress >= 1.0f) {
                        obj.completed = true;
                        ++beacons_charged;
                        score_out += 600;
                        particles.emit(obj.pos, STAR_COLLECT_EFFECT);
                        world.add_trauma(0.24f);
                        particles.emit_text(obj.pos, "BEACON ONLINE! (+600)", hex_to_color(0x38bdf8ff), 16);

                        // In Level 5, charging the Nexus Beacon summons the Dreadnought Battlestar!
                        if (def.requires_battlestar_boss && !boss_spawned) {
                            boss_spawned = true;
                            const Vector2 boss_pos{
                                Vector2Add(obj.pos, Vector2{ 0.0f, -240.0f })
                            };
                            enemies.spawn_at(boss_pos, 180.0f, EnemyArchetype::Battlestar);
                            particles.emit_text(obj.pos, "DREADNOUGHT INBOUND!", hex_to_color(0xa855f7ff), 18);
                        }
                    }
                } else {
                    obj.charge_progress = std::fmaxf(0.0f, obj.charge_progress - 0.12f * dt);
                }
            }
        } else if (obj.type == ObjectiveEntityType::Portal) {
            obj.rotation = std::fmodf(obj.rotation - 65.0f * dt, 360.0f);
            if (spaceship.is_alive() &&
                CheckCollisionCircles(player_pos, player_radius, obj.pos, obj.radius)) {
                obj.active = false;
                obj.completed = true;
                score_out += 1000;
                particles.emit(obj.pos, STAR_COLLECT_EFFECT);
                world.add_trauma(0.30f);
                world.set_camera_lock(std::nullopt);
                death_zone.deactivate();

                completed_mask |= static_cast<u8>(1u << (current_level - 1));
                if (current_level < TOTAL_LEVELS) {
                    unlocked_levels = std::max(unlocked_levels, current_level + 1);
                }
                level_just_completed = true;
                return;
            }
        }
    }

    // 3. Check if primary objectives are met to either spawn the Extraction Portal or complete the level
    const bool stars_done{ stars_collected >= def.target_stars };
    const bool beacons_done{ beacons_charged >= def.target_beacons };
    const bool boss_done{ !def.requires_battlestar_boss || boss_defeated };

    if (stars_done && beacons_done && boss_done) {
        if (def.requires_portal_escape) {
            if (!portal_spawned) {
                portal_spawned = true;
                const Vector2 portal_pos{
                    Vector2Add(player_pos, Vector2{ 0.0f, -220.0f })
                };
                spawn_objective(ObjectiveEntityType::Portal, portal_pos);
                particles.emit_text(portal_pos, "WARP PORTAL OPEN!", hex_to_color(0xc084fcff), 18);
            }
        } else if (!level_just_completed) {
            score_out += 800;
            world.set_camera_lock(std::nullopt);
            death_zone.deactivate();
            completed_mask |= static_cast<u8>(1u << (current_level - 1));
            if (current_level < TOTAL_LEVELS) {
                unlocked_levels = std::max(unlocked_levels, current_level + 1);
            }
            level_just_completed = true;
        }
    }

    (void)engine;
}

f32 LevelManager::get_objective_progress_ratio() const noexcept {
    const LevelDefinition& def{ get_definition() };
    if (portal_spawned) {
        return 1.0f;
    }

    f32 total_parts{ static_cast<f32>(def.target_stars + def.target_beacons + (def.requires_battlestar_boss ? 1u : 0u)) };
    if (total_parts <= 0.0f) return 0.0f;

    f32 current_parts{ static_cast<f32>(stars_collected) };
    for (const ObjectiveEntity& obj : entities) {
        if (obj.type == ObjectiveEntityType::Beacon) {
            current_parts += obj.completed ? 1.0f : obj.charge_progress;
        }
    }
    if (def.requires_battlestar_boss && boss_defeated) {
        current_parts += 1.0f;
    }
    return std::clamp(current_parts / total_parts, 0.0f, 1.0f);
}

void LevelManager::format_objective_status(char* buf, u64 buf_size) const noexcept {
    const LevelDefinition& def{ get_definition() };
    if (portal_spawned) {
        std::snprintf(buf, buf_size, "OBJECTIVE: ENTER THE WARP EXTRACTION PORTAL!");
        return;
    }

    if (def.requires_battlestar_boss) {
        if (beacons_charged < def.target_beacons) {
            f32 pct{ 0.0f };
            for (const ObjectiveEntity& obj : entities) {
                if (obj.type == ObjectiveEntityType::Beacon) {
                    pct = obj.charge_progress * 100.0f;
                    break;
                }
            }
            std::snprintf(buf, buf_size, "CHARGE NEXUS BEACON: %.0f%%", static_cast<f64>(pct));
        } else if (!boss_defeated) {
            std::snprintf(buf, buf_size, "DESTROY THE BATTLESTAR DREADNOUGHT!");
        } else {
            std::snprintf(buf, buf_size, "CLAIM THE NOVA STAR CORE (%u/%u)", stars_collected, def.target_stars);
        }
        return;
    }

    if (def.target_beacons > 0) {
        std::snprintf(
            buf,
            buf_size,
            "BEACONS CHARGED: %u / %u",
            beacons_charged,
            def.target_beacons
        );
    } else if (def.stars_from_asteroids) {
        std::snprintf(
            buf,
            buf_size,
            "SHATTER ASTEROIDS & COLLECT STARS: %u / %u",
            stars_collected,
            def.target_stars
        );
    } else {
        std::snprintf(
            buf,
            buf_size,
            "ASTRAL STARS RECOVERED: %u / %u",
            stars_collected,
            def.target_stars
        );
    }
}

void LevelManager::draw_world() const noexcept {
    death_zone.draw_world();

    if (mode != GameMode::Levels) return;

    for (const ObjectiveEntity& obj : entities) {
        if (!obj.active) continue;
        const f32 pulse{ std::sinf(obj.pulse_timer) };

        if (obj.type == ObjectiveEntityType::Star) {
            const f32 glow_r{ obj.radius + 8.0f + 2.5f * pulse };
            DrawCircleGradient(
                obj.pos,
                glow_r,
                Fade(hex_to_color(0xfef08aff), 0.35f),
                Fade(hex_to_color(0xfacc15ff), 0.0f)
            );
            DrawRing(
                obj.pos,
                obj.radius + 2.0f + pulse,
                obj.radius + 3.8f + pulse,
                0.0f,
                360.0f,
                32,
                Fade(hex_to_color(0xfef08aff), 0.75f)
            );
            if (star_sprite.is_valid()) {
                star_sprite.draw(obj.pos, obj.rotation, 1.85f + 0.1f * pulse, WHITE);
            } else {
                DrawPoly(obj.pos, 4, obj.radius, obj.rotation, hex_to_color(0xfacc15ff));
            }
        } else if (obj.type == ObjectiveEntityType::Beacon) {
            const Color ring_col{
                obj.completed          ? hex_to_color(0x4ade80ff) :
                obj.charge_progress > 0.0f ? hex_to_color(0x38bdf8ff) :
                                             hex_to_color(0x64748bff)
            };

            // Proximity capture ring
            DrawRing(
                obj.pos,
                obj.proximity_radius - 2.0f,
                obj.proximity_radius,
                0.0f,
                360.0f,
                48,
                Fade(ring_col, 0.38f)
            );

            if (obj.charge_progress > 0.0f) {
                DrawRing(
                    obj.pos,
                    obj.proximity_radius - 5.0f,
                    obj.proximity_radius + 1.0f,
                    -90.0f,
                    -90.0f + 360.0f * obj.charge_progress,
                    48,
                    Fade(obj.completed ? hex_to_color(0x4ade80ff) : hex_to_color(0x38bdf8ff), 0.88f)
                );
            }

            if (beacon_sprite.is_valid()) {
                const Color tint{ obj.completed ? hex_to_color(0x86efacff) : WHITE };
                beacon_sprite.draw(obj.pos, 0.0f, 2.0f, tint);
            } else {
                DrawCircleV(obj.pos, obj.radius, ring_col);
            }
        } else if (obj.type == ObjectiveEntityType::Portal) {
            const f32 aura_r{ obj.radius + 14.0f + 3.0f * pulse };
            DrawCircleGradient(
                obj.pos,
                aura_r,
                Fade(hex_to_color(0x38bdf8ff), 0.42f),
                Fade(hex_to_color(0xa855f7ff), 0.0f)
            );
            DrawRing(
                obj.pos,
                obj.radius + 4.0f + 1.5f * pulse,
                obj.radius + 6.5f + 1.5f * pulse,
                0.0f,
                360.0f,
                40,
                Fade(hex_to_color(0xc084fcff), 0.85f)
            );
            if (portal_sprite.is_valid()) {
                portal_sprite.draw(obj.pos, obj.rotation, 2.35f, WHITE);
            } else {
                DrawCircleV(obj.pos, obj.radius, hex_to_color(0xa855f7ff));
            }
        }
    }
}

} // namespace asteroides
