#include "CinderBuildTool/Rules.hpp"


CINDER_MODULE("volk", [](cbt::ModuleRules& m, const cbt::TargetInfo& t) {
    m.thirdParty().sources({"volk.c"}).publicIncludes({"."});
    m.publicDependencies({"vulkan-headers"});
    m.publicDefines({"VK_NO_PROTOTYPES"});
    if (t.platform == "macOS") m.publicDefines({"VK_USE_PLATFORM_METAL_EXT"});
});
