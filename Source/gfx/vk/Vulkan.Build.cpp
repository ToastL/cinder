#include "CinderBuildTool/Rules.hpp"


CINDER_MODULE("gfx/vk", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"gfx/rhi", "volk", "vma"});
    m.privateDependencies({"glfw", "stb"});
});
