#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("gfx/pass", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"gfx/asset"});
});
