#pragma once

#include <cstdint>
#include <limits>
#include <random>

#include "raylib.h"

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using f32 = float;
using f64 = double;

namespace asteroides {

consteval Color hex_to_color(u32 hex) {
    // RRGGBBAA
    return {
        .r = static_cast<u8>((hex >> 8 * 3) & 0xFF),
        .g = static_cast<u8>((hex >> 8 * 2) & 0xFF),
        .b = static_cast<u8>((hex >> 8 * 1) & 0xFF),
        .a = static_cast<u8>((hex >> 8 * 0) & 0xFF)
    };
}

class Random {
    public:
        static void init() {
            s_random_engine.seed(std::random_device{ }());
        }

        static u32 uint() {
            return s_distribution(s_random_engine);
        }

        static u32 uint(u32 min, u32 max) {
            return min + (s_distribution(s_random_engine) % (max - min + 1));
        }

        static f32 real() {
            return static_cast<f32>(s_distribution(s_random_engine)) /
                   static_cast<f32>(std::numeric_limits<u32>::max());
        }

        static f32 real(f32 min, f32 max) {
            return real() * (max - min) + min;
        }

        static Vector2 vec2() {
            return Vector2{ real(), real() };
        }

        static Vector2 vec2(f32 min, f32 max) {
            return Vector2{ real(min, max), real(min, max) };
        }

    private:
        static std::mt19937 s_random_engine;
        static std::uniform_int_distribution<std::mt19937::result_type> s_distribution;
};

} // namespace asteroides
