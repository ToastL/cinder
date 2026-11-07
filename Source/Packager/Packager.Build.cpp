#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("Packager", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.privateDependencies({"Dev"});
});
