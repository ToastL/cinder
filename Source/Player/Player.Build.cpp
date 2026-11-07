#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("Player", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.privateDependencies({"Core"});
    m.linkDependencies({"Game"});
});
