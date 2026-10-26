#pragma once

#include <array>
#include <cmath>
#include <numbers>
#include <string_view>
#include <utility>
#include <vector>

#include "engine.hh"
#include "raylib.h"
#include "raymath.h"
#include "spaceship.hh"
#include "sprite.hh"

namespace asteroides {

struct PowerupConfig {
    PowerupType type;
    std::string_view asset_name;
    f32 drop_chance;
    f32 world_ttl;
    f32 effect_duration;
};

struct Powerup {
    PowerupType type{ PowerupType::DoubleBlaster };
    assets::Sprite sprite{ };
    Vector2 pos{ Vector2Zero() };
    Vector2 vel{ Vector2Zero() };
    f32 scale{ 1.5f };
    f32 radius{ 18.0f };
    f32 ttl{ 8.0f };
    f32 max_ttl{ 8.0f };
    f32 effect_duration{ 10.0f };
    f32 pulse_phase{ 0.0f };
};

struct CollectedPowerup {
    PowerupType type{ PowerupType::DoubleBlaster };
    Vector2 pos{ Vector2Zero() };
};

struct PowerupField {
    public:
        static constexpr u64 POWERUP_COUNT{ static_cast<u64>(PowerupType::Count) };

        static constexpr std::array<PowerupConfig, POWERUP_COUNT> DEFAULT_CONFIGS{ {
            {
                .type = PowerupType::DoubleBlaster,
                .asset_name = "double_blaster_powerup.png",
                .drop_chance = 0.01f,
                .world_ttl = 8.0f,
                .effect_duration = 10.0f
            },
            {
                .type = PowerupType::IncreaseHealth,
                .asset_name = "increase_health_powerup.png",
                .drop_chance = 0.05f,
                .world_ttl = 8.0f,
                .effect_duration = 0.0f
            },
            {
                .type = PowerupType::Shield,
                .asset_name = "shield_powerup.png",
                .drop_chance = 0.01f,
                .world_ttl = 8.0f,
                .effect_duration = 10.0f
            }
        } };

        PowerupField() noexcept = default;

        void set_drop_chance(PowerupType type, f32 chance) noexcept;
        [[nodiscard]] f32 get_drop_chance(PowerupType type) const noexcept;

        void try_spawn(Vector2 origin) noexcept;
        void spawn_guaranteed(Vector2 origin) noexcept;
        void spawn_specific(PowerupType type, Vector2 origin, Vector2 vel = Vector2Zero()) noexcept;
        void update(f32 dt, Spaceship& spaceship) noexcept;
        void draw() const noexcept;
        void clear() noexcept {
            powerups.clear();
            collected_events.clear();
        }
        [[nodiscard]] std::vector<CollectedPowerup> consume_collected() noexcept {
            return std::exchange(collected_events, { });
        }
        [[nodiscard]] const std::vector<Powerup>& get_powerups() const noexcept { return powerups; }

    private:
        std::array<PowerupConfig, POWERUP_COUNT> configs{ DEFAULT_CONFIGS };
        std::vector<Powerup> powerups{ };
        std::vector<CollectedPowerup> collected_events{ };
};

} // namespace asteroides
