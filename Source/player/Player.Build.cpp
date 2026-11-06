#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("player", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.privateDependencies({"core"});
    m.linkDependencies({"game"});
});
