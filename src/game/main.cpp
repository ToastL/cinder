#include "core/Engine.hpp"
#include "core/GameConfig.hpp"
#include "core/GameLoop.hpp"
#include "platform/Assets.hpp"
#include "platform/Glfw.hpp"
#include "platform/Log.hpp"

#include <cstdio>
#include <cstring>
#include <exception>
#include <string>

using cinder::core::Engine;
using cinder::core::GameConfig;
using cinder::core::GameLoop;
using cinder::platform::Glfw;

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IOLBF, 0);

    int frameLimit = 0;
    std::string capture;
    std::string script;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--assets") == 0 && i + 1 < argc) {
            cinder::platform::setAssetRoot(argv[++i]);
        } else if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            frameLimit = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--capture") == 0 && i + 1 < argc) {
            capture = argv[++i];
        } else if (std::strcmp(argv[i], "--script") == 0 && i + 1 < argc) {
            script = argv[++i];
        }
    }

    try {
        GameConfig config = GameConfig::load(cinder::platform::assetPath("game.lua"));
        if (!script.empty()) config.script = script;

        Glfw::acquire();
        {
            Engine engine(config);
            GameLoop loop(config.fixedHz);

            int frames = 0;
            while (engine.running()) {
                loop.tick(engine);
                if (frameLimit > 0 && ++frames >= frameLimit) break;
            }

            if (!capture.empty()) engine.renderer().capture(capture);
        }
        Glfw::release();
        return 0;
    } catch (const std::exception& e) {
        cinder::platform::logError("[fatal] %s\n", e.what());
        return 1;
    }
}
