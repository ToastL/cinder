#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("UI/Docking", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"UI/Widgets"});
});
