#include "world.hh"

namespace asteroides {

World::World(Vector2 initial_target, u64 star_count) {
    stars.reserve(star_count);

    const f32 w{ static_cast<f32>(GetScreenWidth()) };
    const f32 h{ static_cast<f32>(GetScreenHeight()) };

    camera = Camera2D{
        .offset = Vector2{ w * 0.5f, h * 0.5f },
        .target = initial_target,
        .rotation = 0.0f,
        .zoom = 1.0f
    };

    constexpr std::array<Color, 4> palette{ {
        hex_to_color(0xf8fafcff),
        hex_to_color(0xbae6fdff),
        hex_to_color(0xfef08aff),
        hex_to_color(0xc7d2feff)
    } };

    for (u64 i{ 0 }; i < star_count; ++i) {
        const f32 depth{ Random::real(0.08f, 0.45f) };
        const f32 size{ (depth > 0.32f) ? 2.0f : 1.0f };
        const f32 base_alpha{ 0.25f + depth * 1.4f };

        stars.emplace_back(
            Vector2{ Random::real(0.0f, w), Random::real(0.0f, h) },
            depth,
            size,
            std::fminf(0.95f, base_alpha),
            Random::real(0.0f, TWO_PI),
            Random::real(1.5f, 4.5f),
            palette[Random::uint(0, static_cast<u32>(palette.size() - 1))]
        );
    }
}

void World::reset_camera(Vector2 target_pos) noexcept {
    camera.target = target_pos;
    locked_camera_target.reset();
    trauma = 0.0f;
}

void World::add_trauma(f32 amount) noexcept {
    trauma = std::clamp(trauma + amount, 0.0f, 1.0f);
}

void World::update(f32 dt, Vector2 target_pos, Vector2 target_vel) noexcept {
    trauma = std::fmaxf(0.0f, trauma - 1.85f * dt);
    if (locked_camera_target.has_value()) {
        camera.target = *locked_camera_target;
    } else {
        const f32 lerp_factor{ std::fminf(1.0f, camera_smooth_speed * dt) };
        camera.target = Vector2Lerp(camera.target, target_pos, lerp_factor);
    }

    const f32 w{ static_cast<f32>(GetScreenWidth()) };
    const f32 h{ static_cast<f32>(GetScreenHeight()) };
    const Vector2 base_vel{ Vector2Subtract(ambient_drift, target_vel) };

    for (Star& star : stars) {
        star.pos.x += base_vel.x * star.depth * dt;
        star.pos.y += base_vel.y * star.depth * dt;

        if (star.pos.x < 0.0f) star.pos.x += w;
        else if (star.pos.x >= w) star.pos.x -= w;

        if (star.pos.y < 0.0f) star.pos.y += h;
        else if (star.pos.y >= h) star.pos.y -= h;

        star.twinkle_phase = std::fmodf(star.twinkle_phase + star.twinkle_speed * dt, TWO_PI);
    }
}

void World::draw_background() const noexcept {
    ClearBackground(BACKGROUND_COLOR);

    for (const Star& star : stars) {
        const f32 twinkle{ 0.8f + 0.2f * std::sinf(star.twinkle_phase) };
        const f32 alpha{ std::fmaxf(0.05f, std::fminf(1.0f, star.base_alpha * twinkle)) };
        DrawRectangleV(star.pos, Vector2{ star.size, star.size }, Fade(star.color, alpha));
    }
}

void World::begin_camera() const noexcept {
    Camera2D shaken{ camera };
    if (trauma > 0.001f) {
        const f32 shake{ trauma * trauma };
        constexpr f32 max_offset{ 14.0f };
        constexpr f32 max_angle{ 2.2f };
        shaken.offset.x += Random::real(-1.0f, 1.0f) * max_offset * shake;
        shaken.offset.y += Random::real(-1.0f, 1.0f) * max_offset * shake;
        shaken.rotation = Random::real(-1.0f, 1.0f) * max_angle * shake;
    }
    BeginMode2D(shaken);
}

void World::end_camera() const noexcept {
    EndMode2D();
}

} // namespace asteroides
