#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("gfx/asset", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"gfx/rhi", "scene"});
});
