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

        void update(f32 dt) noexcept;
        void draw() const noexcept;
        void reset() noexcept;

    private:
        void _handle_combat(Vector2 view_center) noexcept;

        ScreenManager screens{ };
        Spaceship spaceship{ };
        World world;
        ParticleSystem particles{ };
        PowerupField powerups{ };
        AsteroidField asteroids;
        EnemyField enemies{ };
};

} // namespace asteroides
