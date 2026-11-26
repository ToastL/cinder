#include "CinderBuildTool/Rules.hpp"


CINDER_MODULE("Gfx/VK", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"Gfx/RHI", "vulkan-headers", "vma"});
    m.privateDependencies({"glfw"});
});
