#pragma once

#include <array>
#include <cmath>
#include <numbers>
#include <vector>

#include "engine.hh"
#include "raylib.h"
#include "raymath.h"

namespace asteroides {

struct Particle {
    Vector2 pos{ Vector2Zero() };
    Vector2 vel{ Vector2Zero() };
    f32 size{ 3.0f };
    f32 rotation{ 0.0f };
    f32 angular_vel{ 0.0f };
    f32 ttl{ 0.6f };
    f32 max_ttl{ 0.6f };
    f32 drag{ 2.5f };
    bool shrink{ true };
    Color start_color{ WHITE };
    Color end_color{ WHITE };
};

struct ParticleEffect {
    u32 count_min{ 16 };
    u32 count_max{ 28 };
    f32 speed_min{ 50.0f };
    f32 speed_max{ 220.0f };
    f32 size_min{ 2.0f };
    f32 size_max{ 6.0f };
    f32 ttl_min{ 0.35f };
    f32 ttl_max{ 0.85f };
    f32 drag{ 2.5f };
    f32 direction_deg{ 0.0f };
    f32 spread_deg{ 360.0f };
    bool shrink{ true };
    std::array<Color, 4> start_palette{ {
        WHITE, WHITE, WHITE, WHITE
    } };
    Color end_color{ hex_to_color(0x334155ff) };
};

struct ParticleSystem {
    public:
        ParticleSystem() noexcept = default;

        void emit(Vector2 origin, const ParticleEffect& effect, Vector2 inherited_vel = Vector2Zero()) noexcept;
        void update(f32 dt) noexcept;
        void draw() const noexcept;
        void clear() noexcept;
        [[nodiscard]] const std::vector<Particle>& get_particles() const noexcept { return particles; }
        [[nodiscard]] u64 count() const noexcept { return static_cast<u64>(particles.size()); }

    private:
        std::vector<Particle> particles{ };
};

} // namespace asteroides
