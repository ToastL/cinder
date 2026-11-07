#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("Editor", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.privateDependencies({"Dev"});
    m.linkDependencies({"Game"});
});
