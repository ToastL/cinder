#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("serial", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"scene"});
});
