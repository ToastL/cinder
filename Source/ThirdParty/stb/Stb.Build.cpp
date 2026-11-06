#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("stb", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.thirdParty().headerOnly().publicIncludes({"."});
});
