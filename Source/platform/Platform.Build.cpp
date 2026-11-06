#include "CinderBuildTool/Rules.hpp"


CINDER_MODULE("platform", [](cbt::ModuleRules& m, const cbt::TargetInfo& t) {
    m.publicDependencies({"glm"});
    m.privateDependencies({"glfw"});
    m.publicDefines({"GLFW_INCLUDE_NONE", "GLM_FORCE_DEPTH_ZERO_TO_ONE", "GLM_ENABLE_EXPERIMENTAL"});
    m.privateDefines({"CINDER_ENGINE_DEFAULT=\"" + t.root.string() + "\"",
                      "CINDER_SHADERS_DEFAULT=\"" + (t.binaries / "Shaders").string() + "\""});
    if (t.backend == "mtl") {
        m.publicDefines({"CINDER_BACKEND_MTL", "GLFW_EXPOSE_NATIVE_COCOA"});
    } else {
        m.publicDefines({"CINDER_BACKEND_VK"});
        m.privateDependencies({"volk"});
    }
});
