#include "CinderBuildTool/Rules.hpp"


CINDER_MODULE("tests", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.privateDependencies({"dev", "doctest", "glfw"});
    m.linkDependencies({"game"});
    m.exclude({"selftest"});
});
