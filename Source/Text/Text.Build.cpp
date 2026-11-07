#include "CinderBuildTool/Rules.hpp"


CINDER_MODULE("Text", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"Platform"});
    m.privateDependencies({"freetype", "harfbuzz", "stb"});
});
