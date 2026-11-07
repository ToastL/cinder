#include "Core/LaunchOptions.hpp"

#include "Platform/Log.hpp"

#include <charconv>
#include <string_view>

namespace cinder::Core {
namespace {

template <class T>
T numberArgument(std::string_view flag, std::string_view text, T fallback) {
    T value{};
    const char* end = text.data() + text.size();
    auto [at, error] = std::from_chars(text.data(), end, value);
    if (error == std::errc{} && at == end && !text.empty()) return value;
    cinder::Platform::logError("%.*s expects a number, not '%.*s'\n", static_cast<int>(flag.size()), flag.data(),
                               static_cast<int>(text.size()), text.data());
    return fallback;
}

}

LaunchOptions LaunchOptions::parse(int argc, const char* const* argv, LaunchTarget target) {
    LaunchOptions options;
    const bool editor = target == LaunchTarget::Editor;
    const bool gallery = target == LaunchTarget::Gallery;
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument = argv[i];
        const bool hasValue = i + 1 < argc;
        if (argument == "--frames" && hasValue) {
            options.frameLimit = numberArgument(argument, argv[++i], options.frameLimit);
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
            if (const float gamma = numberArgument(argument, argv[++i], -1.0f); gamma >= 0.0f) options.textGamma = gamma;
        } else if (gallery && argument == "--lowdpi") {
            options.lowDpi = true;
        } else if (!gallery && (argument.empty() || argument.front() != '-') && options.project.empty()) {
            options.project = argument;
        }
    }
    return options;
}

}
