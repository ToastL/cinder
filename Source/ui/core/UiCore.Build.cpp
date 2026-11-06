#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("ui/core", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"text"});
});
