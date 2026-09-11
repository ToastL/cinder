#pragma once

#include <volk.h>

#include <cstdint>
#include <functional>
#include <memory>

namespace cinder::platform { class Window; }
namespace cinder::gfx::vk { class VkCtx; }

namespace cinder::gfx {

class Overlay {
public:
    virtual ~Overlay() = default;

    virtual void beginFrame() = 0;
    virtual void record(VkCommandBuffer cmd) = 0;
    virtual void discardFrame() = 0;
    virtual void setMinImageCount(uint32_t minImageCount) = 0;
    virtual VkDescriptorSet addTexture(VkImageView view) = 0;
    virtual void removeTexture(VkDescriptorSet texture) = 0;
};

using OverlayFactory = std::function<std::unique_ptr<Overlay>(
        const cinder::gfx::vk::VkCtx& ctx, cinder::platform::Window& window,
        VkRenderPass renderPass, uint32_t minImageCount, uint32_t imageCount)>;

}
