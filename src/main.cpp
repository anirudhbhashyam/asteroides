#include "engine.hh"
#include "game.hh"
#include "raylib.h"

i32 main() {
    constexpr i32 window_width{ 800 };
    constexpr i32 window_height{ 600 };
    constexpr i32 fps{ 60 };
    asteroides::Random::init();
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(window_width, window_height, "Asteroids on steroids");
    SetTargetFPS(fps);

    {
        asteroides::Game game{ };

        while (!WindowShouldClose()) {
            const f32 dt{ GetFrameTime() };
            game.update(dt);
            BeginDrawing();
                game.draw();
            EndDrawing();
        }
    }

    CloseWindow();
    return 0;
}
