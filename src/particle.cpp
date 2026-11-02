#include "particle.hh"

namespace asteroides {

const assets::Sprite& ParticleSystem::_get_explosion_sprite(ExplosionType type) const noexcept {
    switch (type) {
        case ExplosionType::Player:
            return player_explosion_sprite;
        case ExplosionType::Battlestar:
            return battlestar_explosion_sprite;
        case ExplosionType::Volatile:
            return volatile_explosion_sprite;
        case ExplosionType::Asteroid:
            return asteroid_explosion_sprite;
        case ExplosionType::Enemy:
        default:
            return enemy_explosion_sprite;
    }
}

void ParticleSystem::emit(Vector2 origin, const ParticleEffect& effect, Vector2 inherited_vel) noexcept {
    const u32 count{ Random::uint(effect.count_min, effect.count_max) };
    const f32 half_spread{ effect.spread_deg * 0.5f };
    const u32 palette_max{ static_cast<u32>(effect.start_palette.size() - 1) };

    for (u32 i{ 0 }; i < count; ++i) {
        const f32 angle_deg{
            effect.direction_deg + Random::real(-half_spread, half_spread)
        };
        const f32 angle_rad{ angle_deg * DEG2RAD };
        const Vector2 dir{ std::sinf(angle_rad), -std::cosf(angle_rad) };
        const f32 speed{ Random::real(effect.speed_min, effect.speed_max) };
        const f32 lifetime{ Random::real(effect.ttl_min, effect.ttl_max) };

        particles.emplace_back(
            Vector2Add(origin, Vector2Scale(dir, Random::real(0.0f, 4.0f))),
            Vector2Add(Vector2Scale(dir, speed), Vector2Scale(inherited_vel, 0.35f)),
            Random::real(effect.size_min, effect.size_max),
            Random::real(0.0f, 360.0f),
            Random::real(-360.0f, 360.0f),
            lifetime,
            lifetime,
            effect.drag,
            effect.shrink,
            effect.start_palette[Random::uint(0, palette_max)],
            effect.end_color
        );
    }
}

void ParticleSystem::emit_explosion(
    Vector2 origin,
    ExplosionType type,
    f32 scale,
    f32 duration,
    Vector2 inherited_vel
) noexcept {
    explosions.emplace_back(
        type,
        origin,
        Vector2Scale(inherited_vel, 0.25f),
        scale,
        static_cast<f32>(Random::uint(0, 3)) * 90.0f,
        0.0f,
        std::fmaxf(0.12f, duration),
        0u
    );
}

void ParticleSystem::emit_text(Vector2 origin, std::string_view message, Color color, i32 font_size) noexcept {
    FloatingText ft{
        .pos = origin,
        .vel = Vector2{ Random::real(-12.0f, 12.0f), -48.0f },
        .ttl = 0.95f,
        .max_ttl = 0.95f,
        .font_size = font_size,
        .color = color
    };
    const u64 copy_len{ std::min<u64>(static_cast<u64>(message.size()), ft.text.size() - 1) };
    std::memcpy(ft.text.data(), message.data(), copy_len);
    ft.text[copy_len] = '\0';
    texts.emplace_back(std::move(ft));
}

void ParticleSystem::update(f32 dt) noexcept {
    for (Particle& p : particles) {
        p.pos = Vector2Add(p.pos, Vector2Scale(p.vel, dt));
        p.vel = Vector2Scale(p.vel, std::fmaxf(0.0f, 1.0f - p.drag * dt));
        p.rotation = std::remainderf(p.rotation + p.angular_vel * dt, 360.0f);
        p.ttl -= dt;
    }

    std::erase_if(particles, [](const Particle& p) noexcept {
        return p.ttl <= 0.0f;
    });

    for (AnimatedExplosion& ex : explosions) {
        ex.pos = Vector2Add(ex.pos, Vector2Scale(ex.vel, dt));
        ex.vel = Vector2Scale(ex.vel, std::fmaxf(0.0f, 1.0f - 2.2f * dt));
        ex.elapsed += dt;
        const f32 progress{ std::clamp(ex.elapsed / ex.duration, 0.0f, 0.999f) };
        ex.frame_index = static_cast<u32>(progress * static_cast<f32>(EXPLOSION_FRAME_COUNT));
    }

    std::erase_if(explosions, [](const AnimatedExplosion& ex) noexcept {
        return ex.elapsed >= ex.duration;
    });

    for (FloatingText& ft : texts) {
        ft.pos = Vector2Add(ft.pos, Vector2Scale(ft.vel, dt));
        ft.vel = Vector2Scale(ft.vel, std::fmaxf(0.0f, 1.0f - 1.8f * dt));
        ft.ttl -= dt;
    }

    std::erase_if(texts, [](const FloatingText& ft) noexcept {
        return ft.ttl <= 0.0f;
    });
}

void ParticleSystem::draw() const noexcept {
    for (const AnimatedExplosion& ex : explosions) {
        const assets::Sprite& sp{ _get_explosion_sprite(ex.type) };
        if (!sp.is_valid()) continue;
        const Rectangle src{
            static_cast<f32>(ex.frame_index) * EXPLOSION_FRAME_SIZE,
            0.0f,
            EXPLOSION_FRAME_SIZE,
            EXPLOSION_FRAME_SIZE
        };
        sp.draw(ex.pos, src, ex.rotation, ex.scale, WHITE);
    }

    for (const Particle& p : particles) {
        const f32 t{ std::fmaxf(0.0f, std::fminf(1.0f, p.ttl / p.max_ttl)) };
        const f32 progress{ 1.0f - t };
        const f32 smooth_alpha{ t * t * (3.0f - 2.0f * t) };
        const f32 current_size{ p.shrink ? (p.size * (0.3f + 0.7f * t)) : p.size };

        const Color blended{
            .r = static_cast<u8>(static_cast<f32>(p.start_color.r) + (static_cast<f32>(p.end_color.r) - static_cast<f32>(p.start_color.r)) * progress),
            .g = static_cast<u8>(static_cast<f32>(p.start_color.g) + (static_cast<f32>(p.end_color.g) - static_cast<f32>(p.start_color.g)) * progress),
            .b = static_cast<u8>(static_cast<f32>(p.start_color.b) + (static_cast<f32>(p.end_color.b) - static_cast<f32>(p.start_color.b)) * progress),
            .a = 255
        };

        const Rectangle rect{ p.pos.x, p.pos.y, current_size, current_size };
        const Vector2 origin{ current_size * 0.5f, current_size * 0.5f };
        DrawRectanglePro(rect, origin, p.rotation, Fade(blended, smooth_alpha));
    }

    for (const FloatingText& ft : texts) {
        const f32 t{ std::fmaxf(0.0f, std::fminf(1.0f, ft.ttl / ft.max_ttl)) };
        const f32 alpha{ t * t * (3.0f - 2.0f * t) };
        const i32 tw{ MeasureText(ft.text.data(), ft.font_size) };
        const i32 tx{ static_cast<i32>(ft.pos.x) - tw / 2 };
        const i32 ty{ static_cast<i32>(ft.pos.y) };
        DrawText(ft.text.data(), tx + 1, ty + 1, ft.font_size, Fade(BLACK, alpha * 0.85f));
        DrawText(ft.text.data(), tx, ty, ft.font_size, Fade(ft.color, alpha));
    }
}

void ParticleSystem::clear() noexcept {
    particles.clear();
    explosions.clear();
    texts.clear();
}

} // namespace asteroides
