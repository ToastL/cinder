#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("harfbuzz", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.thirdParty().sources({"src/harfbuzz.cc"}).publicIncludes({"src"});
});
