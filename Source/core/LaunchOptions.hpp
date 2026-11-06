#pragma once

#include <optional>
#include <string>

namespace cinder::core {

enum class LaunchTarget { Player, Editor, Gallery };

struct LaunchOptions {
    int frameLimit = 0;
    bool play = false;
    bool lowDpi = false;
    std::optional<float> textGamma;
    std::string capture;
    std::string windowCapture;
    std::string inputScript;
    std::string scene;
    std::string project;

    static LaunchOptions parse(int argc, const char* const* argv, LaunchTarget target);
};

}
