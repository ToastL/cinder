#include "core/LaunchOptions.hpp"

#include <doctest/doctest.h>

#include <initializer_list>

using cinder::core::LaunchOptions;
using cinder::core::LaunchTarget;

namespace {
LaunchOptions parse(std::initializer_list<const char*> args, LaunchTarget target = LaunchTarget::Editor) {
    return LaunchOptions::parse(static_cast<int>(args.size()), args.begin(), target);
}
}

TEST_CASE("launch options preserve project and capture arguments") {
    const auto options = parse({"editor", "my game/demo.cinder", "--scene", "Scenes/test.scene",
                               "--frames", "12", "--capture", "scene.png", "--capture-window",
                               "window.png", "--input-script", "steps.txt", "--play"});
    CHECK(options.project == "my game/demo.cinder");
    CHECK(options.scene == "Scenes/test.scene");
    CHECK(options.frameLimit == 12);
    CHECK(options.capture == "scene.png");
    CHECK(options.windowCapture == "window.png");
    CHECK(options.inputScript == "steps.txt");
    CHECK(options.play);
}

TEST_CASE("launch options retain permissive legacy parsing") {
    CHECK(parse({"editor", "first", "second"}).project == "first");
    CHECK(parse({"editor", "--unknown", "game"}).project == "game");
    CHECK(parse({"editor", "--frames"}).frameLimit == 0);
    CHECK(parse({"editor", "--frames", "invalid"}).frameLimit == 0);
    CHECK(parse({"editor", "--frames", "12suffix"}).frameLimit == 12);
    CHECK(parse({"editor", "--frames", "-4"}).frameLimit == -4);
    CHECK(parse({"editor", "--frames", "2", "--frames", "9"}).frameLimit == 9);
    CHECK(parse({"editor", "--scene", "--play"}).scene == "--play");
    CHECK_FALSE(parse({"editor", "--scene", "--play"}).play);
    for (const char* flag : {"--capture", "--capture-window", "--input-script", "--scene"}) {
        const auto options = parse({"editor", "game", flag});
        CHECK(options.project == "game");
        CHECK(options.capture.empty());
        CHECK(options.windowCapture.empty());
        CHECK(options.inputScript.empty());
        CHECK(options.scene.empty());
    }
}

TEST_CASE("player and gallery accept only their own launch options") {
    const auto player = parse({"player", "game", "--play", "--capture-window", "window.png",
                               "--input-script", "steps.txt", "--frames", "3", "--capture", "out.png"},
                              LaunchTarget::Player);
    CHECK(player.project == "game");
    CHECK(player.frameLimit == 3);
    CHECK(player.capture == "out.png");
    CHECK_FALSE(player.play);
    CHECK(player.windowCapture.empty());
    CHECK(player.inputScript.empty());
    CHECK(parse({"player"}, LaunchTarget::Player).project.empty());
    CHECK(parse({"player", "--capture-window", "game"}, LaunchTarget::Player).project == "game");

    const auto gallery = parse({"gallery", "--frames", "7", "--capture-window", "out.png",
                                "--input-script", "steps.txt", "--lowdpi", "--text-gamma", "1.5",
                                "--scene", "ignored", "--play"}, LaunchTarget::Gallery);
    CHECK(gallery.frameLimit == 7);
    CHECK(gallery.windowCapture == "out.png");
    CHECK(gallery.inputScript == "steps.txt");
    CHECK(gallery.lowDpi);
    REQUIRE(gallery.textGamma.has_value());
    CHECK(*gallery.textGamma == doctest::Approx(1.5f));
    CHECK(gallery.project.empty());
    CHECK(gallery.scene.empty());
    CHECK_FALSE(gallery.play);
    CHECK_FALSE(parse({"gallery", "--text-gamma"}, LaunchTarget::Gallery).textGamma.has_value());
}
