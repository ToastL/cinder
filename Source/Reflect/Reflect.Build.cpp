#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("Reflect", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"glm"});
});
