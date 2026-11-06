#include "CinderBuildTool/Rules.hpp"


CINDER_MODULE("text", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"platform"});
    m.privateDependencies({"freetype", "harfbuzz", "stb"});
});
