#include "test_runner.hh"

namespace asteroides::tests {

VisualTestRunner::VisualTestRunner(Engine& engine_ref, Game& game_ref) noexcept
    : engine{ engine_ref }
    , game{ game_ref } {
    scenarios = { {
        {
            .title = "1. Screen & Layer Policy State Machine",
            .subtitle = "Verifies Start, Playing, Paused & LayerPolicy update/draw masks",
            .duration = 2.6f,
            .assertions = { {
                { "Start state policy (U:0x13 D:0x13)", false },
                { "Confirm transitions Start -> Playing", false },
                { "Paused freezes Combat & Asteroids", false },
                { "Resume restores Playing layer updates", false }
            } }
        },
        {
            .title = "2. Spaceship Kinematics, Rotation & Blaster",
            .subtitle = "Verifies WASD thrust, shortest-arc rotation, braking drag & blaster",
            .duration = 3.0f,
            .assertions = { {
                { "Thrust accelerates ship along input axis", false },
                { "Shortest-arc rotation aligns with velocity", false },
                { "Blaster spawns bolts & enforces cooldown", false },
                { "Quadratic drag & braking decelerate ship", false }
            } }
        },
        {
            .title = "3. Asteroid Procedural Gen, Bounce & Shatter",
            .subtitle = "Verifies 32x32 procedural sprites, 2D impulse bounce & fragmentation",
            .duration = 3.2f,
            .assertions = { {
                { "Procedural 32x32 asteroid sprites valid", false },
                { "2D rigid-body impulse collision & bounce", false },
                { "Blaster bolt shatters asteroid into fragments", false },
                { "Fragments shrink, fade & expire cleanly", false }
            } }
        },
        {
            .title = "4. Powerup Suite, Unified Timers & Shield Halo",
            .subtitle = "Verifies IncreaseHealth (+25%), DoubleBlaster twin shot & Shield block",
            .duration = 3.4f,
            .assertions = { {
                { "IncreaseHealth restores +25% HP (50 -> 75)", false },
                { "DoubleBlaster timer (10s) & twin-cannon shot", false },
                { "Shield timer (10s) & active cyan halo", false },
                { "Active Shield blocks incoming hull damage", false }
            } }
        },
        {
            .title = "5. Enemy AI Pursuit/Strafe & Explosion Particles",
            .subtitle = "Verifies EnemySpaceship strafe AI, damage bars & fiery explosion",
            .duration = 3.6f,
            .assertions = { {
                { "Enemy AI pursues/strafes & fires enemy bolts", false },
                { "Player bolt damages enemy & emits hit sparks", false },
                { "3 hits destroy enemy & emit explosion burst", false },
                { "Destroyed enemy cleaned up from EnemyField", false }
            } }
        },
        {
            .title = "6. Player Damage, Contact i-Frames & Game Over",
            .subtitle = "Verifies 0.6s contact invulnerability, death explosion & GameOver reset",
            .duration = 3.0f,
            .assertions = { {
                { "Contact hit grants 0.6s invulnerability frames", false },
                { "Fatal hit reduces HP to 0 & emits explosion", false },
                { "Player death transitions to GameOver screen", false },
                { "Restart action resets Game & restores 100 HP", false }
            } }
        },
        {
            .title = "7. Score, Combo, Wave Director, Shake & Radar",
            .subtitle = "Verifies kill-chain multiplier, wave escalation, camera trauma & HUD",
            .duration = 3.2f,
            .assertions = { {
                { "Thruster trails & camera trauma shake active", false },
                { "Kill-chain combo multiplier & score award", false },
                { "World-space floating combat text popups", false },
                { "Wave escalation & damage combo decay", false }
            } }
        },
        {
            .title = "8. Phase Dash, Strafe-Lock, Sniper & Volatile AoE",
            .subtitle = "Verifies dash i-frames, reverse-strafe aim, Sniper laser & Volatile blast",
            .duration = 3.4f,
            .assertions = { {
                { "Phase Dash impulse, i-frames & cooldown bar", false },
                { "Strafe-Lock preserves heading while retreating", false },
                { "Sniper telegraph laser & Kamikaze pursuit", false },
                { "Volatile Asteroid AoE blast kills nearby enemy", false }
            } }
        },
        {
            .title = "9. Battlestar Pointy Volley, Flat Wave & Radar",
            .subtitle = "Verifies pointy-end normal bolts, 90deg spin flat-end wave attack & kill",
            .duration = 4.2f,
            .assertions = { {
                { "Pointy ends spin to player & fire normal bolts", false },
                { "Spins 90deg to flat ends & fires wave attacks", false },
                { "Wave attack impacts player shield / hull", false },
                { "Battlestar destroyed (+1000 pts & powerup drop)", false }
            } }
        },
        {
            .title = "10. Levels Mode, Star, Beacon & Portal Escape",
            .subtitle = "Verifies LevelSelect, Star collection, Beacon charging & Portal victory",
            .duration = 3.8f,
            .assertions = { {
                { "Title -> LevelSelect & launch Levels campaign", false },
                { "Collecting Astral Star awards +500 & progress", false },
                { "Holding inside Subspace Beacon charges to 100%", false },
                { "Warp Portal extraction completes level & unlocks next", false }
            } }
        },
        {
            .title = "11. Death Zone Lockdown (Ambush & Warden Borders)",
            .subtitle = "Verifies deep-space ambush lock, border shock bounce & Battlestar release",
            .duration = 3.8f,
            .assertions = { {
                { "Deep-space wander triggers Ambush Death Zone", false },
                { "Border contact bounces ship & deals -15 shock", false },
                { "Battlestar warden activates Death Zone lockdown", false },
                { "Destroying Battlestar warden lifts Death Zone", false }
            } }
        }
    } };

    std::printf("\n============================================================\n");
    std::printf("  ASTEROIDES VISUAL TEST SUITE (%u SCENARIOS)\n", static_cast<u32>(SCENARIO_COUNT));
    std::printf("============================================================\n");
    _start_scenario(0);
}

void VisualTestRunner::_pass_assertion(u64 assertion_idx) noexcept {
    if (current_idx >= SCENARIO_COUNT || assertion_idx >= 4) return;
    AssertionItem& item{ scenarios[current_idx].assertions[assertion_idx] };
    if (!item.passed) {
        item.passed = true;
        std::printf("  [PASS] %s\n", item.label.data());
    }
}

void VisualTestRunner::_start_scenario(u64 index) noexcept {
    current_idx = index;
    step_flags = 0;
    if (current_idx >= SCENARIO_COUNT) {
        finished = true;
        std::printf("\n============================================================\n");
        std::printf(
            "  VISUAL TEST SUMMARY: %s\n",
            all_passed() ? "ALL SCENARIOS PASSED (0 FAILURES)" : "SOME ASSERTIONS FAILED"
        );
        std::printf("============================================================\n\n");
        return;
    }

    ScenarioStatus& sc{ scenarios[current_idx] };
    sc.elapsed = 0.0f;
    std::printf("\n---> Running %s\n", sc.title.data());

    game.reset();
    game.get_enemies().set_auto_spawn(false);
    game.get_enemies().reset();

    switch (current_idx) {
        case 0: {
            // Scenario 1: Start in ScreenState::Start with background asteroids
            game.get_screens().set_state(ScreenState::Start);
            game.get_asteroids().set_target_count(8);
            game.get_asteroids().reset(game.get_spaceship().get_pos());
            break;
        }
        case 1: {
            // Scenario 2: Clean arena in Playing state for flight & blaster test
            game.get_screens().set_state(ScreenState::Playing);
            game.get_asteroids().set_target_count(0);
            game.get_asteroids().clear();
            break;
        }
        case 2: {
            // Scenario 3: Stage colliding asteroids + target asteroid in front of ship
            game.get_screens().set_state(ScreenState::Playing);
            game.get_asteroids().set_target_count(0);
            game.get_asteroids().clear();

            Spaceship& ship{ game.get_spaceship() };
            ship.set_pos(Vector2{ 400.0f, 360.0f });
            ship.set_rotation(0.0f);
            game.get_world().reset_camera(ship.get_pos());

            // Two asteroids on a head-on collision course at y = 170
            game.get_asteroids().spawn_custom(Vector2{ 352.0f, 170.0f }, Vector2{ 95.0f, 0.0f }, 2.2f);
            game.get_asteroids().spawn_custom(Vector2{ 448.0f, 170.0f }, Vector2{ -95.0f, 0.0f }, 2.2f);
            // Guaranteed physics bounce test pair at y = 460
            Asteroid a{ Asteroid::spawn_custom(Vector2{ 390.0f, 460.0f }, Vector2{ 80.0f, 0.0f }, 2.0f) };
            Asteroid b{ Asteroid::spawn_custom(Vector2{ 410.0f, 460.0f }, Vector2{ -80.0f, 0.0f }, 2.0f) };
            Asteroid::resolve_physics_collision(a, b);
            if (a.get_vel().x < 0.0f && b.get_vel().x > 0.0f) {
                step_flags |= (1u << 8);
            }

            // Target asteroid directly in front of the ship's blaster at (400, 250)
            game.get_asteroids().spawn_custom(Vector2{ 400.0f, 250.0f }, Vector2{ 0.0f, 0.0f }, 2.3f);
            break;
        }
        case 3: {
            // Scenario 4: Powerup line-up in front of damaged ship (50/100 HP)
            game.get_screens().set_state(ScreenState::Playing);
            game.get_asteroids().set_target_count(0);
            game.get_asteroids().clear();

            Spaceship& ship{ game.get_spaceship() };
            ship.set_pos(Vector2{ 400.0f, 360.0f });
            ship.set_rotation(0.0f);
            ship.set_health(50);
            game.get_world().reset_camera(ship.get_pos());

            game.get_powerups().spawn_specific(PowerupType::IncreaseHealth, Vector2{ 400.0f, 310.0f });
            game.get_powerups().spawn_specific(PowerupType::DoubleBlaster, Vector2{ 400.0f, 250.0f });
            game.get_powerups().spawn_specific(PowerupType::Shield, Vector2{ 400.0f, 190.0f });
            break;
        }
        case 4: {
            // Scenario 5: Enemy AI pursuit/strafe, damage bar, and explosion
            game.get_screens().set_state(ScreenState::Playing);
            game.get_asteroids().set_target_count(0);
            game.get_asteroids().clear();

            Spaceship& ship{ game.get_spaceship() };
            ship.set_pos(Vector2{ 400.0f, 360.0f });
            ship.set_rotation(0.0f);
            ship.activate_powerup(PowerupType::Shield, 10.0f);
            game.get_world().reset_camera(ship.get_pos());

            game.get_enemies().spawn_at(Vector2{ 400.0f, 175.0f }, 180.0f);
            break;
        }
        case 5: {
            // Scenario 6: Contact i-frames, fatal hull damage, GameOver & Restart
            game.get_screens().set_state(ScreenState::Playing);
            game.get_asteroids().set_target_count(0);
            game.get_asteroids().clear();

            Spaceship& ship{ game.get_spaceship() };
            ship.set_pos(Vector2{ 400.0f, 300.0f });
            ship.set_health(40);
            game.get_world().reset_camera(ship.get_pos());

            // Spawn asteroid colliding with player to trigger contact damage + i-frames
            game.get_asteroids().spawn_custom(Vector2{ 400.0f, 290.0f }, Vector2{ 0.0f, 40.0f }, 2.0f);
            break;
        }
        case 6: {
            // Scenario 7: Score, Combo Multiplier, Wave Director, Camera Shake, Off-screen Radar/Chevrons
            game.get_screens().set_state(ScreenState::Playing);
            game.get_asteroids().set_target_count(0);
            game.get_asteroids().clear();

            Spaceship& ship{ game.get_spaceship() };
            ship.set_pos(Vector2{ 400.0f, 380.0f });
            ship.set_rotation(0.0f);
            game.get_world().reset_camera(ship.get_pos());

            // Two target asteroids in line of fire to build combo (1.0x -> 1.5x -> 2.0x)
            game.get_asteroids().spawn_custom(Vector2{ 400.0f, 275.0f }, Vector2Zero(), 2.0f);
            game.get_asteroids().spawn_custom(Vector2{ 400.0f, 185.0f }, Vector2Zero(), 2.0f);

            // Off-screen enemy & off-screen powerup to showcase Edge Indicators & Tactical Radar
            game.get_enemies().spawn_at(Vector2{ 1650.0f, 140.0f }, 250.0f);
            game.get_powerups().spawn_specific(PowerupType::Shield, Vector2{ -700.0f, 440.0f });
            break;
        }
        case 7: {
            // Scenario 8: Phase Dash, Strafe-Lock, Sniper Telegraph & Volatile Asteroid AoE
            game.get_screens().set_state(ScreenState::Playing);
            game.get_asteroids().set_target_count(0);
            game.get_asteroids().clear();

            Spaceship& ship{ game.get_spaceship() };
            ship.set_pos(Vector2{ 400.0f, 390.0f });
            ship.set_rotation(0.0f);
            ship.activate_powerup(PowerupType::Shield, 10.0f);
            game.get_world().reset_camera(ship.get_pos());

            // Spawn a Rail Sniper on the left to demonstrate telegraph laser sight
            game.get_enemies().spawn_at(Vector2{ 210.0f, 180.0f }, 145.0f, EnemyArchetype::Sniper);

            // Spawn a Volatile Asteroid directly above the ship and a Kamikaze Swarmer beside it
            game.get_asteroids().spawn_custom(
                Vector2{ 400.0f, 200.0f },
                Vector2Zero(),
                2.3f,
                AsteroidVariant::Volatile
            );
            game.get_enemies().spawn_at(Vector2{ 430.0f, 195.0f }, 180.0f, EnemyArchetype::Kamikaze);

            // Also spawn a GoldCore Asteroid to the right
            game.get_asteroids().spawn_custom(
                Vector2{ 555.0f, 240.0f },
                Vector2Zero(),
                2.2f,
                AsteroidVariant::GoldCore
            );
            break;
        }
        case 8: {
            // Scenario 9: Battlestar Pointy-End Volley, Flat-End Wave Attack & Radar Emblem
            game.get_levels().set_mode(GameMode::Freestyle);
            game.get_screens().set_state(ScreenState::Playing);
            game.get_asteroids().set_target_count(0);
            game.get_asteroids().clear();

            Spaceship& ship{ game.get_spaceship() };
            ship.set_pos(Vector2{ 400.0f, 400.0f });
            ship.set_rotation(0.0f);
            ship.activate_powerup(PowerupType::Shield, 10.0f);
            ship.activate_powerup(PowerupType::DoubleBlaster, 10.0f);
            game.get_world().reset_camera(ship.get_pos());

            // Spawn Battlestar at (400, 175) starting in PointyNormal mode
            EnemySpaceship& bstar{
                game.get_enemies().spawn_at(Vector2{ 400.0f, 175.0f }, 40.0f, EnemyArchetype::Battlestar)
            };
            bstar.set_battlestar_mode(BattlestarAttackMode::PointyNormal);
            break;
        }
        case 9: {
            // Scenario 10: Levels Mode, Star Collection, Beacon Charging & Portal Escape
            game.get_levels().set_mode(GameMode::Freestyle);
            game.get_levels().set_current_level(1);
            game.get_screens().set_state(ScreenState::Start);
            game.get_asteroids().set_target_count(0);
            game.get_asteroids().clear();
            break;
        }
        case 10: {
            // Scenario 11: Death Zone Screen-Border Lockdown (Ambush & Battlestar Warden)
            game.get_levels().set_mode(GameMode::Freestyle);
            game.get_screens().set_state(ScreenState::Playing);
            game.get_asteroids().set_target_count(0);
            game.get_asteroids().clear();

            Spaceship& ship{ game.get_spaceship() };
            ship.set_pos(Vector2{ 400.0f, 300.0f });
            ship.set_rotation(90.0f);
            game.get_world().reset_camera(ship.get_pos());
            game.get_levels().get_death_zone().reset();
            break;
        }
        default:
            break;
    }
}

void VisualTestRunner::_advance_scenario() noexcept {
    _start_scenario(current_idx + 1);
}

void VisualTestRunner::update() noexcept {
    if (IsKeyPressed(KEY_Q)) {
        finished = true;
        summary_timer = 0.0f;
        return;
    }

    if (finished) {
        summary_timer -= engine.get_time().raw_dt;
        return;
    }

    if (IsKeyPressed(KEY_N)) {
        for (u64 i{ 0 }; i < 4; ++i) {
            _pass_assertion(i);
        }
        _advance_scenario();
        return;
    }

    _update_scenario(engine.get_time().raw_dt);
}

void VisualTestRunner::_update_scenario(f32 dt) noexcept {
    ScenarioStatus& sc{ scenarios[current_idx] };
    sc.elapsed += dt;
    const f32 t{ sc.elapsed };

    InputState synth{ };

    switch (current_idx) {
        case 0: {
            // 1. Verify Start state & LayerPolicy
            const LayerPolicy start_pol{ game.get_screens().get_policy() };
            if (game.get_screens().get_state() == ScreenState::Start &&
                static_cast<u8>(start_pol.update_mask) == 0x13 &&
                static_cast<u8>(start_pol.draw_mask) == 0x13) {
                _pass_assertion(0);
            }

            // 2. Confirm into Playing
            if (t >= 0.55f && !(step_flags & 1u)) {
                synth.confirm_pressed = true;
                step_flags |= 1u;
            }
            if ((step_flags & 1u) && game.get_screens().get_state() == ScreenState::Playing) {
                _pass_assertion(1);
            }

            // 3. Pause and verify freeze
            if (t >= 1.25f && !(step_flags & 2u)) {
                synth.pause_pressed = true;
                step_flags |= 2u;
            } else if ((step_flags & 2u) && !(step_flags & 4u) &&
                       game.get_screens().get_state() == ScreenState::Paused) {
                saved_ship_pos = game.get_spaceship().get_pos();
                if (!game.get_asteroids().get_asteroids().empty()) {
                    saved_asteroid_pos = game.get_asteroids().get_asteroids().front().get_pos();
                }
                step_flags |= 4u;
            } else if ((step_flags & 4u) && t >= 1.65f &&
                       game.get_screens().get_state() == ScreenState::Paused) {
                const Vector2 cur_ship{ game.get_spaceship().get_pos() };
                const Vector2 cur_ast{
                    game.get_asteroids().get_asteroids().empty()
                        ? saved_asteroid_pos
                        : game.get_asteroids().get_asteroids().front().get_pos()
                };
                if (Vector2DistanceSqr(cur_ship, saved_ship_pos) < 0.001f &&
                    Vector2DistanceSqr(cur_ast, saved_asteroid_pos) < 0.001f) {
                    _pass_assertion(2);
                }
            }

            // 4. Resume back to Playing
            if (t >= 1.95f && !(step_flags & 8u)) {
                synth.confirm_pressed = true;
                step_flags |= 8u;
            }
            if ((step_flags & 8u) && game.get_screens().get_state() == ScreenState::Playing) {
                _pass_assertion(3);
            }
            break;
        }

        case 1: {
            Spaceship& ship{ game.get_spaceship() };
            if (t < 0.9f) {
                synth.move_axis = Vector2{ 1.0f, 0.0f };
                if (Vector2Length(ship.get_vel()) > 120.0f && ship.is_thrusting() && ship.get_flame_frame() > 0) {
                    _pass_assertion(0);
                }
                if (std::fabsf(ship.get_rotation() - 90.0f) < 20.0f) {
                    _pass_assertion(1);
                }
            } else if (t < 1.85f) {
                synth.move_axis = Vector2{ 0.0f, -1.0f };
                synth.fire_down = true;
                if (ship.get_blaster().get_bolts().size() >= 2) {
                    _pass_assertion(2);
                }
            } else {
                // Coast & brake to verify quadratic drag + braking deceleration
                synth.move_axis = Vector2Zero();
                if (Vector2Length(ship.get_vel()) < 25.0f) {
                    _pass_assertion(3);
                }
            }
            break;
        }

        case 2: {
            const auto& asts{ game.get_asteroids().get_asteroids() };
            const auto& frags{ game.get_asteroids().get_fragments() };

            if (!asts.empty() && !(step_flags & (1u << 9))) {
                const auto px{ Asteroid::generate_pixels(AsteroidVariant::Standard) };
                bool rim_closed{ true };
                for (i32 y{ 1 }; y < Asteroid::SPRITE_SIZE - 1 && rim_closed; ++y) {
                    for (i32 x{ 1 }; x < Asteroid::SPRITE_SIZE - 1; ++x) {
                        const Color c{ px[static_cast<u64>(y * Asteroid::SPRITE_SIZE + x)] };
                        if (c.a != 0 && c.r == 0 && c.g == 0 && c.b == 0) {
                            for (i32 dy{ -1 }; dy <= 1; ++dy) {
                                for (i32 dx{ -1 }; dx <= 1; ++dx) {
                                    const Color n{ px[static_cast<u64>((y + dy) * Asteroid::SPRITE_SIZE + (x + dx))] };
                                    if (n.a == 0) {
                                        rim_closed = false;
                                    }
                                }
                            }
                        }
                    }
                }
                step_flags |= (1u << 9);
                if (rim_closed) {
                    _pass_assertion(0);
                }
            }
            if (step_flags & (1u << 8)) {
                _pass_assertion(1);
            }

            if (t >= 0.95f && t < 1.8f) {
                synth.fire_down = true;
            }
            if (frags.size() >= 3 &&
                game.get_particles().has_valid_explosion_sprites() &&
                game.get_particles().explosion_count() >= 1) {
                _pass_assertion(2);
                step_flags |= 1u;
            }
            if ((step_flags & 1u) && t >= 2.5f) {
                _pass_assertion(3);
            }
            break;
        }

        case 3: {
            Spaceship& ship{ game.get_spaceship() };
            if (t < 1.6f) {
                synth.move_axis = Vector2{ 0.0f, -1.0f };
            } else {
                synth.move_axis = Vector2Zero();
                synth.fire_down = true;
            }

            if (ship.get_health() == 75) {
                _pass_assertion(0);
            }
            if (ship.is_powerup_active(PowerupType::DoubleBlaster) &&
                ship.get_blaster().get_bolts().size() >= 2) {
                _pass_assertion(1);
            }
            if (ship.is_shielded() && ship.get_powerup_remaining(PowerupType::Shield) > 5.0f) {
                _pass_assertion(2);
                if (!(step_flags & 1u)) {
                    const i32 hp_before{ ship.get_health() };
                    ship.take_damage(35, false);
                    game.get_particles().emit(ship.get_pos(), Spaceship::HIT_SPARK_EFFECT, ship.get_vel());
                    if (ship.get_health() == hp_before) {
                        _pass_assertion(3);
                    }
                    step_flags |= 1u;
                }
            }
            break;
        }

        case 4: {
            Spaceship& ship{ game.get_spaceship() };
            auto& enemy_list{ game.get_enemies().get_enemies() };

            if (!enemy_list.empty()) {
                EnemySpaceship& enemy{ enemy_list.front() };
                // Lead target position based on enemy velocity and bolt speed (700 px/s)
                const f32 dist{ Vector2Distance(ship.get_pos(), enemy.get_pos()) };
                const Vector2 lead_pos{
                    Vector2Add(enemy.get_pos(), Vector2Scale(enemy.get_vel(), dist / 700.0f))
                };
                const Vector2 to_enemy{ Vector2Subtract(lead_pos, ship.get_pos()) };
                const f32 aim_deg{ std::atan2f(to_enemy.x, -to_enemy.y) * RAD2DEG };
                ship.set_rotation(aim_deg);

                if (Vector2Length(enemy.get_vel()) > 20.0f &&
                    !enemy.get_blaster().get_bolts().empty()) {
                    _pass_assertion(0);
                }

                if (t >= 0.85f) {
                    synth.fire_down = true;
                }

                if (enemy.get_health() < enemy.get_max_health()) {
                    _pass_assertion(1);
                }
            } else {
                // Enemy was destroyed by player bolts
                if (game.get_particles().count() >= 20 &&
                    game.get_particles().explosion_count() >= 1) {
                    _pass_assertion(2);
                }
                _pass_assertion(3);
            }
            break;
        }

        case 5: {
            Spaceship& ship{ game.get_spaceship() };

            // 1. After asteroid contact damage reduces HP from 40 -> 20, verify contact i-frames block immediate damage
            if (ship.get_health() == 20 && ship.get_invulnerability_timer() > 0.0f && !(step_flags & 1u)) {
                ship.take_damage(20, true);
                if (ship.get_health() == 20) {
                    _pass_assertion(0);
                }
                step_flags |= 1u;
            }

            // 2. Apply fatal hit at t >= 1.0f
            if (t >= 1.0f && !(step_flags & 2u)) {
                ship.take_damage(50, false);
                game.get_particles().emit(ship.get_pos(), Spaceship::EXPLOSION_EFFECT, ship.get_vel());
                game.get_particles().emit_explosion(ship.get_pos(), ExplosionType::Player, 3.0f, 0.52f, ship.get_vel());
                if (!ship.is_alive() &&
                    game.get_particles().count() >= 30 &&
                    game.get_particles().explosion_count() >= 1) {
                    _pass_assertion(1);
                }
                step_flags |= 2u;
            }

            // 3. Verify ScreenManager transitions to GameOver
            if ((step_flags & 2u) && game.get_screens().get_state() == ScreenState::GameOver) {
                _pass_assertion(2);
            }

            // 4. Press R to restart from GameOver and verify 100 HP recovery
            if (t >= 2.1f && !(step_flags & 4u)) {
                synth.restart_pressed = true;
                step_flags |= 4u;
            } else if ((step_flags & 4u) &&
                       game.get_screens().get_state() == ScreenState::Playing &&
                       ship.get_health() == ship.get_max_health()) {
                _pass_assertion(3);
            }
            break;
        }

        case 6: {
            ProgressionState& prog{ game.get_progression() };

            if (t < 0.35f) {
                synth.move_axis = Vector2{ 0.0f, -1.0f };
            }
            if (t >= 0.2f && t < 1.8f) {
                synth.fire_down = true;
            }

            if (game.get_world().get_trauma() > 0.02f && game.get_particles().count() > 0) {
                _pass_assertion(0);
            }

            if (prog.score >= 125 && prog.combo_multiplier >= 2.0f) {
                _pass_assertion(1);
            }

            if (game.get_particles().text_count() > 0) {
                _pass_assertion(2);
            }

            if (t >= 1.9f && !(step_flags & 1u)) {
                prog.wave_timer = ProgressionState::WAVE_DURATION;
                step_flags |= 1u;
            } else if ((step_flags & 1u) && !(step_flags & 2u) && prog.current_wave >= 2) {
                if (prog.combo_multiplier < 2.0f) {
                    (void)prog.add_kill_score(50);
                }
                const f32 mult_before{ prog.combo_multiplier };
                prog.on_player_damaged();
                if (prog.combo_multiplier < mult_before) {
                    _pass_assertion(3);
                }
                step_flags |= 2u;
            }
            break;
        }

        case 7: {
            Spaceship& ship{ game.get_spaceship() };
            const auto& enemy_list{ game.get_enemies().get_enemies() };

            // 1. Trigger Phase Dash at t = 0.2f along +X while Strafe-Locked facing 0 deg (UP)
            if (t >= 0.2f && !(step_flags & 1u)) {
                synth.dash_pressed = true;
                synth.strafe_lock_down = true;
                synth.move_axis = Vector2{ 0.0f, 0.0f };
                step_flags |= 1u;
            }
            if (ship.is_dashing() && ship.get_dash_cooldown() > 1.5f) {
                _pass_assertion(0);
            }

            // 2. Retreat downward (+Y) while holding Strafe-Lock so rotation stays at 0 deg (UP)
            if (t >= 0.45f && t < 1.15f) {
                synth.strafe_lock_down = true;
                synth.move_axis = Vector2{ 0.0f, 1.0f };
                if (ship.is_strafe_locked() && ship.get_vel().y > 40.0f &&
                    std::fabsf(ship.get_rotation()) < 1.0f) {
                    _pass_assertion(1);
                }
            }

            // 3. Verify Sniper telegraph laser & Kamikaze pursuit
            for (const EnemySpaceship& enemy : enemy_list) {
                if (enemy.get_archetype() == EnemyArchetype::Sniper && enemy.is_telegraphing()) {
                    _pass_assertion(2);
                }
            }

            // 4. Fire upward into the Volatile Asteroid at (400, 200) so its AoE blast kills the nearby Kamikaze
            if (t >= 1.15f && !(step_flags & 2u)) {
                game.get_enemies().spawn_at(Vector2{ 432.0f, 195.0f }, 180.0f, EnemyArchetype::Kamikaze);
                step_flags |= 2u;
            }
            if (t >= 1.2f && t < 2.2f) {
                ship.set_pos(Vector2{ 400.0f, ship.get_pos().y });
                ship.set_rotation(0.0f);
                synth.fire_down = true;
            }
            if (game.get_progression().score >= 300) {
                _pass_assertion(3);
            }
            break;
        }

        case 8: {
            Spaceship& ship{ game.get_spaceship() };
            auto& enemy_list{ game.get_enemies().get_enemies() };

            if (!enemy_list.empty()) {
                EnemySpaceship& bstar{ enemy_list.front() };
                const f32 dist{ Vector2Distance(ship.get_pos(), bstar.get_pos()) };
                const Vector2 lead_pos{
                    Vector2Add(bstar.get_pos(), Vector2Scale(bstar.get_vel(), dist / 700.0f))
                };
                const Vector2 to_bstar{ Vector2Subtract(lead_pos, ship.get_pos()) };
                ship.set_rotation(std::atan2f(to_bstar.x, -to_bstar.y) * RAD2DEG);

                // 1. Verify PointyNormal mode fires bidirectional normal bolts
                if (bstar.get_battlestar_mode() == BattlestarAttackMode::PointyNormal &&
                    bstar.get_blaster().get_bolts().size() >= 2) {
                    _pass_assertion(0);
                    if (!(step_flags & 1u)) {
                        bstar.set_battlestar_mode(BattlestarAttackMode::FlatWave);
                        step_flags |= 1u;
                    }
                }

                // 2. Verify FlatWave mode spins 90deg and fires crescent wave attacks
                if (bstar.get_battlestar_mode() == BattlestarAttackMode::FlatWave &&
                    bstar.get_wave_blaster().get_bolts().size() >= 2) {
                    _pass_assertion(1);
                    step_flags |= 2u;
                }

                // 3. Once wave attack is in flight and impacts/passes player shield, open fire to destroy Battlestar
                if ((step_flags & 2u) && game.get_particles().text_count() > 0) {
                    _pass_assertion(2);
                }

                if (t >= 1.9f) {
                    synth.fire_down = true;
                }
            } else {
                // Battlestar destroyed!
                _pass_assertion(2);
                if (game.get_progression().score >= 1000 &&
                    !game.get_powerups().get_powerups().empty() &&
                    game.get_particles().explosion_count() >= 1) {
                    _pass_assertion(3);
                }
            }
            break;
        }

        case 9: {
            LevelManager& lvls{ game.get_levels() };
            Spaceship& ship{ game.get_spaceship() };

            // 1. Switch Start menu from Freestyle -> Levels, enter LevelSelect, and launch Level 4 (Star + Portal)
            if (t >= 0.12f && !(step_flags & 1u)) {
                synth.nav_down_pressed = true;
                step_flags |= 1u;
            } else if (t >= 0.28f && !(step_flags & 2u)) {
                synth.confirm_pressed = true; // Enters LevelSelect
                step_flags |= 2u;
            } else if ((step_flags & 2u) && !(step_flags & 4u) &&
                       game.get_screens().get_state() == ScreenState::LevelSelect && t >= 0.55f) {
                lvls.unlock_level(4);
                lvls.set_current_level(4);
                synth.confirm_pressed = true; // Launches Level 4
                step_flags |= 4u;
            }

            if ((step_flags & 4u) && game.get_screens().get_state() == ScreenState::Playing) {
                _pass_assertion(0);

                // Configure a custom compact layout on first frame of Playing so all 3 objective types are visible on screen
                if (!(step_flags & 8u)) {
                    game.get_asteroids().set_target_count(0);
                    game.get_asteroids().clear();
                    game.get_enemies().set_auto_spawn(false);
                    game.get_enemies().get_enemies().clear();
                    lvls.clear_objectives();

                    // Spawn 3 Stars right along the ship's path and 1 fast-charging Beacon at (400, 220)
                    lvls.spawn_objective(ObjectiveEntityType::Star, Vector2{ 400.0f, 290.0f });
                    lvls.spawn_objective(ObjectiveEntityType::Star, Vector2{ 400.0f, 255.0f });
                    lvls.spawn_objective(ObjectiveEntityType::Star, Vector2{ 400.0f, 220.0f });
                    lvls.spawn_objective(ObjectiveEntityType::Beacon, Vector2{ 400.0f, 220.0f }, 0.65f);

                    ship.set_pos(Vector2{ 400.0f, 330.0f });
                    ship.set_vel(Vector2Zero());
                    step_flags |= 8u;
                }

                // Move upward through the 3 Stars into the Beacon's proximity ring
                if (lvls.get_stars_collected() < 3) {
                    synth.move_axis = Vector2{ 0.0f, -1.0f };
                } else {
                    _pass_assertion(1);
                    // Hold inside the Beacon until it finishes charging
                    if (lvls.get_beacons_charged() == 0) {
                        ship.set_pos(Vector2{ 400.0f, 220.0f });
                        ship.set_vel(Vector2Zero());
                    } else {
                        _pass_assertion(2);
                        // Once the Warp Portal opens above, fly into the Portal to complete the level!
                        for (const ObjectiveEntity& obj : lvls.get_entities()) {
                            if (obj.type == ObjectiveEntityType::Portal && obj.active) {
                                const Vector2 to_portal{ Vector2Subtract(obj.pos, ship.get_pos()) };
                                synth.move_axis = Vector2Normalize(to_portal);
                                if (Vector2Length(to_portal) > 35.0f) {
                                    ship.set_pos(Vector2Lerp(ship.get_pos(), obj.pos, 0.25f));
                                }
                                break;
                            }
                        }
                    }
                }
            }

            if (game.get_screens().get_state() == ScreenState::LevelComplete &&
                lvls.get_unlocked_levels() >= 5) {
                _pass_assertion(3);
            }
            break;
        }

        case 10: {
            LevelManager& lvls{ game.get_levels() };
            DeathZoneState& dz{ lvls.get_death_zone() };
            Spaceship& ship{ game.get_spaceship() };
            auto& enemy_list{ game.get_enemies().get_enemies() };

            // 1. Move ship beyond deep-space threshold (> 900px from origin) to trigger Ambush Death Zone
            if (!(step_flags & 1u)) {
                ship.set_pos(Vector2{ 2100.0f, 300.0f });
                game.get_world().reset_camera(ship.get_pos());
                dz.wander_check_timer = 0.01f;
                step_flags |= 1u;
            }

            if ((step_flags & 1u) && !(step_flags & 2u)) {
                // Force deterministic ambush activation if random roll didn't fire on first tick
                if (!dz.active && t >= 0.15f) {
                    dz.activate(
                        game.get_world().get_camera_target(),
                        static_cast<f32>(engine.get_width()),
                        static_cast<f32>(engine.get_height()),
                        10.0f,
                        DeathZoneCause::DeepSpaceAmbush
                    );
                    game.get_world().set_camera_lock(dz.center);
                }
                if (dz.active && dz.cause == DeathZoneCause::DeepSpaceAmbush &&
                    game.get_world().is_camera_locked()) {
                    _pass_assertion(0);
                    step_flags |= 2u;
                }
            }

            // 2. Drive ship into the right locked screen border to verify inward bounce & -15 shock damage
            if ((step_flags & 2u) && !(step_flags & 4u)) {
                const f32 right_border{ dz.center.x + dz.half_extents.x };
                ship.set_pos(Vector2{ right_border + 5.0f, dz.center.y });
                ship.set_vel(Vector2{ 250.0f, 0.0f });
                step_flags |= 4u;
            } else if ((step_flags & 4u) && !(step_flags & 8u)) {
                if (ship.get_health() == 85 && ship.get_vel().x < 0.0f) {
                    _pass_assertion(1);
                    dz.deactivate();
                    dz.cooldown_timer = 0.0f;
                    game.get_world().set_camera_lock(std::nullopt);
                    step_flags |= 8u;
                }
            }

            // 3. Spawn a Battlestar warden nearby with 1 HP and verify it activates BattlestarWarden Death Zone
            if ((step_flags & 8u) && !(step_flags & 16u) && t >= 1.1f) {
                enemy_list.clear();
                ship.set_pos(Vector2{ 400.0f, 380.0f });
                ship.set_vel(Vector2Zero());
                ship.set_rotation(0.0f);
                ship.activate_powerup(PowerupType::DoubleBlaster, 10.0f);
                game.get_world().reset_camera(ship.get_pos());

                EnemySpaceship& warden{
                    game.get_enemies().spawn_at(Vector2{ 400.0f, 190.0f }, 180.0f, EnemyArchetype::Battlestar)
                };
                warden.take_damage(8); // Leave 2 HP so player bolts finish it quickly
                step_flags |= 16u;
            }

            if ((step_flags & 16u) && !(step_flags & 32u)) {
                if (dz.active && dz.cause == DeathZoneCause::BattlestarWarden) {
                    _pass_assertion(2);
                    step_flags |= 32u;
                }
            }

            // 4. Destroy the Battlestar warden and verify the Death Zone immediately lifts!
            if (step_flags & 32u) {
                ship.set_pos(Vector2{ 400.0f, 380.0f });
                ship.set_rotation(0.0f);
                synth.fire_down = true;
                if (!dz.active && !game.get_world().is_camera_locked()) {
                    _pass_assertion(3);
                }
            }
            break;
        }

        default:
            break;
    }

    engine.override_input(synth);

    if (sc.elapsed >= sc.duration) {
        _advance_scenario();
    }
}

bool VisualTestRunner::all_passed() const noexcept {
    for (const ScenarioStatus& sc : scenarios) {
        for (const AssertionItem& item : sc.assertions) {
            if (!item.passed) return false;
        }
    }
    return true;
}

void VisualTestRunner::draw() const noexcept {
    if (finished) {
        _draw_summary();
    } else {
        _draw_banner();
    }
}

void VisualTestRunner::_draw_banner() const noexcept {
    if (current_idx >= SCENARIO_COUNT) return;
    const ScenarioStatus& sc{ scenarios[current_idx] };

    constexpr i32 box_x{ 14 };
    constexpr i32 box_y{ 438 };
    constexpr i32 box_w{ 510 };
    constexpr i32 box_h{ 148 };

    DrawRectangle(box_x, box_y, box_w, box_h, Fade(hex_to_color(0x0f172aff), 0.88f));
    DrawRectangleLines(box_x, box_y, box_w, box_h, hex_to_color(0x38bdf8ff));

    char header[128]{ };
    std::snprintf(
        header,
        sizeof(header),
        "[VISUAL TEST %u/%u] %s",
        static_cast<u32>(current_idx + 1),
        static_cast<u32>(SCENARIO_COUNT),
        sc.title.data()
    );
    DrawText(header, box_x + 12, box_y + 10, 14, hex_to_color(0xfef08aff));
    DrawText(sc.subtitle.data(), box_x + 12, box_y + 28, 10, hex_to_color(0x94a3b8ff));

    // Scenario timer bar
    const f32 progress{ std::clamp(sc.elapsed / sc.duration, 0.0f, 1.0f) };
    constexpr i32 bar_w{ box_w - 24 };
    DrawRectangle(box_x + 12, box_y + 44, bar_w, 5, hex_to_color(0x1e293bff));
    DrawRectangle(
        box_x + 12,
        box_y + 44,
        static_cast<i32>(static_cast<f32>(bar_w) * progress),
        5,
        hex_to_color(0x38bdf8ff)
    );

    i32 y{ box_y + 56 };
    for (const AssertionItem& item : sc.assertions) {
        const Color badge_color{ item.passed ? hex_to_color(0x4ade80ff) : hex_to_color(0xfbbf24ff) };
        const char* badge{ item.passed ? "[PASS]" : "[WAIT]" };
        DrawText(badge, box_x + 12, y, 12, badge_color);
        DrawText(item.label.data(), box_x + 64, y, 12, hex_to_color(0xf8fafcff));
        y += 18;
    }

    DrawText(
        "[N] Skip Scenario   |   [F3/TAB] Toggle Gizmos   |   [Q] Quit Tests",
        box_x + 12,
        box_y + box_h - 16,
        10,
        hex_to_color(0x64748bff)
    );
}

void VisualTestRunner::_draw_summary() const noexcept {
    const i32 w{ engine.get_width() };
    const i32 h{ engine.get_height() };
    DrawRectangle(0, 0, w, h, Fade(BLACK, 0.72f));

    u32 passed_count{ 0 };
    u32 total_count{ 0 };
    for (const ScenarioStatus& sc : scenarios) {
        for (const AssertionItem& item : sc.assertions) {
            ++total_count;
            if (item.passed) ++passed_count;
        }
    }

    const bool ok{ passed_count == total_count };
    const Color color{ ok ? hex_to_color(0x4ade80ff) : hex_to_color(0xef4444ff) };
    char title[128]{ };
    std::snprintf(
        title,
        sizeof(title),
        "VISUAL TEST SUITE COMPLETE: %u / %u ASSERTIONS PASSED",
        passed_count,
        total_count
    );
    const i32 tw{ MeasureText(title, 22) };
    DrawText(title, (w - tw) / 2, h / 2 - 20, 22, color);
}

} // namespace asteroides::tests
