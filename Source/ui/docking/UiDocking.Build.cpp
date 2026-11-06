#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("ui/docking", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"ui/widgets"});
});
