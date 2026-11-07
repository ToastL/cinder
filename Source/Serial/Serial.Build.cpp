#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("Serial", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"Scene"});
    m.privateDependencies({"Platform"});
});
