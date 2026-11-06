#include "platform/Glfw.hpp"

#include "platform/Log.hpp"

#if defined(CINDER_BACKEND_VK)
#include <volk.h>
#endif

#include <GLFW/glfw3.h>

#include <stdexcept>

#if defined(CINDER_BACKEND_VK) && (defined(__APPLE__) || defined(__linux__))
#include <dlfcn.h>
#endif

namespace cinder::platform {
namespace {

int owners = 0;

#if defined(CINDER_BACKEND_VK)

#if defined(__APPLE__)
constexpr const char* LOADER_CANDIDATES[] = {
    "libvulkan.dylib",
    "libvulkan.1.dylib",
    "/opt/homebrew/lib/libvulkan.dylib",
    "/opt/homebrew/lib/libvulkan.1.dylib",
    "/usr/local/lib/libvulkan.dylib",
    "/usr/local/lib/libvulkan.1.dylib",
    "libMoltenVK.dylib",
    "/opt/homebrew/lib/libMoltenVK.dylib",
    "/usr/local/lib/libMoltenVK.dylib",
};
#elif defined(__linux__)
constexpr const char* LOADER_CANDIDATES[] = {
    "libvulkan.so.1",
    "libvulkan.so",
};
#endif

#endif

void errorCallback(int code, const char* description) {
    logError("[glfw] %d: %s\n", code, description);
}

#if defined(CINDER_BACKEND_VK)

PFN_vkGetInstanceProcAddr findLoader() {
#if defined(__APPLE__) || defined(__linux__)
    for (const char* candidate : LOADER_CANDIDATES) {
        void* module = dlopen(candidate, RTLD_NOW | RTLD_LOCAL);
        if (module == nullptr) continue;
        auto entry = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
                dlsym(module, "vkGetInstanceProcAddr"));
        if (entry != nullptr) return entry;
        dlclose(module);
    }
    return nullptr;
#else
    return nullptr;
#endif
}

void initVulkan() {
    if (volkInitialize() == VK_SUCCESS) return;

    PFN_vkGetInstanceProcAddr entry = findLoader();
    if (entry == nullptr) {
        throw std::runtime_error(
                "No Vulkan loader found (install the Vulkan SDK, or `brew install vulkan-loader molten-vk`)");
    }
    volkInitializeCustom(entry);
}

#endif

}

void Glfw::acquire() {
    if (owners++ > 0) return;

    try {
#if defined(CINDER_BACKEND_VK)
        initVulkan();
        glfwSetErrorCallback(errorCallback);
        glfwInitVulkanLoader(vkGetInstanceProcAddr);
        if (!glfwInit()) throw std::runtime_error("Failed to init GLFW");
        if (!glfwVulkanSupported()) throw std::runtime_error("No Vulkan loader found");
#else
        glfwSetErrorCallback(errorCallback);
        if (!glfwInit()) throw std::runtime_error("Failed to init GLFW");
#endif
    } catch (...) {
        release();
        throw;
    }
}

void Glfw::release() {
    if (owners == 0) throw std::runtime_error("Glfw::release() without a matching acquire()");
    if (--owners > 0) return;
    glfwTerminate();
    glfwSetErrorCallback(nullptr);
}

void Glfw::pollEvents() { glfwPollEvents(); }

void Glfw::waitEvents() { glfwWaitEvents(); }

double Glfw::time() { return glfwGetTime(); }

}
