#pragma once

#include <vulkan/vulkan_core.h>

#define CINDER_VK_GLOBAL_FUNCTIONS(X) \
    X(vkCreateInstance) \
    X(vkEnumerateInstanceExtensionProperties) \
    X(vkEnumerateInstanceLayerProperties)

#define CINDER_VK_INSTANCE_FUNCTIONS(X) \
    X(vkCreateDevice) \
    X(vkDestroyInstance) \
    X(vkDestroySurfaceKHR) \
    X(vkEnumerateDeviceExtensionProperties) \
    X(vkEnumeratePhysicalDevices) \
    X(vkGetDeviceProcAddr) \
    X(vkGetPhysicalDeviceFormatProperties) \
    X(vkGetPhysicalDeviceProperties) \
    X(vkGetPhysicalDeviceQueueFamilyProperties) \
    X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
    X(vkGetPhysicalDeviceSurfaceFormatsKHR) \
    X(vkGetPhysicalDeviceSurfaceSupportKHR)

#define CINDER_VK_OPTIONAL_INSTANCE_FUNCTIONS(X) \
    X(vkCreateDebugUtilsMessengerEXT) \
    X(vkDestroyDebugUtilsMessengerEXT)

#define CINDER_VK_DEVICE_FUNCTIONS(X) \
    X(vkAcquireNextImageKHR) \
    X(vkAllocateCommandBuffers) \
    X(vkAllocateDescriptorSets) \
    X(vkBeginCommandBuffer) \
    X(vkCmdBeginRenderPass) \
    X(vkCmdBindDescriptorSets) \
    X(vkCmdBindIndexBuffer) \
    X(vkCmdBindPipeline) \
    X(vkCmdBindVertexBuffers) \
    X(vkCmdCopyBufferToImage) \
    X(vkCmdCopyImageToBuffer) \
    X(vkCmdDraw) \
    X(vkCmdDrawIndexed) \
    X(vkCmdEndRenderPass) \
    X(vkCmdPipelineBarrier) \
    X(vkCmdPushConstants) \
    X(vkCmdSetScissor) \
    X(vkCmdSetViewport) \
    X(vkCreateCommandPool) \
    X(vkCreateDescriptorPool) \
    X(vkCreateDescriptorSetLayout) \
    X(vkCreateFence) \
    X(vkCreateFramebuffer) \
    X(vkCreateGraphicsPipelines) \
    X(vkCreateImageView) \
    X(vkCreatePipelineLayout) \
    X(vkCreateRenderPass) \
    X(vkCreateSampler) \
    X(vkCreateSemaphore) \
    X(vkCreateShaderModule) \
    X(vkCreateSwapchainKHR) \
    X(vkDestroyCommandPool) \
    X(vkDestroyDescriptorPool) \
    X(vkDestroyDescriptorSetLayout) \
    X(vkDestroyDevice) \
    X(vkDestroyFence) \
    X(vkDestroyFramebuffer) \
    X(vkDestroyImageView) \
    X(vkDestroyPipeline) \
    X(vkDestroyPipelineLayout) \
    X(vkDestroyRenderPass) \
    X(vkDestroySampler) \
    X(vkDestroySemaphore) \
    X(vkDestroyShaderModule) \
    X(vkDestroySwapchainKHR) \
    X(vkDeviceWaitIdle) \
    X(vkEndCommandBuffer) \
    X(vkFreeCommandBuffers) \
    X(vkGetDeviceQueue) \
    X(vkGetSwapchainImagesKHR) \
    X(vkQueuePresentKHR) \
    X(vkQueueSubmit) \
    X(vkQueueWaitIdle) \
    X(vkResetCommandBuffer) \
    X(vkResetFences) \
    X(vkUpdateDescriptorSets) \
    X(vkWaitForFences)

#define CINDER_VK_DECLARE(name) extern __attribute__((visibility("hidden"))) PFN_##name name;
CINDER_VK_DECLARE(vkGetInstanceProcAddr)
CINDER_VK_GLOBAL_FUNCTIONS(CINDER_VK_DECLARE)
CINDER_VK_INSTANCE_FUNCTIONS(CINDER_VK_DECLARE)
CINDER_VK_OPTIONAL_INSTANCE_FUNCTIONS(CINDER_VK_DECLARE)
CINDER_VK_DEVICE_FUNCTIONS(CINDER_VK_DECLARE)
#undef CINDER_VK_DECLARE

namespace cinder::Gfx::RHI {

void loadGlobal(PFN_vkGetInstanceProcAddr entry);
void loadInstance(VkInstance instance);
void loadDevice(VkDevice device);

}
