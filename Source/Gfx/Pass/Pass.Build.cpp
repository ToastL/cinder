#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("Gfx/Pass", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"Gfx/Asset"});
});
