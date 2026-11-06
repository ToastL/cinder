#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("components", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"scene"});
});
