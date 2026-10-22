#include "blaster.hh"

namespace asteroides {

Blaster::Blaster(std::string_view asset_name, f32 bolt_speed, f32 fire_cooldown)
    : sprite{ asset_name }
    , speed{ bolt_speed }
    , cooldown{ fire_cooldown } {}

void Blaster::clear() noexcept {
    bolts.clear();
    cooldown_timer = 0.0f;
}

void Blaster::fire(Vector2 origin, Vector2 ship_vel, f32 heading_deg, bool double_shot) noexcept {
    if (cooldown_timer > 0.0f) return;

    cooldown_timer = cooldown;
    const f32 rad{ heading_deg * DEG2RAD };
    const Vector2 dir{ std::sinf(rad), -std::cosf(rad) };
    const Vector2 spawn_center{ Vector2Add(origin, Vector2Scale(dir, muzzle_offset)) };
    const f32 forward_ship_speed{ std::fmaxf(0.0f, Vector2DotProduct(ship_vel, dir)) };
    const Vector2 bolt_vel{ Vector2Scale(dir, speed + forward_ship_speed) };
    const f32 bolt_rot{ heading_deg - 90.0f };

    if (double_shot) {
        const Vector2 right{ std::cosf(rad), std::sinf(rad) };
        const Vector2 offset{ Vector2Scale(right, double_spread_offset) };

        bolts.push_back(Bolt{
            .pos = Vector2Subtract(spawn_center, offset),
            .vel = bolt_vel,
            .rotation = bolt_rot
        });
        bolts.push_back(Bolt{
            .pos = Vector2Add(spawn_center, offset),
            .vel = bolt_vel,
            .rotation = bolt_rot
        });
    } else {
        bolts.push_back(Bolt{
            .pos = spawn_center,
            .vel = bolt_vel,
            .rotation = bolt_rot
        });
    }
}

void Blaster::update(f32 dt, Vector2 view_center) noexcept {
    if (cooldown_timer > 0.0f) {
        cooldown_timer = std::fmaxf(0.0f, cooldown_timer - dt);
    }

    const f32 half_w{ static_cast<f32>(GetScreenWidth()) * 0.5f };
    const f32 half_h{ static_cast<f32>(GetScreenHeight()) * 0.5f };
    constexpr f32 margin{ 96.0f };

    for (Bolt& bolt : bolts) {
        bolt.pos = Vector2Add(bolt.pos, Vector2Scale(bolt.vel, dt));
    }

    std::erase_if(bolts, [&](const Bolt& bolt) noexcept {
        return (std::fabsf(bolt.pos.x - view_center.x) > half_w + margin) ||
               (std::fabsf(bolt.pos.y - view_center.y) > half_h + margin);
    });
}

void Blaster::draw() const noexcept {
    if (!sprite.is_valid()) return;

    for (const Bolt& bolt : bolts) {
        sprite.draw(bolt.pos, bolt.rotation, scale);
    }
}

} // namespace asteroides
