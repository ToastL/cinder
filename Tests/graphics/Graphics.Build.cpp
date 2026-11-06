#include "CinderBuildTool/Rules.hpp"


CINDER_MODULE("graphics_smoke", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.privateDependencies({"dev", "glfw"});
});
