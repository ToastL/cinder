#include "CinderBuildTool/Rules.hpp"


CINDER_MODULE("core", [](cbt::ModuleRules& m, const cbt::TargetInfo& t) {
    m.publicDependencies({"serial", "components", "physics", "ui/docking", "gfx"});
    m.privateDefines({"CINDER_VERSION=\"" + t.version + "\""});
});
