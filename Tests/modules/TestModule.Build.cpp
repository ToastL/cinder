#include "CinderBuildTool/Rules.hpp"


CINDER_MODULE("test_module", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"Game"});
});
