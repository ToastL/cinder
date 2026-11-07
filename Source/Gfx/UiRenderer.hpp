#pragma once

#include "Gfx/RHI/Format.hpp"
#include "Gfx/RHI/Fwd.hpp"
#include "Gfx/RHI/Handles.hpp"
#include "Gfx/RHI/Backend.hpp"
#include "UI/Core/Batcher.hpp"

#include <glm/vec2.hpp>

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace cinder::Gfx::Asset { class Assets; }
namespace cinder::Text { class GlyphAtlas; }
namespace cinder::UI { class ElementList; }

namespace cinder::Gfx {

class UiRenderer {
public:
    static constexpr float DEFAULT_TEXT_GAMMA = 1.2f;

    UiRenderer(const cinder::Gfx::RHI::Ctx& ctx, cinder::Gfx::Asset::Assets& assets,
               const cinder::Gfx::RHI::Presenter& presenter, uint32_t framesInFlight);
    ~UiRenderer();

    UiRenderer(const UiRenderer&) = delete;
    UiRenderer& operator=(const UiRenderer&) = delete;

    void rebuild();
    void setTextGamma(float gamma) { textGamma_ = gamma; }

    void prepare(cinder::Gfx::RHI::Uploads cmd, uint32_t frame, const cinder::UI::ElementList& list);
    void record(cinder::Gfx::RHI::Commands cmd, uint32_t frame, glm::uvec2 extent,
                cinder::Gfx::RHI::TextureBinding viewport);

private:
    struct Frame {
        std::unique_ptr<cinder::Gfx::RHI::GpuBuffer> vertices;
        std::unique_ptr<cinder::Gfx::RHI::GpuBuffer> indices;
        std::unique_ptr<cinder::Gfx::RHI::GpuBuffer> staging;
    };

    void upload(cinder::Gfx::RHI::Uploads cmd, Frame& frame, cinder::Text::GlyphAtlas& atlas);
    void resolveNames(const cinder::UI::ElementList& list);
    static void reserve(const cinder::Gfx::RHI::Ctx& ctx, std::unique_ptr<cinder::Gfx::RHI::GpuBuffer>& buffer,
                        std::uint64_t size, cinder::Gfx::RHI::BufferUsage usage);
    cinder::Gfx::RHI::TextureBinding resolve(const cinder::UI::TextureRef& texture,
                                             cinder::Gfx::RHI::TextureBinding viewport) const;

    const cinder::Gfx::RHI::Ctx& ctx_;
    cinder::Gfx::Asset::Assets& assets_;
    const cinder::Gfx::RHI::Presenter& presenter_;
    std::unique_ptr<cinder::Gfx::RHI::GraphicsPipeline> pipeline_;
    std::unique_ptr<cinder::Gfx::RHI::GlyphPages> pages_;
    std::vector<Frame> frames_;
    std::vector<cinder::Gfx::RHI::TextureBinding> named_;
    std::unordered_set<std::string> failed_;
    cinder::UI::UiGeometry geometry_;
    const cinder::Text::GlyphAtlas* atlas_ = nullptr;
    glm::vec2 viewport_{1.0f};
    bool encode_ = false;
    float textGamma_ = DEFAULT_TEXT_GAMMA;
};

}
