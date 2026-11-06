#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("glm", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.thirdParty().headerOnly().publicIncludes({"."});
});
