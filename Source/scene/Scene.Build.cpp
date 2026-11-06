#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("scene", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"reflect"});
});
