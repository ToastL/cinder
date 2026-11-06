#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("dev", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"core"});
});
