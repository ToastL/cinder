#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("Gfx", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"Gfx/Pass", "UI/Core"});
});
