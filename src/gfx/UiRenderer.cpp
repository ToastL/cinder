#include "gfx/UiRenderer.hpp"

#include "gfx/rhi/Commands.hpp"
#include "gfx/vk/Commands.hpp"

#include "gfx/asset/Assets.hpp"
#include "gfx/rhi/PipelineBuilder.hpp"
#include "gfx/vk/GlyphPages.hpp"
#include "gfx/vk/Presenter.hpp"
#include "gfx/vk/Ctx.hpp"
#include "platform/Log.hpp"
#include "text/GlyphAtlas.hpp"
#include "ui/core/ElementList.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <exception>
#include <optional>

#include <glm/common.hpp>

namespace cinder::gfx {

using cinder::gfx::rhi::GpuBuffer;
using cinder::gfx::rhi::Ctx;
using cinder::text::GlyphAtlas;
using cinder::ui::TextureRef;
using cinder::ui::UiVertex;
namespace images = cinder::gfx::vk::images;

namespace {

struct Push {
    glm::vec2 viewport{1.0f};
    float encode = 0.0f;
    float textGamma = 1.0f;
    float pixelsPerPoint = 1.0f;
};

constexpr cinder::gfx::rhi::ShaderStages PUSH_STAGES = cinder::gfx::rhi::ShaderStages::Both;

uint32_t offsetOf(std::size_t offset) { return static_cast<uint32_t>(offset); }

}

UiRenderer::UiRenderer(const Ctx& ctx, cinder::gfx::asset::Assets& assets,
                       const cinder::gfx::rhi::Presenter& presenter, uint32_t framesInFlight)
    : ctx_(ctx), assets_(assets), presenter_(presenter), frames_(framesInFlight) {
    pages_ = std::make_unique<cinder::gfx::rhi::GlyphPages>(
            ctx, static_cast<std::uint32_t>(GlyphAtlas::PAGE_SIZE));
    rebuild();
}

UiRenderer::~UiRenderer() = default;

void UiRenderer::rebuild() {
    encode_ = !cinder::gfx::rhi::isSrgb(presenter_.colorFormat());
    pipeline_ = cinder::gfx::rhi::PipelineBuilder(presenter_, cinder::gfx::rhi::PassKind::Present)
                        .shader("ui")
                        .pushConstants(sizeof(Push), PUSH_STAGES)
                        .vertexStride(sizeof(UiVertex))
                        .attribute(0, cinder::gfx::rhi::VertexFormat::Float2, offsetOf(offsetof(UiVertex, position)))
                        .attribute(1, cinder::gfx::rhi::VertexFormat::Float2, offsetOf(offsetof(UiVertex, uv)))
                        .attribute(2, cinder::gfx::rhi::VertexFormat::Float4, offsetOf(offsetof(UiVertex, color)))
                        .attribute(3, cinder::gfx::rhi::VertexFormat::Float4, offsetOf(offsetof(UiVertex, border)))
                        .attribute(4, cinder::gfx::rhi::VertexFormat::Float4, offsetOf(offsetof(UiVertex, shape)))
                        .attribute(5, cinder::gfx::rhi::VertexFormat::Float4, offsetOf(offsetof(UiVertex, radii)))
                        .premultipliedBlend()
                        .build();
}

void UiRenderer::reserve(const Ctx& ctx, std::unique_ptr<GpuBuffer>& buffer, std::uint64_t size,
                         cinder::gfx::rhi::BufferUsage usage) {
    if (buffer && buffer->size() >= size) return;
    const std::uint64_t grown = std::max(size, buffer ? buffer->size() * 2 : std::uint64_t{4096});
    buffer = std::make_unique<GpuBuffer>(ctx, grown, usage, true);
}

void UiRenderer::upload(cinder::gfx::rhi::Uploads cmd, Frame& frame, GlyphAtlas& atlas) {
    if (&atlas != atlas_) {
        pages_->forgetUploads();
        atlas_ = &atlas;
    }

    std::vector<cinder::gfx::rhi::GlyphRegion> regions;
    std::uint64_t total = 0;
    for (int index = 0; index < atlas.pageCount(); ++index) {
        const auto page = static_cast<std::size_t>(index);
        std::optional<cinder::text::AtlasRect> dirty = atlas.dirty(index);
        if (!pages_->uploaded(page)) {
            dirty = cinder::text::AtlasRect{0, 0, GlyphAtlas::PAGE_SIZE, GlyphAtlas::PAGE_SIZE};
        }
        if (!dirty || dirty->width <= 0 || dirty->height <= 0) continue;
        regions.push_back(cinder::gfx::rhi::GlyphRegion{page, dirty->x, dirty->y, dirty->width,
                                                        dirty->height, total});
        total += static_cast<std::uint64_t>(dirty->width) * static_cast<std::uint64_t>(dirty->height);
    }
    if (regions.empty()) return;

    reserve(ctx_, frame.staging, total, cinder::gfx::rhi::BufferUsage::TransferSrc);
    auto* staging = static_cast<std::uint8_t*>(frame.staging->mapped());

    for (const cinder::gfx::rhi::GlyphRegion& region : regions) {
        const std::vector<std::uint8_t>& pixels = atlas.pixels(static_cast<int>(region.page));
        for (int row = 0; row < region.height; ++row) {
            const std::size_t source = static_cast<std::size_t>(region.y + row) * GlyphAtlas::PAGE_SIZE
                    + static_cast<std::size_t>(region.x);
            std::memcpy(staging + region.offset
                                + static_cast<std::size_t>(row) * static_cast<std::size_t>(region.width),
                        pixels.data() + source, static_cast<std::size_t>(region.width));
        }
    }

    pages_->upload(cmd, *frame.staging, regions);
    for (const cinder::gfx::rhi::GlyphRegion& region : regions) {
        atlas.clearDirty(static_cast<int>(region.page));
    }
}

void UiRenderer::resolveNames(const cinder::ui::ElementList& list) {
    named_.clear();
    for (const std::string& name : list.names()) {
        cinder::gfx::rhi::TextureBinding set = assets_.get(0).binding();
        if (!failed_.contains(name)) {
            try {
                set = assets_.get(assets_.load(name)).binding();
            } catch (const std::exception& error) {
                failed_.insert(name);
                cinder::platform::logError("[ui] %s\n", error.what());
            }
        }
        named_.push_back(set);
    }
}

void UiRenderer::prepare(cinder::gfx::rhi::Uploads cmd, uint32_t frame,
                         const cinder::ui::ElementList& list) {
    geometry_.clear();
    if (list.empty()) return;

    Frame& data = frames_[frame];
    if (GlyphAtlas* atlas = list.atlas()) upload(cmd, data, *atlas);
    resolveNames(list);

    cinder::ui::batch(list, geometry_);
    viewport_ = glm::max(list.size(), glm::vec2(1.0f));
    if (geometry_.empty()) return;

    const std::uint64_t vertexBytes = geometry_.vertices.size() * sizeof(UiVertex);
    const std::uint64_t indexBytes = geometry_.indices.size() * sizeof(std::uint32_t);
    reserve(ctx_, data.vertices, vertexBytes, cinder::gfx::rhi::BufferUsage::Vertex);
    reserve(ctx_, data.indices, indexBytes, cinder::gfx::rhi::BufferUsage::Index);
    std::memcpy(data.vertices->mapped(), geometry_.vertices.data(), static_cast<std::size_t>(vertexBytes));
    std::memcpy(data.indices->mapped(), geometry_.indices.data(), static_cast<std::size_t>(indexBytes));
}

cinder::gfx::rhi::TextureBinding UiRenderer::resolve(const TextureRef& texture,
                                                     cinder::gfx::rhi::TextureBinding viewport) const {
    switch (texture.kind) {
        case TextureRef::Kind::GlyphPage:
            if (texture.index < pages_->count()) return pages_->binding(texture.index);
            break;
        case TextureRef::Kind::Viewport:
            if (viewport) return viewport;
            break;
        case TextureRef::Kind::Named:
            if (texture.index < named_.size()) return named_[texture.index];
            break;
        case TextureRef::Kind::None: break;
    }
    return assets_.get(0).binding();
}

void UiRenderer::record(cinder::gfx::rhi::Commands cmd, uint32_t frame, glm::uvec2 extent,
                        cinder::gfx::rhi::TextureBinding viewport) {
    if (geometry_.empty()) return;
    Frame& data = frames_[frame];

    Push push;
    push.viewport = viewport_;
    push.encode = encode_ ? 1.0f : 0.0f;
    push.textGamma = textGamma_;
    push.pixelsPerPoint = static_cast<float>(extent.x) / viewport_.x;

    pipeline_->bind(cmd);
    pipeline_->push(cmd, PUSH_STAGES, sizeof(Push), &push);
    data.vertices->bindVertex(cmd);
    data.indices->bindIndex(cmd);

    cinder::gfx::rhi::TextureBinding bound;
    const float scale = push.pixelsPerPoint;
    for (const cinder::ui::UiBatch& batch : geometry_.batches) {
        const float left = std::clamp(std::floor(batch.clip.min.x * scale), 0.0f, static_cast<float>(extent.x));
        const float top = std::clamp(std::floor(batch.clip.min.y * scale), 0.0f, static_cast<float>(extent.y));
        const float right = std::clamp(std::ceil(batch.clip.max.x * scale), 0.0f, static_cast<float>(extent.x));
        const float bottom = std::clamp(std::ceil(batch.clip.max.y * scale), 0.0f, static_cast<float>(extent.y));
        if (right <= left || bottom <= top) continue;

        cinder::gfx::rhi::scissor(cmd, static_cast<int32_t>(left), static_cast<int32_t>(top),
                                  static_cast<uint32_t>(right - left),
                                  static_cast<uint32_t>(bottom - top));

        const cinder::gfx::rhi::TextureBinding set = resolve(batch.texture, viewport);
        if (set != bound) {
            pipeline_->bindTexture(cmd, set);
            bound = set;
        }
        cinder::gfx::rhi::drawIndexed(cmd, batch.indexCount, batch.firstIndex);
    }

    cinder::gfx::rhi::scissor(cmd, 0, 0, extent.x, extent.y);
}

}
