#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("UI/Widgets", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"UI/Framework"});
});
