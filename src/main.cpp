#include "engine.hh"
#include "game.hh"

i32 main() {
    asteroides::Engine engine{ };

    asteroides::Game game{ };

    while (engine.is_running()) {
        engine.begin_frame();
        game.update(engine);
        game.draw(engine);
        engine.end_frame();
    }

    return 0;
}
