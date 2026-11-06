#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("reflect", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"platform"});
});
