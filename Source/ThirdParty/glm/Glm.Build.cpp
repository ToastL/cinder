#include "CinderBuildTool/Rules.hpp"

CINDER_MODULE("glm", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.thirdParty().headerOnly().publicIncludes({"."});
    m.publicDefines({"GLM_FORCE_DEPTH_ZERO_TO_ONE", "GLM_ENABLE_EXPERIMENTAL"});
});
