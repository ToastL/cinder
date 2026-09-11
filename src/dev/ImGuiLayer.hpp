#pragma once

#include "gfx/Overlay.hpp"

#include <volk.h>

#include <cstdint>

namespace cinder::platform { class Window; }
namespace cinder::gfx::vk { class VkCtx; }

namespace cinder::dev {

class ImGuiLayer final : public cinder::gfx::Overlay {
public:
    ImGuiLayer(const cinder::gfx::vk::VkCtx& ctx, cinder::platform::Window& window,
               VkRenderPass renderPass, uint32_t minImageCount, uint32_t imageCount);
    ~ImGuiLayer() override;

    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    void beginFrame() override;
    void record(VkCommandBuffer cmd) override;
    void discardFrame() override;
    void setMinImageCount(uint32_t minImageCount) override;
    VkDescriptorSet addTexture(VkImageView view) override;
    void removeTexture(VkDescriptorSet texture) override;

private:
    const cinder::gfx::vk::VkCtx& ctx_;
    bool building_ = false;
};

cinder::gfx::OverlayFactory overlayFactory();

}
