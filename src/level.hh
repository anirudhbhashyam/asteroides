#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <optional>
#include <string_view>
#include <vector>

#include "asteroid.hh"
#include "enemy.hh"
#include "engine.hh"
#include "particle.hh"
#include "raylib.h"
#include "raymath.h"
#include "spaceship.hh"
#include "sprite.hh"
#include "world.hh"

namespace asteroides {

enum class GameMode : u8 {
    Freestyle = 0,
    Levels
};

enum class ObjectiveEntityType : u8 {
    Star = 0,
    Beacon,
    Portal
};

enum class DeathZoneCause : u8 {
    None = 0,
    BattlestarWarden,
    DeepSpaceAmbush
};

struct ObjectiveEntity {
    ObjectiveEntityType type{ ObjectiveEntityType::Star };
    Vector2 pos{ Vector2Zero() };
    f32 radius{ 26.0f };
    f32 rotation{ 0.0f };
    f32 pulse_timer{ 0.0f };
    f32 charge_progress{ 0.0f };
    f32 charge_duration{ 3.5f };
    f32 proximity_radius{ 145.0f };
    bool active{ true };
    bool completed{ false };
};

struct LevelDefinition {
    u32 level_number{ 1 };
    std::string_view title{ "Starlight Scavenger" };
    std::string_view objective_summary{ "Find & collect 3 Astral Stars" };
    u64 asteroid_count{ 8 };
    u64 max_enemies{ 2 };
    f32 spawn_interval{ 6.0f };
    u32 target_stars{ 3 };
    u32 target_beacons{ 0 };
    bool stars_from_asteroids{ false };
    bool requires_portal_escape{ false };
    bool requires_battlestar_boss{ false };
};

struct DeathZoneState {
    static constexpr f32 BORDER_INSET{ 16.0f };
    static constexpr i32 BORDER_DAMAGE{ 15 };
    static constexpr f32 DEEP_SPACE_THRESHOLD{ 900.0f };

    bool active{ false };
    DeathZoneCause cause{ DeathZoneCause::None };
    Vector2 center{ Vector2Zero() };
    Vector2 half_extents{ 960.0f, 540.0f };
    f32 timer{ 0.0f };
    f32 max_duration{ 12.0f };
    f32 cooldown_timer{ 0.0f };
    f32 wander_check_timer{ 2.5f };
    f32 pulse_timer{ 0.0f };

    void reset() noexcept;
    void activate(Vector2 lock_center, f32 screen_w, f32 screen_h, f32 duration, DeathZoneCause why) noexcept;
    void deactivate() noexcept;
    [[nodiscard]] bool update(f32 dt, bool warden_alive) noexcept;
    [[nodiscard]] bool enforce_player_bounds(
        Spaceship& spaceship,
        ParticleSystem& particles,
        World& world,
        Engine& engine
    ) noexcept;
    void draw_world() const noexcept;
};

struct LevelManager {
    public:
        static constexpr u32 TOTAL_LEVELS{ 5 };
        static constexpr std::string_view STAR_ASSET{ "objective_star.png" };
        static constexpr std::string_view BEACON_ASSET{ "objective_beacon.png" };
        static constexpr std::string_view PORTAL_ASSET{ "objective_portal.png" };

        static constexpr std::array<LevelDefinition, TOTAL_LEVELS> LEVELS{ {
            {
                .level_number = 1,
                .title = "Starlight Scavenger",
                .objective_summary = "Find & collect 3 Astral Stars across the sector",
                .asteroid_count = 8,
                .max_enemies = 2,
                .spawn_interval = 6.2f,
                .target_stars = 3,
                .target_beacons = 0,
                .stars_from_asteroids = false,
                .requires_portal_escape = false,
                .requires_battlestar_boss = false
            },
            {
                .level_number = 2,
                .title = "Signal in the Void",
                .objective_summary = "Hold inside 2 Subspace Beacons to charge them to 100%",
                .asteroid_count = 10,
                .max_enemies = 3,
                .spawn_interval = 4.8f,
                .target_stars = 0,
                .target_beacons = 2,
                .stars_from_asteroids = false,
                .requires_portal_escape = false,
                .requires_battlestar_boss = false
            },
            {
                .level_number = 3,
                .title = "Volatile Harvest",
                .objective_summary = "Shatter asteroids to unearth & collect 3 Star Cores",
                .asteroid_count = 14,
                .max_enemies = 4,
                .spawn_interval = 4.2f,
                .target_stars = 3,
                .target_beacons = 0,
                .stars_from_asteroids = true,
                .requires_portal_escape = false,
                .requires_battlestar_boss = false
            },
            {
                .level_number = 4,
                .title = "Deep-Space Heist",
                .objective_summary = "Recover 3 Deep-Space Stars & escape via the Warp Portal",
                .asteroid_count = 13,
                .max_enemies = 5,
                .spawn_interval = 3.6f,
                .target_stars = 3,
                .target_beacons = 0,
                .stars_from_asteroids = false,
                .requires_portal_escape = true,
                .requires_battlestar_boss = false
            },
            {
                .level_number = 5,
                .title = "Battlestar Nexus",
                .objective_summary = "Charge the Nexus Beacon, defeat the Battlestar & claim the Nova Star",
                .asteroid_count = 12,
                .max_enemies = 4,
                .spawn_interval = 3.8f,
                .target_stars = 1,
                .target_beacons = 1,
                .stars_from_asteroids = false,
                .requires_portal_escape = true,
                .requires_battlestar_boss = true
            }
        } };

