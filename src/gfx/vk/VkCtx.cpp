#include "gfx/vk/VkCtx.hpp"

#include "gfx/vk/VkUtil.hpp"
#include "platform/Log.hpp"
#include "platform/Window.hpp"

#include <GLFW/glfw3.h>

#include <cstring>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace cinder::gfx::vk {
namespace {

constexpr const char* VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation";

std::vector<std::string> instanceExtensions() {
    uint32_t count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> properties(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, properties.data());

    std::vector<std::string> names;
    names.reserve(count);
    for (const VkExtensionProperties& property : properties) names.emplace_back(property.extensionName);
    return names;
}

std::vector<std::string> deviceExtensions(VkPhysicalDevice device) {
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> properties(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, properties.data());

    std::vector<std::string> names;
    names.reserve(count);
    for (const VkExtensionProperties& property : properties) names.emplace_back(property.extensionName);
    return names;
}

bool has(const std::vector<std::string>& names, std::string_view name) {
    for (const std::string& entry : names) {
        if (entry == name) return true;
    }
    return false;
}

bool layerAvailable(const char* name) {
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    if (count == 0) return false;

    std::vector<VkLayerProperties> properties(count);
    vkEnumerateInstanceLayerProperties(&count, properties.data());
    for (const VkLayerProperties& property : properties) {
        if (std::strcmp(property.layerName, name) == 0) return true;
    }
    return false;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                             VkDebugUtilsMessageTypeFlagsEXT types,
                                             const VkDebugUtilsMessengerCallbackDataEXT* data,
                                             void* user) {
    cinder::platform::logError("[vk] %s\n", data->pMessage);
    return VK_FALSE;
}

}

