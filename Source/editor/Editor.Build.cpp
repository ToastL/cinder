#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("editor", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.privateDependencies({"dev"});
    m.linkDependencies({"game"});
});
