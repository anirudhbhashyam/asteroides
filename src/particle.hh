#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <numbers>
#include <string_view>
#include <utility>
#include <vector>

#include "engine.hh"
#include "raylib.h"
#include "raymath.h"
#include "sprite.hh"

namespace asteroides {

enum class ExplosionType : u8 {
    Player = 0,
    Enemy,
    Battlestar,
    Volatile,
    Asteroid,
    Count
};

struct AnimatedExplosion {
    ExplosionType type{ ExplosionType::Enemy };
    Vector2 pos{ Vector2Zero() };
    Vector2 vel{ Vector2Zero() };
    f32 scale{ 2.6f };
    f32 rotation{ 0.0f };
    f32 elapsed{ 0.0f };
    f32 duration{ 0.45f };
    u32 frame_index{ 0 };
};

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

struct FloatingText {
    Vector2 pos{ Vector2Zero() };
    Vector2 vel{ 0.0f, -44.0f };
    f32 ttl{ 0.9f };
    f32 max_ttl{ 0.9f };
    i32 font_size{ 14 };
    Color color{ WHITE };
    std::array<char, 32> text{ };
};

struct ParticleSystem {
    public:
        static constexpr f32 EXPLOSION_FRAME_SIZE{ 32.0f };
        static constexpr u32 EXPLOSION_FRAME_COUNT{ 6 };
        static constexpr std::string_view PLAYER_EXPLOSION_ASSET{ "explosion_player.png" };
        static constexpr std::string_view ENEMY_EXPLOSION_ASSET{ "explosion_enemy.png" };
        static constexpr std::string_view BATTLESTAR_EXPLOSION_ASSET{ "explosion_battlestar.png" };
        static constexpr std::string_view VOLATILE_EXPLOSION_ASSET{ "explosion_volatile.png" };
        static constexpr std::string_view ASTEROID_EXPLOSION_ASSET{ "explosion_asteroid.png" };

        ParticleSystem() = default;

        void emit(Vector2 origin, const ParticleEffect& effect, Vector2 inherited_vel = Vector2Zero()) noexcept;
        void emit_explosion(
            Vector2 origin,
            ExplosionType type,
            f32 scale = 2.6f,
            f32 duration = 0.45f,
            Vector2 inherited_vel = Vector2Zero()
        ) noexcept;
        void emit_text(Vector2 origin, std::string_view message, Color color, i32 font_size = 14) noexcept;
        void update(f32 dt) noexcept;
        void draw() const noexcept;
        void clear() noexcept;
        [[nodiscard]] const std::vector<Particle>& get_particles() const noexcept { return particles; }
        [[nodiscard]] const std::vector<AnimatedExplosion>& get_explosions() const noexcept { return explosions; }
        [[nodiscard]] u64 count() const noexcept { return static_cast<u64>(particles.size()); }
        [[nodiscard]] u64 explosion_count() const noexcept { return static_cast<u64>(explosions.size()); }
        [[nodiscard]] u64 text_count() const noexcept { return static_cast<u64>(texts.size()); }
        [[nodiscard]] bool has_valid_explosion_sprites() const noexcept {
            return player_explosion_sprite.is_valid() &&
                   enemy_explosion_sprite.is_valid() &&
                   battlestar_explosion_sprite.is_valid() &&
                   volatile_explosion_sprite.is_valid() &&
                   asteroid_explosion_sprite.is_valid();
        }

    private:
        [[nodiscard]] const assets::Sprite& _get_explosion_sprite(ExplosionType type) const noexcept;

        assets::Sprite player_explosion_sprite{ PLAYER_EXPLOSION_ASSET };
        assets::Sprite enemy_explosion_sprite{ ENEMY_EXPLOSION_ASSET };
        assets::Sprite battlestar_explosion_sprite{ BATTLESTAR_EXPLOSION_ASSET };
        assets::Sprite volatile_explosion_sprite{ VOLATILE_EXPLOSION_ASSET };
        assets::Sprite asteroid_explosion_sprite{ ASTEROID_EXPLOSION_ASSET };
        std::vector<Particle> particles{ };
        std::vector<AnimatedExplosion> explosions{ };
        std::vector<FloatingText> texts{ };
};

} // namespace asteroides
