#include "CinderBuildTool/Rules.hpp"


CINDER_TARGET("editor", [](cbt::TargetRules& t, const cbt::TargetInfo&) {
    t.modules({"Editor"}).exportSymbols().prerequisites({"player", "packager"});
});
