#pragma once

#include "gfx/rhi/Format.hpp"
#include "gfx/rhi/Fwd.hpp"
#include "gfx/rhi/Handles.hpp"
#include "gfx/vk/GpuBuffer.hpp"
#include "gfx/vk/GraphicsPipeline.hpp"
#include "gfx/vk/GlyphPages.hpp"
#include "ui/core/Batcher.hpp"

#include <glm/vec2.hpp>

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

    UiRenderer(const cinder::gfx::rhi::Ctx& ctx, cinder::gfx::asset::Assets& assets,
               const cinder::gfx::rhi::Presenter& presenter, uint32_t framesInFlight);
    ~UiRenderer();

    UiRenderer(const UiRenderer&) = delete;
    UiRenderer& operator=(const UiRenderer&) = delete;

    void rebuild();
    void setTextGamma(float gamma) { textGamma_ = gamma; }

    void prepare(cinder::gfx::rhi::Uploads cmd, uint32_t frame, const cinder::ui::ElementList& list);
    void record(cinder::gfx::rhi::Commands cmd, uint32_t frame, glm::uvec2 extent,
                cinder::gfx::rhi::TextureBinding viewport);

private:
    struct Frame {
        std::unique_ptr<cinder::gfx::rhi::GpuBuffer> vertices;
        std::unique_ptr<cinder::gfx::rhi::GpuBuffer> indices;
        std::unique_ptr<cinder::gfx::rhi::GpuBuffer> staging;
    };

    void upload(cinder::gfx::rhi::Uploads cmd, Frame& frame, cinder::text::GlyphAtlas& atlas);
    void resolveNames(const cinder::ui::ElementList& list);
    static void reserve(const cinder::gfx::rhi::Ctx& ctx, std::unique_ptr<cinder::gfx::rhi::GpuBuffer>& buffer,
                        std::uint64_t size, cinder::gfx::rhi::BufferUsage usage);
    cinder::gfx::rhi::TextureBinding resolve(const cinder::ui::TextureRef& texture,
                                             cinder::gfx::rhi::TextureBinding viewport) const;

    const cinder::gfx::rhi::Ctx& ctx_;
    cinder::gfx::asset::Assets& assets_;
    const cinder::gfx::rhi::Presenter& presenter_;
    std::unique_ptr<cinder::gfx::rhi::GraphicsPipeline> pipeline_;
    std::unique_ptr<cinder::gfx::rhi::GlyphPages> pages_;
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
