#pragma once

#include "Gfx/RHI/Format.hpp"

#include <volk.h>

#include <vk_mem_alloc.h>

namespace cinder::Platform { class Window; }

namespace cinder::Gfx::RHI {

class Ctx {
public:
    explicit Ctx(cinder::Platform::Window& window);
    ~Ctx();

    Ctx(const Ctx&) = delete;
    Ctx& operator=(const Ctx&) = delete;

    VkPhysicalDevice physicalDevice() const { return physicalDevice_; }
    VkDevice device() const { return device_; }
    VkQueue graphicsQueue() const { return graphicsQueue_; }
    VkQueue presentQueue() const { return presentQueue_; }
    VkSurfaceKHR surface() const { return surface_; }
    VkCommandPool commandPool() const { return commandPool_; }
    VkDescriptorSetLayout textureLayout() const { return textureLayout_; }
    RHI::Format depthFormat() const { return depthFormat_; }
    VmaAllocator allocator() const { return allocator_; }
    uint32_t graphicsFamily() const { return static_cast<uint32_t>(graphicsFamily_); }
    uint32_t presentFamily() const { return static_cast<uint32_t>(presentFamily_); }

    void waitIdle() const;

    VkCommandBuffer beginSingleTime() const;
    void endSingleTime(VkCommandBuffer cmd) const;

private:
    void createInstance();
    void createDebugMessenger();
    void createSurface(cinder::Platform::Window& window);
    void pickPhysicalDevice();
    bool findQueueFamilies(VkPhysicalDevice candidate);
    void createLogicalDevice();
    void createCommandPool();
    void createTextureLayout();
    void chooseDepthFormat();
    void createAllocator();

    bool validation_ = false;
    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;
    int graphicsFamily_ = -1;
    int presentFamily_ = -1;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout textureLayout_ = VK_NULL_HANDLE;
    RHI::Format depthFormat_ = RHI::Format::Undefined;
    VmaAllocator allocator_ = VK_NULL_HANDLE;
};

}
