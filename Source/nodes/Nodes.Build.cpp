#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("nodes", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"scene"});
});