VkCtx::VkCtx(cinder::platform::Window& window) {
    const std::vector<std::string> available = instanceExtensions();
    validation_ = layerAvailable(VALIDATION_LAYER)
            && has(available, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    if (!validation_) {
        cinder::platform::logInfo("[vk] validation unavailable (install the Vulkan SDK to enable)\n");
    }

    createInstance();
    if (validation_) createDebugMessenger();
    createSurface(window);
    pickPhysicalDevice();
    createLogicalDevice();
    createCommandPool();
    createTextureLayout();
    createAllocator();
}

void VkCtx::createInstance() {
    const std::vector<std::string> available = instanceExtensions();

    uint32_t glfwCount = 0;
    const char** glfwNames = glfwGetRequiredInstanceExtensions(&glfwCount);
    if (glfwNames == nullptr) throw std::runtime_error("GLFW returned no instance extensions");

    std::vector<std::string> wanted(glfwNames, glfwNames + glfwCount);

    const bool portability = has(available, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    if (portability) wanted.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    if (has(available, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
        wanted.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }
    if (validation_) wanted.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    for (const std::string& name : wanted) {
        if (!has(available, name)) {
            throw std::runtime_error("Required instance extension missing: " + name);
        }
    }

    std::vector<const char*> names;
    names.reserve(wanted.size());
    for (const std::string& name : wanted) names.push_back(name.c_str());

    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "cinder_game";
    app.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app.pEngineName = "cinder";
    app.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.flags = portability ? VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR : 0;
    info.pApplicationInfo = &app;
    info.enabledExtensionCount = static_cast<uint32_t>(names.size());
    info.ppEnabledExtensionNames = names.data();

    if (validation_) {
        info.enabledLayerCount = 1;
        info.ppEnabledLayerNames = &VALIDATION_LAYER;

        if (vkCreateInstance(&info, nullptr, &instance_) != VK_SUCCESS) {
            cinder::platform::logInfo(
                    "[vk] validation layer enumerated but would not load; continuing without it\n");
            validation_ = false;
            info.enabledLayerCount = 0;
            info.ppEnabledLayerNames = nullptr;
            info.enabledExtensionCount = static_cast<uint32_t>(names.size() - 1);
        }
    }

    if (instance_ == VK_NULL_HANDLE) {
        check(vkCreateInstance(&info, nullptr, &instance_), "vkCreateInstance");
    }
    volkLoadInstance(instance_);
}

void VkCtx::createDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = debugCallback;

    check(vkCreateDebugUtilsMessengerEXT(instance_, &info, nullptr, &debugMessenger_),
          "vkCreateDebugUtilsMessengerEXT");
}

void VkCtx::createSurface(cinder::platform::Window& window) {
    check(glfwCreateWindowSurface(instance_, window.handle(), nullptr, &surface_),
          "glfwCreateWindowSurface");
}

void VkCtx::pickPhysicalDevice() {
    uint32_t count = 0;
    check(vkEnumeratePhysicalDevices(instance_, &count, nullptr), "vkEnumeratePhysicalDevices");
    if (count == 0) throw std::runtime_error("No Vulkan-capable GPU found");

    std::vector<VkPhysicalDevice> devices(count);
    check(vkEnumeratePhysicalDevices(instance_, &count, devices.data()), "vkEnumeratePhysicalDevices");

    for (VkPhysicalDevice candidate : devices) {
        if (!findQueueFamilies(candidate)) continue;
        if (!has(deviceExtensions(candidate), VK_KHR_SWAPCHAIN_EXTENSION_NAME)) continue;

        physicalDevice_ = candidate;
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(candidate, &properties);
        cinder::platform::logInfo("[vk] using %s\n", properties.deviceName);
        return;
    }

    throw std::runtime_error("No suitable GPU (needs graphics + present + swapchain)");
}

bool VkCtx::findQueueFamilies(VkPhysicalDevice candidate) {
    graphicsFamily_ = -1;
    presentFamily_ = -1;

    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(candidate, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(candidate, &count, families.data());

    for (uint32_t i = 0; i < count; ++i) {
        if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && graphicsFamily_ < 0) {
            graphicsFamily_ = static_cast<int>(i);
        }
        VkBool32 supportsPresent = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface_, &supportsPresent);
        if (supportsPresent == VK_TRUE && presentFamily_ < 0) {
            presentFamily_ = static_cast<int>(i);
        }
    }
    return graphicsFamily_ >= 0 && presentFamily_ >= 0;
}

void VkCtx::createLogicalDevice() {
    const std::vector<std::string> supported = deviceExtensions(physicalDevice_);
    const std::set<int> unique = {graphicsFamily_, presentFamily_};

    const float priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queues;
    queues.reserve(unique.size());
    for (int family : unique) {
        VkDeviceQueueCreateInfo queue{};
        queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue.queueFamilyIndex = static_cast<uint32_t>(family);
        queue.queueCount = 1;
        queue.pQueuePriorities = &priority;
        queues.push_back(queue);
    }

    std::vector<const char*> names = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    if (has(supported, "VK_KHR_portability_subset")) names.push_back("VK_KHR_portability_subset");

    VkPhysicalDeviceFeatures features{};

    VkDeviceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.queueCreateInfoCount = static_cast<uint32_t>(queues.size());
    info.pQueueCreateInfos = queues.data();
    info.enabledExtensionCount = static_cast<uint32_t>(names.size());
    info.ppEnabledExtensionNames = names.data();
    info.pEnabledFeatures = &features;

    check(vkCreateDevice(physicalDevice_, &info, nullptr, &device_), "vkCreateDevice");
    volkLoadDevice(device_);

    vkGetDeviceQueue(device_, static_cast<uint32_t>(graphicsFamily_), 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, static_cast<uint32_t>(presentFamily_), 0, &presentQueue_);
}

void VkCtx::createTextureLayout() {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 1;
    info.pBindings = &binding;

    check(vkCreateDescriptorSetLayout(device_, &info, nullptr, &textureLayout_),
          "vkCreateDescriptorSetLayout");
}

void VkCtx::createCommandPool() {
    VkCommandPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = static_cast<uint32_t>(graphicsFamily_);

    check(vkCreateCommandPool(device_, &info, nullptr, &commandPool_), "vkCreateCommandPool");
}

void VkCtx::createAllocator() {
    VmaVulkanFunctions functions{};
    functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo info{};
    info.physicalDevice = physicalDevice_;
    info.device = device_;
    info.pVulkanFunctions = &functions;
    info.instance = instance_;
    info.vulkanApiVersion = VK_API_VERSION_1_2;

    check(vmaCreateAllocator(&info, &allocator_), "vmaCreateAllocator");
}

void VkCtx::waitIdle() const { vkDeviceWaitIdle(device_); }

VkCommandBuffer VkCtx::beginSingleTime() const {
    VkCommandBufferAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc.commandPool = commandPool_;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    check(vkAllocateCommandBuffers(device_, &alloc, &cmd), "vkAllocateCommandBuffers");

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    check(vkBeginCommandBuffer(cmd, &begin), "vkBeginCommandBuffer");
    return cmd;
}

void VkCtx::endSingleTime(VkCommandBuffer cmd) const {
    check(vkEndCommandBuffer(cmd), "vkEndCommandBuffer");

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    check(vkQueueSubmit(graphicsQueue_, 1, &submit, VK_NULL_HANDLE), "vkQueueSubmit");
    vkQueueWaitIdle(graphicsQueue_);
    vkFreeCommandBuffers(device_, commandPool_, 1, &cmd);
}

VkCtx::~VkCtx() {
    if (allocator_ != VK_NULL_HANDLE) vmaDestroyAllocator(allocator_);
    if (textureLayout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, textureLayout_, nullptr);
    }
    if (commandPool_ != VK_NULL_HANDLE) vkDestroyCommandPool(device_, commandPool_, nullptr);
    if (device_ != VK_NULL_HANDLE) vkDestroyDevice(device_, nullptr);
    if (surface_ != VK_NULL_HANDLE) vkDestroySurfaceKHR(instance_, surface_, nullptr);
    if (debugMessenger_ != VK_NULL_HANDLE) {
        vkDestroyDebugUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
    }
    if (instance_ != VK_NULL_HANDLE) vkDestroyInstance(instance_, nullptr);
}

}
