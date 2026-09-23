#pragma once

#include "gfx/CompositePipeline.hpp"
#include "gfx/FrameTargets.hpp"
#include "gfx/UiRenderer.hpp"
#include "gfx/asset/Assets.hpp"
#include "gfx/pass/DrawPass.hpp"
#include "gfx/pass/MeshPipeline.hpp"
#include "gfx/pass/SpritePipeline.hpp"
#include "gfx/pass/ViewCamera.hpp"
#include "gfx/vk/FrameSync.hpp"
#include "gfx/vk/GpuBuffer.hpp"
#include "gfx/vk/Swapchain.hpp"
#include "ui/core/ElementList.hpp"

#include <glm/vec2.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cinder::platform { class Window; }
namespace cinder::scene { class DrawList; }

namespace cinder::gfx {

class Renderer {
public:
    static constexpr uint32_t FRAMES_IN_FLIGHT = 2;

    Renderer(const cinder::gfx::vk::VkCtx& ctx, cinder::platform::Window& window);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void setClearColor(float r, float g, float b);
    void setViewportSize(int width, int height);
    void overrideCamera(const cinder::scene::View& view);
    void releaseCamera();
    void beginFrame();
    void drawFrame();
    void capture(const std::string& path);
    void requestWindowCapture(const std::string& path);
    bool windowCapturePending() const { return !windowCapture_.empty(); }

    cinder::gfx::asset::Assets& assets() { return *assets_; }
    cinder::gfx::pass::ViewCamera& camera() { return camera_; }
    UiRenderer& ui() { return *ui_; }
    float pixelsPerPoint() const { return pixelsPerPoint_; }
    cinder::scene::DrawList& draws();

    void setUiPaint(std::function<void(cinder::ui::ElementList&)> paint) { uiPaint_ = std::move(paint); }

    void registerApi(cinder::lua::LuaApi& api);

private:
    bool embedded() const { return viewportWidth_ > 0; }
    glm::vec2 viewSize() const;
    VkExtent2D targetExtent() const;
    void createTargets();
    void createCommandBuffers();
    void resizeCameras();
    void measureScale();
    void recreateSwapchain();
    void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex);
    void recordWindowCapture(VkCommandBuffer cmd, uint32_t imageIndex);
    static void setViewport(VkCommandBuffer cmd, uint32_t width, uint32_t height);
    VkDescriptorSetLayout createTextureLayout();

    const cinder::gfx::vk::VkCtx& ctx_;
    cinder::platform::Window& window_;

    VkFormat depthFormat_ = VK_FORMAT_UNDEFINED;
    VkRenderPass sceneRenderPass_ = VK_NULL_HANDLE;
    VkRenderPass presentRenderPass_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout textureLayout_ = VK_NULL_HANDLE;

    std::unique_ptr<cinder::gfx::vk::Swapchain> swapchain_;
    FrameTargets targets_;
    std::vector<VkCommandBuffer> commandBuffers_;
    std::unique_ptr<cinder::gfx::vk::FrameSync> sync_;
    std::unique_ptr<cinder::gfx::asset::Assets> assets_;
    std::unique_ptr<cinder::gfx::pass::SpritePipeline> spritePipeline_;
    std::unique_ptr<cinder::gfx::pass::MeshPipeline> meshPipeline_;
    std::unique_ptr<CompositePipeline> compositePipeline_;
    std::unique_ptr<UiRenderer> ui_;
    std::vector<std::unique_ptr<cinder::gfx::pass::DrawPass>> passes_;
    std::unique_ptr<cinder::scene::DrawList> draws_;
    std::function<void(cinder::ui::ElementList&)> uiPaint_;
    cinder::ui::ElementList uiElements_;

    cinder::gfx::pass::ViewCamera camera_;
    std::optional<cinder::gfx::pass::ViewCamera> override_;

    uint32_t lastFrame_ = 0;
    std::string windowCapture_;
    std::unique_ptr<cinder::gfx::vk::GpuBuffer> captureBuffer_;
    VkImageUsageFlags swapchainUsage_ = 0;
    bool captureRecorded_ = false;
    float pixelsPerPoint_ = 1.0f;
    int viewportWidth_ = 0;
    int viewportHeight_ = 0;

    float clearR_ = 0.02f;
    float clearG_ = 0.02f;
    float clearB_ = 0.05f;
};

}
