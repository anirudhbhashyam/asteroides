#pragma once

#include <array>
#include <cmath>
#include <cstdio>
#include <string_view>

#include "engine.hh"
#include "game.hh"
#include "raylib.h"
#include "raymath.h"

namespace asteroides::tests {

struct AssertionItem {
    std::string_view label{ "" };
    bool passed{ false };
};

struct ScenarioStatus {
    std::string_view title{ "" };
    std::string_view subtitle{ "" };
    f32 duration{ 3.0f };
    f32 elapsed{ 0.0f };
    std::array<AssertionItem, 4> assertions{ };
};

struct VisualTestRunner {
    public:
        static constexpr u64 SCENARIO_COUNT{ 9 };

        VisualTestRunner(Engine& engine_ref, Game& game_ref) noexcept;

        void update() noexcept;
        void draw() const noexcept;

        [[nodiscard]] bool is_finished() const noexcept { return finished && summary_timer <= 0.0f; }
        [[nodiscard]] bool all_passed() const noexcept;
        [[nodiscard]] i32 exit_code() const noexcept { return all_passed() ? 0 : 1; }

    private:
        void _start_scenario(u64 index) noexcept;
        void _update_scenario(f32 dt) noexcept;
        void _pass_assertion(u64 assertion_idx) noexcept;
        void _advance_scenario() noexcept;
        void _draw_banner() const noexcept;
        void _draw_summary() const noexcept;

        Engine& engine;
        Game& game;
        std::array<ScenarioStatus, SCENARIO_COUNT> scenarios{ };
        u64 current_idx{ 0 };
        u32 step_flags{ 0 };
        Vector2 saved_ship_pos{ Vector2Zero() };
        Vector2 saved_asteroid_pos{ Vector2Zero() };
        bool finished{ false };
        f32 summary_timer{ 1.8f };
};

} // namespace asteroides::tests
