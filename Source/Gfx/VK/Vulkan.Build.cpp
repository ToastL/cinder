#include "CinderBuildTool/Rules.hpp"


CINDER_MODULE("Gfx/VK", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"Gfx/RHI", "volk", "vma"});
    m.privateDependencies({"glfw"});
});
