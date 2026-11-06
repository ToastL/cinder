#include "CinderBuildTool/Rules.hpp"


CINDER_MODULE("glfw", [](cbt::ModuleRules& m, const cbt::TargetInfo& t) {
    m.thirdParty().publicIncludes({"include"});
    if (t.platform == "macOS") {
        m.sources({"src/context.c", "src/init.c", "src/input.c", "src/monitor.c", "src/platform.c", "src/vulkan.c",
                   "src/window.c", "src/egl_context.c", "src/osmesa_context.c", "src/null_init.c", "src/null_monitor.c",
                   "src/null_window.c", "src/null_joystick.c", "src/cocoa_time.c", "src/posix_module.c",
                   "src/posix_thread.c", "src/cocoa_init.m", "src/cocoa_joystick.m", "src/cocoa_monitor.m",
                   "src/cocoa_window.m", "src/nsgl_context.m"});
        m.privateDefines({"_GLFW_COCOA"});
        m.frameworks({"Cocoa", "IOKit", "CoreFoundation"});
    } else {
        m.sources({"src/context.c", "src/init.c", "src/input.c", "src/monitor.c", "src/platform.c", "src/vulkan.c",
                   "src/window.c", "src/egl_context.c", "src/osmesa_context.c", "src/null_init.c", "src/null_monitor.c",
                   "src/null_window.c", "src/null_joystick.c", "src/posix_time.c", "src/posix_module.c",
                   "src/posix_thread.c", "src/x11_init.c", "src/x11_monitor.c", "src/x11_window.c", "src/xkb_unicode.c",
                   "src/glx_context.c", "src/linux_joystick.c", "src/posix_poll.c"});
        m.privateDefines({"_GLFW_X11", "_DEFAULT_SOURCE"});
        m.systemLibraries({"m", "dl"});
    }
});
