#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("Nodes", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"Scene"});
});
