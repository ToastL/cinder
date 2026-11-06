#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("doctest", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.thirdParty().headerOnly().publicIncludes({"."});
});
