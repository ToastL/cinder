#pragma once

#include <volk.h>

#include <vk_mem_alloc.h>

namespace cinder::platform { class Window; }

namespace cinder::gfx::vk {

class VkCtx {
public:
    explicit VkCtx(cinder::platform::Window& window);
    ~VkCtx();

    VkCtx(const VkCtx&) = delete;
    VkCtx& operator=(const VkCtx&) = delete;

    VkPhysicalDevice physicalDevice() const { return physicalDevice_; }
    VkDevice device() const { return device_; }
    VkQueue graphicsQueue() const { return graphicsQueue_; }
    VkQueue presentQueue() const { return presentQueue_; }
    VkSurfaceKHR surface() const { return surface_; }
    VkCommandPool commandPool() const { return commandPool_; }
    VmaAllocator allocator() const { return allocator_; }
    uint32_t graphicsFamily() const { return static_cast<uint32_t>(graphicsFamily_); }
    uint32_t presentFamily() const { return static_cast<uint32_t>(presentFamily_); }

    void waitIdle() const;

    VkCommandBuffer beginSingleTime() const;
    void endSingleTime(VkCommandBuffer cmd) const;

private:
    void createInstance();
    void createDebugMessenger();
    void createSurface(cinder::platform::Window& window);
    void pickPhysicalDevice();
    bool findQueueFamilies(VkPhysicalDevice candidate);
    void createLogicalDevice();
    void createCommandPool();
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
    VmaAllocator allocator_ = VK_NULL_HANDLE;
};

}
