#include "core/LaunchOptions.hpp"

#include <cstdlib>
#include <string_view>

namespace cinder::core {

LaunchOptions LaunchOptions::parse(int argc, const char* const* argv, LaunchTarget target) {
    LaunchOptions options;
    const bool editor = target == LaunchTarget::Editor;
    const bool gallery = target == LaunchTarget::Gallery;
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument = argv[i];
        const bool hasValue = i + 1 < argc;
        if (argument == "--frames" && hasValue) {
            options.frameLimit = std::atoi(argv[++i]);
        } else if (!gallery && argument == "--capture" && hasValue) {
            options.capture = argv[++i];
        } else if ((editor || gallery) && argument == "--capture-window" && hasValue) {
            options.windowCapture = argv[++i];
        } else if ((editor || gallery) && argument == "--input-script" && hasValue) {
            options.inputScript = argv[++i];
        } else if (!gallery && argument == "--scene" && hasValue) {
            options.scene = argv[++i];
        } else if (editor && argument == "--play") {
            options.play = true;
        } else if (gallery && argument == "--text-gamma" && hasValue) {
            options.textGamma = static_cast<float>(std::atof(argv[++i]));
        } else if (gallery && argument == "--lowdpi") {
            options.lowDpi = true;
        } else if (!gallery && (argument.empty() || argument.front() != '-') && options.project.empty()) {
            options.project = argument;
        }
    }
    return options;
}

}
