#include "core/Engine.hpp"
#include "core/GameConfig.hpp"
#include "core/GameLoop.hpp"
#include "dev/Console.hpp"
#include "dev/ImGuiLayer.hpp"
#include "dev/PlaySession.hpp"
#include "dev/Toolbar.hpp"
#include "platform/Assets.hpp"
#include "platform/Glfw.hpp"
#include "platform/Log.hpp"

#include <cstdio>
#include <cstring>
#include <exception>
#include <filesystem>
#include <string>

using cinder::core::Engine;
using cinder::core::GameConfig;
using cinder::core::GameLoop;
using cinder::dev::Console;
using cinder::dev::PlaySession;
using cinder::dev::Toolbar;
using cinder::platform::Glfw;

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IOLBF, 0);

    int frameLimit = 0;
    bool play = false;
    std::string capture;
    std::string scene;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--assets") == 0 && i + 1 < argc) {
            cinder::platform::setAssetRoot(argv[++i]);
        } else if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            frameLimit = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--capture") == 0 && i + 1 < argc) {
            capture = argv[++i];
        } else if (std::strcmp(argv[i], "--scene") == 0 && i + 1 < argc) {
            scene = argv[++i];
        } else if (std::strcmp(argv[i], "--play") == 0) {
            play = true;
        }
    }

    try {
        GameConfig config = GameConfig::load(cinder::platform::assetPath("game.lua"));
        if (!scene.empty()) config.scene = scene;
        const std::filesystem::path scenePath = cinder::platform::resolveAsset(config.scene);

        Glfw::acquire();
        {
            Engine engine(config, cinder::dev::overlayFactory());
            PlaySession session(engine);
            Console console(engine.script());
            Toolbar toolbar(session, engine, scenePath);
            engine.renderer().setOverlayDraw([&toolbar, &console] {
                toolbar.draw();
                console.draw();
            });

            if (std::filesystem::exists(scenePath)) {
                engine.openScene(scenePath);
                cinder::platform::logInfo("[editor] opened %s\n", scenePath.string().c_str());
            } else {
                cinder::platform::logInfo("[editor] new scene, Save writes %s\n",
                                          scenePath.string().c_str());
            }
            if (play) session.togglePlay();

            GameLoop loop(config.fixedHz);

            int frames = 0;
            while (!engine.window().shouldClose()) {
                session.tick(loop);
                if (frameLimit > 0 && ++frames >= frameLimit) break;
            }

            if (!capture.empty()) engine.renderer().capture(capture);
            engine.renderer().setOverlayDraw(nullptr);
        }
        Glfw::release();
        return 0;
    } catch (const std::exception& e) {
        cinder::platform::logError("[fatal] %s\n", e.what());
        return 1;
    }
}
