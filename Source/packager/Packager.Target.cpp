#include "CinderBuildTool/Rules.hpp"


CINDER_TARGET("packager", [](cbt::TargetRules& t, const cbt::TargetInfo&) {
    t.modules({"packager"});
});
