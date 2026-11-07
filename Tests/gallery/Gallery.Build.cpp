#include "CinderBuildTool/Rules.hpp"


CINDER_MODULE("gallery", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.privateDependencies({"Core", "glfw"});
});
