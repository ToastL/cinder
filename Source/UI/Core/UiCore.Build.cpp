#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("UI/Core", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"Text"});
});
