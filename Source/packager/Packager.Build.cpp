#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("packager", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.privateDependencies({"dev"});
});
