#include "CinderBuildTool/Rules.hpp"


CINDER_MODULE("Gfx/RHI", [](cbt::ModuleRules& m, const cbt::TargetInfo& t) {
    m.publicDependencies({"Platform"});
    m.privateDependencies({"stb"});
    m.linkDependencies({t.backend == "mtl" ? "Gfx/MTL" : "Gfx/VK"});
    if (t.backend == "vk") m.publicDependencies({"volk", "vma"});
});
