#include "CinderBuildTool/Rules.hpp"


CINDER_MODULE("tests", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.privateDependencies({"Dev", "doctest", "glfw"});
    m.linkDependencies({"Game"});
    m.exclude({"selftest"});
});
