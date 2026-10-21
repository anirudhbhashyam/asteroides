#include "spaceship.hh"

namespace asteroides {

void Spaceship::draw() const noexcept {
    blaster.draw();
    if (sprite.is_valid()) {
        sprite.draw(pos, FRAME_UP, rotation, scale);
    } else {
        DrawCircleV(pos, radius, color);
    }
}

void Spaceship::update(const f32 dt) noexcept {
    Vector2 directional_acc{ Vector2Zero() };
    f32 k_drag{ 0.01f };
    if (IsKeyDown(KEY_W)) directional_acc.y -= acc.y;
    if (IsKeyDown(KEY_S)) directional_acc.y += acc.y;
    if (IsKeyDown(KEY_A)) directional_acc.x -= acc.x;
    if (IsKeyDown(KEY_D)) directional_acc.x += acc.x;
    bool has_input{ directional_acc.x != 0.0f || directional_acc.y != 0.0f };
    if (has_input) {
        directional_acc = Vector2Multiply(Vector2Normalize(directional_acc), acc);
        const f32 target_rotation{ std::atan2f(directional_acc.x, -directional_acc.y) * RAD2DEG };
        const f32 angle_diff{ std::remainderf(target_rotation - rotation, 360.0f) };
        rotation = std::remainderf(rotation + angle_diff * std::fminf(1.0f, turn_speed * dt), 360.0f);
    }
    Vector2 drag_acc{ Vector2Zero() };
    f32 speed{ Vector2Length(vel) };
    if (speed > 0) {
        f32 max_braking_acc{ speed / dt };
        f32 braking{ has_input ? 0.0f : 600.0f };
        f32 drag_magnitude{ std::fminf(0.5f * k_drag * speed * speed + braking, max_braking_acc) };
        drag_acc = Vector2Scale(Vector2Normalize(vel), -drag_magnitude);
    }
    Vector2 net_acc{ Vector2Add(directional_acc, drag_acc) };
    Vector2 s{
        Vector2Add(
            Vector2Scale(vel, dt),
            Vector2Scale(net_acc, 0.5f * dt * dt)
        )
    };
    pos = Vector2Add(pos, s);
    vel = Vector2Add(vel, Vector2Scale(net_acc, dt));

    // TODO: Quadratic decay doesn't actually stop, so force.
    if (!has_input && Vector2LengthSqr(vel) < 75.0f) vel = Vector2{ 0.0f, 0.0f };

    if (IsKeyDown(KEY_SPACE)) {
        blaster.fire(pos, vel, rotation);
    }
    blaster.update(dt, pos);
}

} // namespace asteroides
