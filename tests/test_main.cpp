#include "engine.hh"
#include "game.hh"
#include "test_runner.hh"

i32 main() {
    asteroides::Engine engine{ asteroides::EngineConfig{
        .window_width = 800,
        .window_height = 600,
        .target_fps = 60,
        .title = "Asteroides - Visual Feature Test Suite",
        .rng_seed = 42,
        .start_with_debug = true
    } };

    i32 result{ 0 };
    {
        asteroides::Game game{ };
        asteroides::tests::VisualTestRunner runner{ engine, game };

        while (engine.is_running() && !runner.is_finished()) {
            runner.update();
            engine.begin_frame();
            game.update(engine);
            game.draw(engine);
            runner.draw();
            engine.end_frame();
        }

        result = runner.exit_code();
    }

    return result;
}
