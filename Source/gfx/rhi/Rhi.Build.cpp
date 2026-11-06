#include "CinderBuildTool/Rules.hpp"


CINDER_MODULE("gfx/rhi", [](cbt::ModuleRules& m, const cbt::TargetInfo& t) {
    m.publicDependencies({"platform"});
    m.privateDependencies({"stb"});
    m.linkDependencies({t.backend == "mtl" ? "gfx/mtl" : "gfx/vk"});
    if (t.backend == "vk") m.publicDependencies({"volk", "vma"});
});