        static constexpr ParticleEffect STAR_COLLECT_EFFECT{
            .count_min = 20,
            .count_max = 32,
            .speed_min = 60.0f,
            .speed_max = 220.0f,
            .size_min = 2.5f,
            .size_max = 6.0f,
            .ttl_min = 0.35f,
            .ttl_max = 0.75f,
            .drag = 2.5f,
            .direction_deg = 0.0f,
            .spread_deg = 360.0f,
            .shrink = true,
            .start_palette = { {
                hex_to_color(0xfef08aff),
                hex_to_color(0xfacc15ff),
                hex_to_color(0x38bdf8ff),
                hex_to_color(0xf8fafcff)
            } },
            .end_color = hex_to_color(0x1e293bff)
        };

        LevelManager() = default;

        void setup_run(
            Vector2 player_pos,
            AsteroidField& asteroids,
            EnemyField& enemies
        ) noexcept;

        void update(
            f32 dt,
            Spaceship& spaceship,
            EnemyField& enemies,
            ParticleSystem& particles,
            World& world,
            Engine& engine,
            u64& score_out
        ) noexcept;

        void on_asteroid_shattered(Vector2 pos, AsteroidVariant variant, bool by_player) noexcept;
        void on_enemy_killed(Vector2 pos, EnemyArchetype archetype, ParticleSystem& particles) noexcept;
        void draw_world() const noexcept;

        ObjectiveEntity& spawn_objective(ObjectiveEntityType type, Vector2 pos, f32 charge_time = 3.5f) noexcept;
        void clear_objectives() noexcept { entities.clear(); }

        void set_mode(GameMode new_mode) noexcept { mode = new_mode; }
        [[nodiscard]] GameMode get_mode() const noexcept { return mode; }

        void set_current_level(u32 lvl) noexcept {
            current_level = std::clamp(lvl, 1u, TOTAL_LEVELS);
        }
        [[nodiscard]] u32 get_current_level() const noexcept { return current_level; }
        [[nodiscard]] u32 get_unlocked_levels() const noexcept { return unlocked_levels; }
        void unlock_level(u32 lvl) noexcept {
            unlocked_levels = std::clamp(std::max(unlocked_levels, lvl), 1u, TOTAL_LEVELS);
        }
        [[nodiscard]] bool is_level_completed_once(u32 lvl) const noexcept {
            return (lvl >= 1 && lvl <= TOTAL_LEVELS) && ((completed_mask & (1u << (lvl - 1))) != 0);
        }

        [[nodiscard]] const LevelDefinition& get_definition() const noexcept {
            return LEVELS[std::clamp(current_level, 1u, TOTAL_LEVELS) - 1];
        }
        [[nodiscard]] static const LevelDefinition& get_definition(u32 lvl) noexcept {
            return LEVELS[std::clamp(lvl, 1u, TOTAL_LEVELS) - 1];
        }

        [[nodiscard]] bool consume_level_just_completed() noexcept {
            return std::exchange(level_just_completed, false);
        }
        [[nodiscard]] u32 get_stars_collected() const noexcept { return stars_collected; }
        [[nodiscard]] u32 get_beacons_charged() const noexcept { return beacons_charged; }
        [[nodiscard]] bool is_portal_active() const noexcept { return portal_spawned; }
        [[nodiscard]] bool is_boss_spawned() const noexcept { return boss_spawned; }
        [[nodiscard]] bool is_boss_defeated() const noexcept { return boss_defeated; }
        [[nodiscard]] f32 get_objective_progress_ratio() const noexcept;
        void format_objective_status(char* buf, u64 buf_size) const noexcept;

        [[nodiscard]] const std::vector<ObjectiveEntity>& get_entities() const noexcept { return entities; }
        [[nodiscard]] std::vector<ObjectiveEntity>& get_entities() noexcept { return entities; }
        [[nodiscard]] const DeathZoneState& get_death_zone() const noexcept { return death_zone; }
        [[nodiscard]] DeathZoneState& get_death_zone() noexcept { return death_zone; }

    private:
        assets::Sprite star_sprite{ STAR_ASSET };
        assets::Sprite beacon_sprite{ BEACON_ASSET };
        assets::Sprite portal_sprite{ PORTAL_ASSET };

        GameMode mode{ GameMode::Freestyle };
        u32 current_level{ 1 };
        u32 unlocked_levels{ 1 };
        u8 completed_mask{ 0 };

        std::vector<ObjectiveEntity> entities{ };
        DeathZoneState death_zone{ };

        u32 stars_collected{ 0 };
        u32 stars_spawned_from_asteroids{ 0 };
        u32 beacons_charged{ 0 };
        bool boss_spawned{ false };
        bool boss_defeated{ false };
        bool portal_spawned{ false };
        bool level_just_completed{ false };
        Vector2 origin_pos{ 960.0f, 540.0f };
};

} // namespace asteroides
