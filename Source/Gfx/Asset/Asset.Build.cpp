#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("Gfx/Asset", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"Gfx/RHI", "Scene"});
});
