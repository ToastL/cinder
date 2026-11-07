#include "Gfx/UiRenderer.hpp"

#include "Gfx/RHI/Commands.hpp"

#include "Gfx/Asset/Assets.hpp"
#include "Gfx/RHI/PipelineBuilder.hpp"
#include "Gfx/RHI/Backend.hpp"
#include "Platform/Assets.hpp"
#include "Platform/Log.hpp"
#include "Text/GlyphAtlas.hpp"
#include "UI/Core/ElementList.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <exception>
#include <optional>

#include <glm/common.hpp>

namespace cinder::Gfx {

using cinder::Gfx::RHI::GpuBuffer;
using cinder::Gfx::RHI::Ctx;
using cinder::Text::GlyphAtlas;
using cinder::UI::TextureRef;
using cinder::UI::UiVertex;
namespace {

struct Push {
    glm::vec2 viewport{1.0f};
    float encode = 0.0f;
    float textGamma = 1.0f;
    float pixelsPerPoint = 1.0f;
};

constexpr cinder::Gfx::RHI::ShaderStages PUSH_STAGES = cinder::Gfx::RHI::ShaderStages::Both;

uint32_t offsetOf(std::size_t offset) { return static_cast<uint32_t>(offset); }

}

UiRenderer::UiRenderer(const Ctx& ctx, cinder::Gfx::Asset::Assets& assets,
                       const cinder::Gfx::RHI::Presenter& presenter, uint32_t framesInFlight)
    : ctx_(ctx), assets_(assets), presenter_(presenter), frames_(framesInFlight) {
    pages_ = std::make_unique<cinder::Gfx::RHI::GlyphPages>(
            ctx, static_cast<std::uint32_t>(GlyphAtlas::PAGE_SIZE));
    rebuild();
}

UiRenderer::~UiRenderer() = default;

void UiRenderer::rebuild() {
    encode_ = !cinder::Gfx::RHI::isSrgb(presenter_.colorFormat());
    pipeline_ = cinder::Gfx::RHI::PipelineBuilder(presenter_, cinder::Gfx::RHI::PassKind::Present)
                        .shader("ui")
                        .pushConstants(sizeof(Push), PUSH_STAGES)
                        .vertexStride(sizeof(UiVertex))
                        .attribute(0, cinder::Gfx::RHI::VertexFormat::Float2, offsetOf(offsetof(UiVertex, position)))
                        .attribute(1, cinder::Gfx::RHI::VertexFormat::Float2, offsetOf(offsetof(UiVertex, uv)))
                        .attribute(2, cinder::Gfx::RHI::VertexFormat::Float4, offsetOf(offsetof(UiVertex, color)))
                        .attribute(3, cinder::Gfx::RHI::VertexFormat::Float4, offsetOf(offsetof(UiVertex, border)))
                        .attribute(4, cinder::Gfx::RHI::VertexFormat::Float4, offsetOf(offsetof(UiVertex, shape)))
                        .attribute(5, cinder::Gfx::RHI::VertexFormat::Float4, offsetOf(offsetof(UiVertex, radii)))
                        .premultipliedBlend()
                        .build();
}

void UiRenderer::reserve(const Ctx& ctx, std::unique_ptr<GpuBuffer>& buffer, std::uint64_t size,
                         cinder::Gfx::RHI::BufferUsage usage) {
    if (buffer && buffer->size() >= size) return;
    const std::uint64_t grown = std::max(size, buffer ? buffer->size() * 2 : std::uint64_t{4096});
    buffer = std::make_unique<GpuBuffer>(ctx, grown, usage);
}

void UiRenderer::upload(cinder::Gfx::RHI::Uploads cmd, Frame& frame, GlyphAtlas& atlas) {
    if (&atlas != atlas_) {
        pages_->forgetUploads();
        atlas_ = &atlas;
    }

    std::vector<cinder::Gfx::RHI::GlyphRegion> regions;
    std::uint64_t total = 0;
    for (int index = 0; index < atlas.pageCount(); ++index) {
        const auto page = static_cast<std::size_t>(index);
        std::optional<cinder::Text::AtlasRect> dirty = atlas.dirty(index);
        if (!pages_->uploaded(page)) {
            dirty = cinder::Text::AtlasRect{0, 0, GlyphAtlas::PAGE_SIZE, GlyphAtlas::PAGE_SIZE};
        }
        if (!dirty || dirty->width <= 0 || dirty->height <= 0) continue;
        regions.push_back(cinder::Gfx::RHI::GlyphRegion{page, dirty->x, dirty->y, dirty->width,
                                                        dirty->height, total});
        total += static_cast<std::uint64_t>(dirty->width) * static_cast<std::uint64_t>(dirty->height);
    }
    if (regions.empty()) return;

    reserve(ctx_, frame.staging, total, cinder::Gfx::RHI::BufferUsage::TransferSrc);
    auto* staging = static_cast<std::uint8_t*>(frame.staging->mapped());

    for (const cinder::Gfx::RHI::GlyphRegion& region : regions) {
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
    for (const cinder::Gfx::RHI::GlyphRegion& region : regions) {
        atlas.clearDirty(static_cast<int>(region.page));
    }
}

void UiRenderer::resolveNames(const cinder::UI::ElementList& list) {
    named_.clear();
    for (const std::string& name : list.names()) {
        cinder::Gfx::RHI::TextureBinding set = assets_.get(0).binding();
        if (!failed_.contains(name)) {
            try {
                set = assets_.get(assets_.load(cinder::Platform::contentPath(name).string())).binding();
            } catch (const std::exception& error) {
                failed_.insert(name);
                cinder::Platform::logError("[ui] %s\n", error.what());
            }
        }
        named_.push_back(set);
    }
}

void UiRenderer::prepare(cinder::Gfx::RHI::Uploads cmd, uint32_t frame,
                         const cinder::UI::ElementList& list) {
    geometry_.clear();
    if (list.empty()) return;

    Frame& data = frames_[frame];
    if (GlyphAtlas* atlas = list.atlas()) upload(cmd, data, *atlas);
    resolveNames(list);

    cinder::UI::batch(list, geometry_);
    viewport_ = glm::max(list.size(), glm::vec2(1.0f));
    if (geometry_.empty()) return;

    const std::uint64_t vertexBytes = geometry_.vertices.size() * sizeof(UiVertex);
    const std::uint64_t indexBytes = geometry_.indices.size() * sizeof(std::uint32_t);
    reserve(ctx_, data.vertices, vertexBytes, cinder::Gfx::RHI::BufferUsage::Vertex);
    reserve(ctx_, data.indices, indexBytes, cinder::Gfx::RHI::BufferUsage::Index);
    std::memcpy(data.vertices->mapped(), geometry_.vertices.data(), static_cast<std::size_t>(vertexBytes));
    std::memcpy(data.indices->mapped(), geometry_.indices.data(), static_cast<std::size_t>(indexBytes));
}

cinder::Gfx::RHI::TextureBinding UiRenderer::resolve(const TextureRef& texture,
                                                     cinder::Gfx::RHI::TextureBinding viewport) const {
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

void UiRenderer::record(cinder::Gfx::RHI::Commands cmd, uint32_t frame, glm::uvec2 extent,
                        cinder::Gfx::RHI::TextureBinding viewport) {
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

    cinder::Gfx::RHI::TextureBinding bound;
    const float scale = push.pixelsPerPoint;
    for (const cinder::UI::UiBatch& batch : geometry_.batches) {
        const float left = std::clamp(std::floor(batch.clip.min.x * scale), 0.0f, static_cast<float>(extent.x));
        const float top = std::clamp(std::floor(batch.clip.min.y * scale), 0.0f, static_cast<float>(extent.y));
        const float right = std::clamp(std::ceil(batch.clip.max.x * scale), 0.0f, static_cast<float>(extent.x));
        const float bottom = std::clamp(std::ceil(batch.clip.max.y * scale), 0.0f, static_cast<float>(extent.y));
        if (right <= left || bottom <= top) continue;

        cinder::Gfx::RHI::scissor(cmd, static_cast<int32_t>(left), static_cast<int32_t>(top),
                                  static_cast<uint32_t>(right - left),
                                  static_cast<uint32_t>(bottom - top));

        const cinder::Gfx::RHI::TextureBinding set = resolve(batch.texture, viewport);
        if (set != bound) {
            pipeline_->bindTexture(cmd, set);
            bound = set;
        }
        cinder::Gfx::RHI::drawIndexed(cmd, batch.indexCount, batch.firstIndex);
    }

    cinder::Gfx::RHI::scissor(cmd, 0, 0, extent.x, extent.y);
}

}
