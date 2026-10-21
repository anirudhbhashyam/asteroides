#include "world.hh"

namespace asteroides {

World::World(u64 star_count, u64 asteroid_count)
    : asteroids{ spaceship.get_pos(), asteroid_count } {
    stars.reserve(star_count);

    const f32 w{ static_cast<f32>(GetScreenWidth()) };
    const f32 h{ static_cast<f32>(GetScreenHeight()) };
    constexpr f32 two_pi{ 2.0f * std::numbers::pi_v<f32> };

    camera = Camera2D{
        .offset = Vector2{ w * 0.5f, h * 0.5f },
        .target = spaceship.get_pos(),
        .rotation = 0.0f,
        .zoom = 1.0f
    };

    constexpr std::array<Color, 4> palette{ {
        hex_to_color(0xf8fafcff), // crisp white
        hex_to_color(0xbae6fdff), // pale sky blue
        hex_to_color(0xfef08aff), // warm starlight yellow
        hex_to_color(0xc7d2feff)  // soft indigo-white
    } };

    for (u64 i{ 0 }; i < star_count; ++i) {
        const f32 depth{ Random::real(0.08f, 0.45f) };
        const f32 size{ (depth > 0.32f) ? 2.0f : 1.0f };
        const f32 base_alpha{ 0.25f + depth * 1.4f };

        stars.push_back(Star{
            .pos = Vector2{ Random::real(0.0f, w), Random::real(0.0f, h) },
            .depth = depth,
            .size = size,
            .base_alpha = std::fminf(0.95f, base_alpha),
            .twinkle_phase = Random::real(0.0f, two_pi),
            .twinkle_speed = Random::real(1.5f, 4.5f),
            .color = palette[Random::uint(0, static_cast<u32>(palette.size() - 1))]
        });
    }
}

void World::update(f32 dt) noexcept {
    spaceship.update(dt);

    // Smooth camera follow toward the spaceship's world position
    const f32 lerp_factor{ std::fminf(1.0f, camera_smooth_speed * dt) };
    camera.target = Vector2Lerp(camera.target, spaceship.get_pos(), lerp_factor);

    asteroids.update(dt, camera.target, spaceship.get_blaster(), powerups);
    powerups.update(dt, spaceship);

    const f32 w{ static_cast<f32>(GetScreenWidth()) };
    const f32 h{ static_cast<f32>(GetScreenHeight()) };
    const Vector2 ship_vel{ spaceship.get_vel() };
    const Vector2 base_vel{ Vector2Subtract(ambient_drift, ship_vel) };
    constexpr f32 two_pi{ 2.0f * std::numbers::pi_v<f32> };

    for (Star& star : stars) {
        star.pos.x += base_vel.x * star.depth * dt;
        star.pos.y += base_vel.y * star.depth * dt;

        if (star.pos.x < 0.0f) star.pos.x += w;
        else if (star.pos.x >= w) star.pos.x -= w;

        if (star.pos.y < 0.0f) star.pos.y += h;
        else if (star.pos.y >= h) star.pos.y -= h;

        star.twinkle_phase = std::fmodf(star.twinkle_phase + star.twinkle_speed * dt, two_pi);
    }
}

void World::draw() const noexcept {
    ClearBackground(BACKGROUND_COLOR);

    for (const Star& star : stars) {
        const f32 twinkle{ 0.8f + 0.2f * std::sinf(star.twinkle_phase) };
        const f32 alpha{ std::fmaxf(0.05f, std::fminf(1.0f, star.base_alpha * twinkle)) };
        DrawRectangleV(star.pos, Vector2{ star.size, star.size }, Fade(star.color, alpha));
    }

    BeginMode2D(camera);
        asteroids.draw();
        powerups.draw();
        spaceship.draw();
    EndMode2D();
}

} // namespace asteroides
