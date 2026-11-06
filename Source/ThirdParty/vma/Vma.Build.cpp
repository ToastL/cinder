#include "CinderBuildTool/Rules.hpp"


CINDER_MODULE("vma", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.thirdParty().sources({"VmaImplementation.cpp"}).publicIncludes({"include"});
    m.publicDependencies({"volk"});
    m.publicDefines({"VMA_STATIC_VULKAN_FUNCTIONS=0", "VMA_DYNAMIC_VULKAN_FUNCTIONS=1"});
});
