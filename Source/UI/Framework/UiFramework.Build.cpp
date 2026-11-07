#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("UI/Framework", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"UI/Core"});
});
