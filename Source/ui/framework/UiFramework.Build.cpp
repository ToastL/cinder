#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("ui/framework", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"ui/core"});
});
