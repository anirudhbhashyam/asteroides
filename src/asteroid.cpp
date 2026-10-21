#include "asteroid.hh"

namespace asteroides {

std::array<Color, Asteroid::PIXEL_COUNT> Asteroid::generate_pixels() noexcept {
    constexpr f32 cx{ 16.5f };
    constexpr f32 cy{ 16.5f };
    constexpr f32 two_pi{ 2.0f * std::numbers::pi_v<f32> };
    constexpr f32 angle_step{ two_pi / static_cast<f32>(VERTEX_COUNT) };

    std::array<Vector2, VERTEX_COUNT> verts{ };
    const u64 notch_idx{ static_cast<u64>(Random::uint(0, static_cast<u32>(VERTEX_COUNT - 1))) };

    for (u64 i{ 0 }; i < VERTEX_COUNT; ++i) {
        const f32 base_angle{ static_cast<f32>(i) * angle_step };
        const f32 angle{ base_angle + Random::real(-angle_step * 0.25f, angle_step * 0.25f) };
        const f32 r{ (i == notch_idx) ? Random::real(3.8f, 5.2f) : Random::real(6.0f, 9.2f) };
        verts[i] = Vector2{
            cx + std::cosf(angle) * r,
            cy + std::sinf(angle) * r
        };
    }

    const u64 spur_idx{ (notch_idx + 1) % VERTEX_COUNT };
    const f32 spur_angle{ static_cast<f32>(spur_idx) * angle_step };
    const Vector2 spur_start{ verts[spur_idx] };
    const Vector2 spur_end{
        spur_start.x + std::cosf(spur_angle) * 2.2f,
        spur_start.y + std::sinf(spur_angle) * 2.2f
    };

    auto dist_sq_to_segment = [](Vector2 p, Vector2 a, Vector2 b) noexcept -> f32 {
        const f32 abx{ b.x - a.x };
        const f32 aby{ b.y - a.y };
        const f32 apx{ p.x - a.x };
        const f32 apy{ p.y - a.y };
        const f32 ab_len_sq{ abx * abx + aby * aby };
        f32 t{ ab_len_sq > 0.0001f ? (apx * abx + apy * aby) / ab_len_sq : 0.0f };
        t = std::fmaxf(0.0f, std::fminf(1.0f, t));
        const f32 dx{ p.x - (a.x + t * abx) };
        const f32 dy{ p.y - (a.y + t * aby) };
        return dx * dx + dy * dy;
    };

    std::array<bool, PIXEL_COUNT> inside_mask{ };
    std::array<Color, PIXEL_COUNT> pixels{ };
    pixels.fill(COLOR_EMPTY);

    // Pass 1: Rasterize thick black rocky contour (#000000ff)
    for (i32 y{ 0 }; y < SPRITE_SIZE; ++y) {
        for (i32 x{ 0 }; x < SPRITE_SIZE; ++x) {
            const Vector2 p{ static_cast<f32>(x) + 0.5f, static_cast<f32>(y) + 0.5f };

            bool inside{ false };
            f32 min_edge_dist_sq{ 10000.0f };

            for (u64 i{ 0 }, j{ VERTEX_COUNT - 1 }; i < VERTEX_COUNT; j = i++) {
                const Vector2 vi{ verts[i] };
                const Vector2 vj{ verts[j] };

                if (((vi.y > p.y) != (vj.y > p.y)) &&
                    (p.x < (vj.x - vi.x) * (p.y - vi.y) / (vj.y - vi.y) + vi.x)) {
                    inside = !inside;
                }

                const f32 d_sq{ dist_sq_to_segment(p, vj, vi) };
                min_edge_dist_sq = std::fminf(min_edge_dist_sq, d_sq);
            }

            const u64 idx{ static_cast<u64>(y * SPRITE_SIZE + x) };
            inside_mask[idx] = inside;

            const f32 spur_d_sq{ dist_sq_to_segment(p, spur_start, spur_end) };
            if ((inside && min_edge_dist_sq <= 2.15f * 2.15f) ||
                min_edge_dist_sq <= 0.75f * 0.75f ||
                spur_d_sq <= 0.85f * 0.85f) {
                pixels[idx] = COLOR_BODY;
            }
        }
    }

    // Pass 2: Add outer rocky grey rim/shadow (#4e4d50ff)
    for (i32 y{ 1 }; y < SPRITE_SIZE - 1; ++y) {
        for (i32 x{ 1 }; x < SPRITE_SIZE - 1; ++x) {
            const u64 idx{ static_cast<u64>(y * SPRITE_SIZE + x) };
            if (pixels[idx].a != 0 || inside_mask[idx]) {
                continue;
            }

            auto is_body = [&](i32 nx, i32 ny) noexcept -> bool {
                if (nx < 0 || nx >= SPRITE_SIZE || ny < 0 || ny >= SPRITE_SIZE) return false;
                const Color c{ pixels[static_cast<u64>(ny * SPRITE_SIZE + nx)] };
                return c.a != 0 && c.r == 0;
            };

            const bool left_or_bottom_rim{
                is_body(x + 1, y) ||
                is_body(x + 2, y) ||
                is_body(x + 1, y - 1) ||
                is_body(x, y - 1) ||
                is_body(x - 1, y - 1)
            };
            const bool top_or_right_accent{
                (is_body(x - 1, y) || is_body(x, y + 1)) && (Random::uint(0, 2) == 0)
            };

            if (left_or_bottom_rim || top_or_right_accent) {
                pixels[idx] = COLOR_RIM;
            }
        }
    }

    return pixels;
}

Asteroid Asteroid::spawn_from_edge(Vector2 view_center, i32 screen_w, i32 screen_h) noexcept {
    Asteroid asteroid{ };
    const auto pixels{ generate_pixels() };
    asteroid.sprite = assets::Sprite{ pixels.data(), SPRITE_SIZE, SPRITE_SIZE };
    asteroid.scale = Random::real(1.75f, 2.75f);
    asteroid.radius = 9.0f * asteroid.scale;
    asteroid.rotation = Random::real(0.0f, 360.0f);
    asteroid.angular_vel = Random::real(-90.0f, 90.0f);

    const f32 w{ static_cast<f32>(screen_w) };
    const f32 h{ static_cast<f32>(screen_h) };
    const f32 left{ view_center.x - w * 0.5f };
    const f32 right{ view_center.x + w * 0.5f };
    const f32 top{ view_center.y - h * 0.5f };
    const f32 bottom{ view_center.y + h * 0.5f };
    const f32 pad{ asteroid.radius + 24.0f };

    switch (Random::uint(0, 3)) {
        case 0:
            asteroid.pos = Vector2{ Random::real(left, right), top - pad };
            break;
        case 1:
            asteroid.pos = Vector2{ Random::real(left, right), bottom + pad };
            break;
        case 2:
            asteroid.pos = Vector2{ left - pad, Random::real(top, bottom) };
            break;
        default:
            asteroid.pos = Vector2{ right + pad, Random::real(top, bottom) };
            break;
    }

    const Vector2 target{
        Random::real(left + w * 0.15f, left + w * 0.85f),
        Random::real(top + h * 0.15f, top + h * 0.85f)
    };
    const Vector2 dir{ Vector2Normalize(Vector2Subtract(target, asteroid.pos)) };
    const f32 speed{ Random::real(70.0f, 200.0f) };
    asteroid.vel = Vector2Scale(dir, speed);

    return asteroid;
}

Asteroid Asteroid::spawn_fragment(Vector2 origin, f32 parent_scale, f32 angle_rad) noexcept {
    Asteroid frag{ };
    const auto pixels{ generate_pixels() };
    frag.sprite = assets::Sprite{ pixels.data(), SPRITE_SIZE, SPRITE_SIZE };
    frag.scale = parent_scale * Random::real(0.4f, 0.6f);
    frag.radius = 9.0f * frag.scale;
    frag.rotation = Random::real(0.0f, 360.0f);
    frag.angular_vel = Random::real(-240.0f, 240.0f);
    frag.ttl = Random::real(0.9f, 1.6f);
    frag.max_ttl = frag.ttl;
    frag.is_fragment = true;

    const Vector2 dir{ std::cosf(angle_rad), std::sinf(angle_rad) };
    frag.pos = Vector2Add(origin, Vector2Scale(dir, 6.0f));
    frag.vel = Vector2Scale(dir, Random::real(90.0f, 220.0f));
    return frag;
}

void Asteroid::update(f32 dt) noexcept {
    pos = Vector2Add(pos, Vector2Scale(vel, dt));
    rotation = std::remainderf(rotation + angular_vel * dt, 360.0f);
    if (is_fragment) {
        ttl -= dt;
        vel = Vector2Scale(vel, std::fmaxf(0.0f, 1.0f - 1.5f * dt));
    }
}

void Asteroid::draw() const noexcept {
    if (!sprite.is_valid()) return;

    if (is_fragment) {
        const f32 t{ std::fmaxf(0.0f, std::fminf(1.0f, ttl / max_ttl)) };
        const f32 smooth_alpha{ t * t * (3.0f - 2.0f * t) };
        const f32 render_scale{ scale * (0.65f + 0.35f * smooth_alpha) };
        sprite.draw(pos, rotation, render_scale, Fade(WHITE, smooth_alpha));
    } else {
        sprite.draw(pos, rotation, scale);
    }
}

bool Asteroid::is_offscreen(Vector2 view_center, i32 screen_w, i32 screen_h, f32 margin) const noexcept {
    const f32 half_w{ static_cast<f32>(screen_w) * 0.5f + radius + margin };
    const f32 half_h{ static_cast<f32>(screen_h) * 0.5f + radius + margin };
    return (std::fabsf(pos.x - view_center.x) > half_w) ||
           (std::fabsf(pos.y - view_center.y) > half_h);
}

bool Asteroid::is_expired() const noexcept {
    return is_fragment && (ttl <= 0.0f);
}

bool Asteroid::collides_with(Vector2 point, f32 point_radius) const noexcept {
    return CheckCollisionCircles(pos, radius, point, point_radius);
}

bool Asteroid::collides_with(const Asteroid& other) const noexcept {
    return CheckCollisionCircles(pos, radius, other.pos, other.radius);
}

void Asteroid::resolve_physics_collision(Asteroid& a, Asteroid& b) noexcept {
    Vector2 delta{ Vector2Subtract(b.pos, a.pos) };
    f32 dist{ Vector2Length(delta) };
    const f32 min_dist{ a.radius + b.radius };

    if (dist >= min_dist) return;

    const Vector2 normal{
        dist > 0.0001f ? Vector2Scale(delta, 1.0f / dist) : Vector2{ 1.0f, 0.0f }
    };

    // Mass proportional to area (radius^2)
    const f32 mass_a{ a.radius * a.radius };
    const f32 mass_b{ b.radius * b.radius };
    const f32 inv_mass_a{ 1.0f / mass_a };
    const f32 inv_mass_b{ 1.0f / mass_b };
    const f32 inv_mass_sum{ inv_mass_a + inv_mass_b };

    // Positional separation so colliding asteroids don't overlap/stick
    const f32 penetration{ min_dist - dist };
    const Vector2 correction{ Vector2Scale(normal, penetration / inv_mass_sum) };
    a.pos = Vector2Subtract(a.pos, Vector2Scale(correction, inv_mass_a));
    b.pos = Vector2Add(b.pos, Vector2Scale(correction, inv_mass_b));

    // 2D impulse resolution along collision normal
    const Vector2 rel_vel{ Vector2Subtract(b.vel, a.vel) };
    const f32 vel_along_normal{ Vector2DotProduct(rel_vel, normal) };
    if (vel_along_normal > 0.0f) return;

    constexpr f32 restitution{ 0.9f };
    const f32 j{ -(1.0f + restitution) * vel_along_normal / inv_mass_sum };
    const Vector2 impulse{ Vector2Scale(normal, j) };

    a.vel = Vector2Subtract(a.vel, Vector2Scale(impulse, inv_mass_a));
    b.vel = Vector2Add(b.vel, Vector2Scale(impulse, inv_mass_b));

    // Impart angular spin from tangential relative velocity
    const Vector2 tangent{ -normal.y, normal.x };
    const f32 tangent_speed{ Vector2DotProduct(rel_vel, tangent) };
    a.angular_vel = std::fmaxf(-240.0f, std::fminf(240.0f, a.angular_vel - tangent_speed * 0.4f));
    b.angular_vel = std::fmaxf(-240.0f, std::fminf(240.0f, b.angular_vel + tangent_speed * 0.4f));
}

void AsteroidField::_spawn_explosion(Vector2 origin, f32 parent_scale) noexcept {
    constexpr f32 two_pi{ 2.0f * std::numbers::pi_v<f32> };
    const u32 frag_count{ Random::uint(3, 5) };
    const f32 base_offset{ Random::real(0.0f, two_pi) };

    for (u32 i{ 0 }; i < frag_count; ++i) {
        const f32 sector{ base_offset + (static_cast<f32>(i) / static_cast<f32>(frag_count)) * two_pi };
        const f32 angle{ sector + Random::real(-0.35f, 0.35f) };
        fragments.push_back(Asteroid::spawn_fragment(origin, parent_scale, angle));
    }
}

AsteroidField::AsteroidField(Vector2 initial_center, u64 count) {
    asteroids.reserve(count);
    const i32 w{ GetScreenWidth() };
    const i32 h{ GetScreenHeight() };
    for (u64 i{ 0 }; i < count; ++i) {
        asteroids.push_back(Asteroid::spawn_from_edge(initial_center, w, h));
    }
}

void AsteroidField::update(f32 dt, Vector2 view_center, Blaster& blaster, PowerupField& powerups) noexcept {
    const i32 w{ GetScreenWidth() };
    const i32 h{ GetScreenHeight() };
    constexpr f32 bolt_radius{ 4.0f };

    auto& bolts{ blaster.get_bolts() };

    for (Asteroid& asteroid : asteroids) {
        asteroid.update(dt);

        bool hit{ false };
        for (auto it{ bolts.begin() }; it != bolts.end(); ++it) {
            if (asteroid.collides_with(it->pos, bolt_radius)) {
                bolts.erase(it);
                hit = true;
                break;
            }
        }

        if (hit) {
            _spawn_explosion(asteroid.get_pos(), asteroid.get_scale());
            powerups.try_spawn(asteroid.get_pos());
            asteroid = Asteroid::spawn_from_edge(view_center, w, h);
        } else if (asteroid.is_offscreen(view_center, w, h)) {
            asteroid = Asteroid::spawn_from_edge(view_center, w, h);
        }
    }

    // Asteroid-to-asteroid collisions: randomly either fragment or bounce with physics
    const u64 count{ static_cast<u64>(asteroids.size()) };
    for (u64 i{ 0 }; i < count; ++i) {
        if (asteroids[i].is_offscreen(view_center, w, h, 0.0f)) continue;

        for (u64 j{ i + 1 }; j < count; ++j) {
            if (asteroids[j].is_offscreen(view_center, w, h, 0.0f)) continue;

            if (asteroids[i].collides_with(asteroids[j])) {
                // 35% chance to shatter on impact, 65% chance to bounce with 2D impulse physics
                if (Random::real() < 0.35f) {
                    _spawn_explosion(asteroids[i].get_pos(), asteroids[i].get_scale());
                    _spawn_explosion(asteroids[j].get_pos(), asteroids[j].get_scale());
                    asteroids[i] = Asteroid::spawn_from_edge(view_center, w, h);
                    asteroids[j] = Asteroid::spawn_from_edge(view_center, w, h);
                    break;
                } else {
                    Asteroid::resolve_physics_collision(asteroids[i], asteroids[j]);
                }
            }
        }
    }

    for (Asteroid& frag : fragments) {
        frag.update(dt);
    }

    std::erase_if(fragments, [&](const Asteroid& frag) noexcept {
        return frag.is_expired() || frag.is_offscreen(view_center, w, h);
    });
}

void AsteroidField::draw() const noexcept {
    for (const Asteroid& asteroid : asteroids) {
        asteroid.draw();
    }
    for (const Asteroid& frag : fragments) {
        frag.draw();
    }
}

} // namespace asteroides
