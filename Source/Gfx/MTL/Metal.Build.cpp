#include "CinderBuildTool/Rules.hpp"


CINDER_MODULE("Gfx/MTL", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"Gfx/RHI"});
    m.privateDependencies({"glfw"});
    m.objcFlags({"-fobjc-arc"});
    m.frameworks({"Metal", "QuartzCore", "Foundation", "CoreGraphics", "AppKit"});
});
