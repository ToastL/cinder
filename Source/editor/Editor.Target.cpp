#include "CinderBuildTool/Rules.hpp"


CINDER_TARGET("editor", [](cbt::TargetRules& t, const cbt::TargetInfo&) {
    t.modules({"editor"}).exportSymbols().prerequisites({"player", "packager"});
});
