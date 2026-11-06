#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("vulkan-headers", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.thirdParty().headerOnly().publicIncludes({"include"});
});
