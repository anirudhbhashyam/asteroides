#include "powerup.hh"

namespace asteroides {

void PowerupField::set_drop_chance(PowerupType type, f32 chance) noexcept {
    const u64 idx{ static_cast<u64>(type) };
    if (idx < POWERUP_COUNT) {
        configs[idx].drop_chance = std::fmaxf(0.0f, std::fminf(1.0f, chance));
    }
}

f32 PowerupField::get_drop_chance(PowerupType type) const noexcept {
    const u64 idx{ static_cast<u64>(type) };
    return (idx < POWERUP_COUNT) ? configs[idx].drop_chance : 0.0f;
}

void PowerupField::try_spawn(Vector2 origin) noexcept {
    const f32 roll{ Random::real() };
    f32 cumulative{ 0.0f };

    for (u64 i{ 0 }; i < POWERUP_COUNT; ++i) {
        cumulative += configs[i].drop_chance;
        if (roll <= cumulative) {
            constexpr f32 two_pi{ 2.0f * std::numbers::pi_v<f32> };
            const f32 angle{ Random::real(0.0f, two_pi) };
            const f32 speed{ Random::real(15.0f, 40.0f) };

            powerups.push_back(Powerup{
                .type = configs[i].type,
                .sprite = assets::Sprite{ configs[i].asset_name },
                .pos = origin,
                .vel = Vector2{ std::cosf(angle) * speed, std::sinf(angle) * speed },
                .scale = 1.5f,
                .radius = 18.0f,
                .ttl = configs[i].world_ttl,
                .max_ttl = configs[i].world_ttl,
                .effect_duration = configs[i].effect_duration,
                .pulse_phase = Random::real(0.0f, two_pi)
            });
            return;
        }
    }
}

void PowerupField::spawn_guaranteed(Vector2 origin) noexcept {
    constexpr f32 two_pi{ 2.0f * std::numbers::pi_v<f32> };
    const f32 angle{ Random::real(0.0f, two_pi) };
    const f32 speed{ Random::real(15.0f, 35.0f) };
    const PowerupType chosen{ static_cast<PowerupType>(Random::uint(0, static_cast<u32>(POWERUP_COUNT - 1))) };
    spawn_specific(chosen, origin, Vector2{ std::cosf(angle) * speed, std::sinf(angle) * speed });
}

void PowerupField::spawn_specific(PowerupType type, Vector2 origin, Vector2 vel) noexcept {
    const u64 idx{ static_cast<u64>(type) };
    if (idx >= POWERUP_COUNT) return;

    powerups.push_back(Powerup{
        .type = configs[idx].type,
        .sprite = assets::Sprite{ configs[idx].asset_name },
        .pos = origin,
        .vel = vel,
        .scale = 1.5f,
        .radius = 18.0f,
        .ttl = configs[idx].world_ttl,
        .max_ttl = configs[idx].world_ttl,
        .effect_duration = configs[idx].effect_duration,
        .pulse_phase = 0.0f
    });
}

void PowerupField::update(f32 dt, Spaceship& spaceship) noexcept {
    constexpr f32 two_pi{ 2.0f * std::numbers::pi_v<f32> };
    const Vector2 ship_pos{ spaceship.get_pos() };
    const f32 ship_radius{ spaceship.get_radius() };

    for (auto it{ powerups.begin() }; it != powerups.end();) {
        it->pos = Vector2Add(it->pos, Vector2Scale(it->vel, dt));
        it->ttl -= dt;
        it->pulse_phase = std::fmodf(it->pulse_phase + 5.0f * dt, two_pi);

        if (CheckCollisionCircles(ship_pos, ship_radius, it->pos, it->radius)) {
            if (it->type == PowerupType::IncreaseHealth) {
                spaceship.heal_percent(0.25f);
            } else if (it->effect_duration > 0.0f) {
                spaceship.activate_powerup(it->type, it->effect_duration);
            }
            collected_events.push_back(CollectedPowerup{
                .type = it->type,
                .pos = it->pos
            });
            it = powerups.erase(it);
        } else if (it->ttl <= 0.0f) {
            it = powerups.erase(it);
        } else {
            ++it;
        }
    }
}

void PowerupField::draw() const noexcept {
    for (const Powerup& p : powerups) {
        if (!p.sprite.is_valid()) continue;

        const f32 pulse{ 1.0f + 0.1f * std::sinf(p.pulse_phase) };
        const f32 fade_t{ std::fmaxf(0.0f, std::fminf(1.0f, p.ttl / 2.0f)) };
        const f32 alpha{ fade_t * fade_t * (3.0f - 2.0f * fade_t) };
        p.sprite.draw(p.pos, 0.0f, p.scale * pulse, Fade(WHITE, alpha));
    }
}

} // namespace asteroides
