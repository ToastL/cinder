#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("game", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"core"});
});
