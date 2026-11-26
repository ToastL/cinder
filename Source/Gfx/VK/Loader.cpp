#include "Gfx/VK/Loader.hpp"

#include <stdexcept>
#include <string>

#define CINDER_VK_DEFINE(name) PFN_##name name = nullptr;
CINDER_VK_DEFINE(vkGetInstanceProcAddr)
CINDER_VK_GLOBAL_FUNCTIONS(CINDER_VK_DEFINE)
CINDER_VK_INSTANCE_FUNCTIONS(CINDER_VK_DEFINE)
CINDER_VK_OPTIONAL_INSTANCE_FUNCTIONS(CINDER_VK_DEFINE)
CINDER_VK_DEVICE_FUNCTIONS(CINDER_VK_DEFINE)
#undef CINDER_VK_DEFINE

namespace cinder::Gfx::RHI {
namespace {

template <typename Fn>
void require(Fn& slot, PFN_vkVoidFunction found, const char* name) {
    if (found == nullptr) throw std::runtime_error(std::string("Vulkan loader has no ") + name);
    slot = reinterpret_cast<Fn>(found);
}

}

void loadGlobal(PFN_vkGetInstanceProcAddr entry) {
    if (entry == nullptr) throw std::runtime_error("No Vulkan loader found");
    vkGetInstanceProcAddr = entry;
#define CINDER_VK_LOAD(name) require(name, entry(VK_NULL_HANDLE, #name), #name);
    CINDER_VK_GLOBAL_FUNCTIONS(CINDER_VK_LOAD)
#undef CINDER_VK_LOAD
}

void loadInstance(VkInstance instance) {
#define CINDER_VK_LOAD(name) require(name, vkGetInstanceProcAddr(instance, #name), #name);
    CINDER_VK_INSTANCE_FUNCTIONS(CINDER_VK_LOAD)
#undef CINDER_VK_LOAD
#define CINDER_VK_LOAD(name) name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name));
    CINDER_VK_OPTIONAL_INSTANCE_FUNCTIONS(CINDER_VK_LOAD)
#undef CINDER_VK_LOAD
}

void loadDevice(VkDevice device) {
#define CINDER_VK_LOAD(name) require(name, vkGetDeviceProcAddr(device, #name), #name);
    CINDER_VK_DEVICE_FUNCTIONS(CINDER_VK_LOAD)
#undef CINDER_VK_LOAD
}

}
