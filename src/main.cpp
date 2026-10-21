#include "engine.hh"
#include "raylib.h"
#include "world.hh"

i32 main() {
    constexpr i32 window_width{ 800 };
    constexpr i32 window_height{ 600 };
    constexpr i32 fps{ 60 };
    asteroides::Random::init();
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(window_width, window_height, "Asteroids on steroids");
    SetTargetFPS(fps);

    {
        asteroides::World world{ };

        while (!WindowShouldClose()) {
            const f32 dt{ GetFrameTime() };
            world.update(dt);
            BeginDrawing();
                world.draw();
            EndDrawing();
        }
    }

    CloseWindow();
    return 0;
}
