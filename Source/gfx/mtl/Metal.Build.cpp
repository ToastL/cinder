#include "CinderBuildTool/Rules.hpp"


CINDER_MODULE("gfx/mtl", [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
    m.publicDependencies({"gfx/rhi"});
    m.privateDependencies({"glfw"});
    m.objcFlags({"-fobjc-arc"});
    m.frameworks({"Metal", "QuartzCore", "Foundation", "CoreGraphics", "AppKit"});
});
