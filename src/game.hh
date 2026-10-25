#pragma once

#include "asteroid.hh"
#include "enemy.hh"
#include "engine.hh"
#include "particle.hh"
#include "powerup.hh"
#include "raylib.h"
#include "raymath.h"
#include "screen.hh"
#include "spaceship.hh"
#include "world.hh"

namespace asteroides {

struct Game {
    public:
        explicit Game(u64 star_count = 120, u64 asteroid_count = 12);

        void update(const Engine& engine) noexcept;
        void draw(const Engine& engine) const noexcept;
        void reset() noexcept;

        [[nodiscard]] ScreenManager& get_screens() noexcept { return screens; }
        [[nodiscard]] const ScreenManager& get_screens() const noexcept { return screens; }
        [[nodiscard]] Spaceship& get_spaceship() noexcept { return spaceship; }
        [[nodiscard]] const Spaceship& get_spaceship() const noexcept { return spaceship; }
        [[nodiscard]] World& get_world() noexcept { return world; }
        [[nodiscard]] const World& get_world() const noexcept { return world; }
        [[nodiscard]] ParticleSystem& get_particles() noexcept { return particles; }
        [[nodiscard]] const ParticleSystem& get_particles() const noexcept { return particles; }
        [[nodiscard]] PowerupField& get_powerups() noexcept { return powerups; }
        [[nodiscard]] const PowerupField& get_powerups() const noexcept { return powerups; }
        [[nodiscard]] AsteroidField& get_asteroids() noexcept { return asteroids; }
        [[nodiscard]] const AsteroidField& get_asteroids() const noexcept { return asteroids; }
        [[nodiscard]] EnemyField& get_enemies() noexcept { return enemies; }
        [[nodiscard]] const EnemyField& get_enemies() const noexcept { return enemies; }

    private:
        void _handle_combat(Vector2 view_center) noexcept;
        void _draw_debug_world(const Engine& engine) const noexcept;
        [[nodiscard]] TelemetryData _build_telemetry() const noexcept;

        ScreenManager screens{ };
        Spaceship spaceship{ };
        World world;
        ParticleSystem particles{ };
        PowerupField powerups{ };
        AsteroidField asteroids;
        EnemyField enemies{ };
};

} // namespace asteroides
