#include "CinderBuildTool/Rules.hpp"


CINDER_MODULE("Core", [](cbt::ModuleRules& m, const cbt::TargetInfo& t) {
    m.publicDependencies({"Serial", "Nodes", "Physics", "UI/Docking", "Gfx"});
    m.privateDefines({"CINDER_VERSION=\"" + t.version + "\""});
});
