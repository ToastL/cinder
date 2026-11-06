#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("gfx", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"gfx/pass", "ui/core"});
});
