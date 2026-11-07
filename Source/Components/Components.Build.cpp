#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("Components", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"Scene"});
});
