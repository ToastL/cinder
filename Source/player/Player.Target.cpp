#include "CinderBuildTool/Rules.hpp"


CINDER_TARGET("player", [](cbt::TargetRules& t, const cbt::TargetInfo&) {
    t.modules({"player"}).exportSymbols();
});
