#pragma once

#include "gfx/rhi/Format.hpp"
#include "gfx/rhi/Handles.hpp"
#include "gfx/vk/GpuBuffer.hpp"
#include "gfx/vk/GraphicsPipeline.hpp"
#include "gfx/vk/TexturePool.hpp"
#include "gfx/vk/VkImages.hpp"
#include "ui/core/Batcher.hpp"

#include <volk.h>

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace cinder::gfx::asset { class Assets; }
namespace cinder::text { class GlyphAtlas; }
namespace cinder::ui { class ElementList; }

namespace cinder::gfx {

class UiRenderer {
public:
    static constexpr float DEFAULT_TEXT_GAMMA = 1.2f;

    UiRenderer(const cinder::gfx::vk::VkCtx& ctx, cinder::gfx::asset::Assets& assets, VkRenderPass renderPass,
               cinder::gfx::rhi::Format format, uint32_t framesInFlight);
    ~UiRenderer();

    UiRenderer(const UiRenderer&) = delete;
    UiRenderer& operator=(const UiRenderer&) = delete;

    void rebuild(VkRenderPass renderPass, cinder::gfx::rhi::Format format);
    void setTextGamma(float gamma) { textGamma_ = gamma; }

    void prepare(cinder::gfx::rhi::Uploads cmd, uint32_t frame, const cinder::ui::ElementList& list);
    void record(cinder::gfx::rhi::Commands cmd, uint32_t frame, VkExtent2D extent,
                cinder::gfx::rhi::TextureBinding viewport);

private:
    struct GlyphPage {
        cinder::gfx::vk::Allocated image;
        VkImageView view = VK_NULL_HANDLE;
        std::unique_ptr<cinder::gfx::vk::TexturePool> pool;
        cinder::gfx::rhi::TextureBinding set;
        bool uploaded = false;
    };

    struct Frame {
        std::unique_ptr<cinder::gfx::vk::GpuBuffer> vertices;
        std::unique_ptr<cinder::gfx::vk::GpuBuffer> indices;
        std::unique_ptr<cinder::gfx::vk::GpuBuffer> staging;
    };

    void upload(cinder::gfx::rhi::Uploads cmd, Frame& frame, cinder::text::GlyphAtlas& atlas);
    void resolveNames(const cinder::ui::ElementList& list);
    GlyphPage& page(std::size_t index);
    void destroyPage(GlyphPage& page);
    static void reserve(const cinder::gfx::vk::VkCtx& ctx, std::unique_ptr<cinder::gfx::vk::GpuBuffer>& buffer,
                        VkDeviceSize size, VkBufferUsageFlags usage);
    cinder::gfx::rhi::TextureBinding resolve(const cinder::ui::TextureRef& texture,
                                             cinder::gfx::rhi::TextureBinding viewport) const;

    const cinder::gfx::vk::VkCtx& ctx_;
    cinder::gfx::asset::Assets& assets_;
    std::unique_ptr<cinder::gfx::vk::GraphicsPipeline> pipeline_;
    std::vector<GlyphPage> pages_;
    std::vector<Frame> frames_;
    std::vector<cinder::gfx::rhi::TextureBinding> named_;
    std::unordered_set<std::string> failed_;
    cinder::ui::UiGeometry geometry_;
    const cinder::text::GlyphAtlas* atlas_ = nullptr;
    glm::vec2 viewport_{1.0f};
    bool encode_ = false;
    float textGamma_ = DEFAULT_TEXT_GAMMA;
};

}
