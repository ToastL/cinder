#include "gfx/UiRenderer.hpp"

#include "gfx/asset/Assets.hpp"
#include "gfx/vk/GraphicsPipelineBuilder.hpp"
#include "gfx/vk/VkCtx.hpp"
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

using cinder::gfx::vk::GpuBuffer;
using cinder::gfx::vk::GraphicsPipelineBuilder;
using cinder::gfx::vk::VkCtx;
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

constexpr VkShaderStageFlags PUSH_STAGES = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
constexpr VkFormat PAGE_FORMAT = VK_FORMAT_R8_UNORM;

bool srgb(VkFormat format) {
    switch (format) {
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32: return true;
        default: return false;
    }
}

uint32_t offsetOf(std::size_t offset) { return static_cast<uint32_t>(offset); }

void barrier(VkCommandBuffer cmd, VkImage image, VkImageLayout from, VkImageLayout to, VkAccessFlags srcAccess,
             VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) {
    VkImageMemoryBarrier info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    info.oldLayout = from;
    info.newLayout = to;
    info.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    info.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    info.image = image;
    info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    info.srcAccessMask = srcAccess;
    info.dstAccessMask = dstAccess;
    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &info);
}

}

UiRenderer::UiRenderer(const VkCtx& ctx, cinder::gfx::asset::Assets& assets, VkRenderPass renderPass,
                       VkFormat format, uint32_t framesInFlight)
    : ctx_(ctx), assets_(assets), frames_(framesInFlight) {
    rebuild(renderPass, format);
}

UiRenderer::~UiRenderer() {
    for (GlyphPage& page : pages_) destroyPage(page);
}

void UiRenderer::rebuild(VkRenderPass renderPass, VkFormat format) {
    encode_ = !srgb(format);
    pipeline_ = GraphicsPipelineBuilder(ctx_, renderPass)
                        .shader("ui")
                        .pushConstants(sizeof(Push), PUSH_STAGES)
                        .vertexStride(sizeof(UiVertex))
                        .attribute(0, VK_FORMAT_R32G32_SFLOAT, offsetOf(offsetof(UiVertex, position)))
                        .attribute(1, VK_FORMAT_R32G32_SFLOAT, offsetOf(offsetof(UiVertex, uv)))
                        .attribute(2, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(offsetof(UiVertex, color)))
                        .attribute(3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(offsetof(UiVertex, border)))
                        .attribute(4, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(offsetof(UiVertex, shape)))
                        .attribute(5, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(offsetof(UiVertex, radii)))
                        .premultipliedBlend()
                        .build();
}

void UiRenderer::reserve(const VkCtx& ctx, std::unique_ptr<GpuBuffer>& buffer, VkDeviceSize size,
                         VkBufferUsageFlags usage) {
    if (buffer && buffer->size() >= size) return;
    const VkDeviceSize grown = std::max(size, buffer ? buffer->size() * 2 : VkDeviceSize{4096});
    buffer = std::make_unique<GpuBuffer>(ctx, grown, usage, true);
}

UiRenderer::GlyphPage& UiRenderer::page(std::size_t index) {
    while (pages_.size() <= index) {
        GlyphPage& created = pages_.emplace_back();
        const auto size = static_cast<uint32_t>(GlyphAtlas::PAGE_SIZE);
        created.image = images::create(ctx_, PAGE_FORMAT, size, size,
                                       VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        created.view = images::view(ctx_, created.image.image, PAGE_FORMAT, VK_IMAGE_ASPECT_COLOR_BIT);
        created.pool = std::make_unique<cinder::gfx::vk::TexturePool>(ctx_, 1, VK_FILTER_LINEAR);
        created.set = created.pool->bind(created.view);
    }
    return pages_[index];
}

void UiRenderer::destroyPage(GlyphPage& page) {
    page.pool.reset();
    vkDestroyImageView(ctx_.device(), page.view, nullptr);
    vmaDestroyImage(ctx_.allocator(), page.image.image, page.image.allocation);
}

void UiRenderer::upload(VkCommandBuffer cmd, Frame& frame, GlyphAtlas& atlas) {
    if (&atlas != atlas_) {
        for (GlyphPage& existing : pages_) existing.uploaded = false;
        atlas_ = &atlas;
    }

    struct Pending {
        std::size_t page;
        cinder::text::AtlasRect rect;
        VkDeviceSize offset;
    };
    std::vector<Pending> pending;
    VkDeviceSize total = 0;
    for (int index = 0; index < atlas.pageCount(); ++index) {
        GlyphPage& target = page(static_cast<std::size_t>(index));
        std::optional<cinder::text::AtlasRect> dirty = atlas.dirty(index);
        if (!target.uploaded) dirty = cinder::text::AtlasRect{0, 0, GlyphAtlas::PAGE_SIZE, GlyphAtlas::PAGE_SIZE};
        if (!dirty || dirty->width <= 0 || dirty->height <= 0) continue;
        pending.push_back(Pending{static_cast<std::size_t>(index), *dirty, total});
        total += static_cast<VkDeviceSize>(dirty->width) * static_cast<VkDeviceSize>(dirty->height);
    }
    if (pending.empty()) return;

    reserve(ctx_, frame.staging, total, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    auto* staging = static_cast<std::uint8_t*>(frame.staging->mapped());

    for (const Pending& item : pending) {
        const std::vector<std::uint8_t>& pixels = atlas.pixels(static_cast<int>(item.page));
        for (int row = 0; row < item.rect.height; ++row) {
            const std::size_t source = static_cast<std::size_t>(item.rect.y + row) * GlyphAtlas::PAGE_SIZE
                    + static_cast<std::size_t>(item.rect.x);
            std::memcpy(staging + item.offset + static_cast<std::size_t>(row) * static_cast<std::size_t>(item.rect.width),
                        pixels.data() + source, static_cast<std::size_t>(item.rect.width));
        }

        GlyphPage& target = pages_[item.page];
        barrier(cmd, target.image.image,
                target.uploaded ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkBufferImageCopy copy{};
        copy.bufferOffset = item.offset;
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy.imageOffset = {item.rect.x, item.rect.y, 0};
        copy.imageExtent = {static_cast<uint32_t>(item.rect.width), static_cast<uint32_t>(item.rect.height), 1};
        vkCmdCopyBufferToImage(cmd, frame.staging->handle(), target.image.image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        barrier(cmd, target.image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        target.uploaded = true;
        atlas.clearDirty(static_cast<int>(item.page));
    }
}

void UiRenderer::resolveNames(const cinder::ui::ElementList& list) {
    named_.clear();
    for (const std::string& name : list.names()) {
        VkDescriptorSet set = assets_.get(0).descriptorSet();
        if (!failed_.contains(name)) {
            try {
                set = assets_.get(assets_.load(name)).descriptorSet();
            } catch (const std::exception& error) {
                failed_.insert(name);
                cinder::platform::logError("[ui] %s\n", error.what());
            }
        }
        named_.push_back(set);
    }
}

void UiRenderer::prepare(VkCommandBuffer cmd, uint32_t frame, const cinder::ui::ElementList& list) {
    geometry_.clear();
    if (list.empty()) return;

    Frame& data = frames_[frame];
    if (GlyphAtlas* atlas = list.atlas()) upload(cmd, data, *atlas);
    resolveNames(list);

    cinder::ui::batch(list, geometry_);
    viewport_ = glm::max(list.size(), glm::vec2(1.0f));
    if (geometry_.empty()) return;

    const VkDeviceSize vertexBytes = geometry_.vertices.size() * sizeof(UiVertex);
    const VkDeviceSize indexBytes = geometry_.indices.size() * sizeof(std::uint32_t);
    reserve(ctx_, data.vertices, vertexBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    reserve(ctx_, data.indices, indexBytes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    std::memcpy(data.vertices->mapped(), geometry_.vertices.data(), static_cast<std::size_t>(vertexBytes));
    std::memcpy(data.indices->mapped(), geometry_.indices.data(), static_cast<std::size_t>(indexBytes));
}

VkDescriptorSet UiRenderer::resolve(const TextureRef& texture, VkDescriptorSet viewport) const {
    switch (texture.kind) {
        case TextureRef::Kind::GlyphPage:
            if (texture.index < pages_.size()) return pages_[texture.index].set;
            break;
        case TextureRef::Kind::Viewport:
            if (viewport != VK_NULL_HANDLE) return viewport;
            break;
        case TextureRef::Kind::Named:
            if (texture.index < named_.size()) return named_[texture.index];
            break;
        case TextureRef::Kind::None: break;
    }
    return assets_.get(0).descriptorSet();
}

void UiRenderer::record(VkCommandBuffer cmd, uint32_t frame, VkExtent2D extent, VkDescriptorSet viewport) {
    if (geometry_.empty()) return;
    Frame& data = frames_[frame];

    Push push;
    push.viewport = viewport_;
    push.encode = encode_ ? 1.0f : 0.0f;
    push.textGamma = textGamma_;
    push.pixelsPerPoint = static_cast<float>(extent.width) / viewport_.x;

    pipeline_->bind(cmd);
    pipeline_->push(cmd, PUSH_STAGES, sizeof(Push), &push);
    const VkBuffer vertexBuffer = data.vertices->handle();
    const VkDeviceSize zero = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &zero);
    vkCmdBindIndexBuffer(cmd, data.indices->handle(), 0, VK_INDEX_TYPE_UINT32);

    VkDescriptorSet bound = VK_NULL_HANDLE;
    const float scale = push.pixelsPerPoint;
    for (const cinder::ui::UiBatch& batch : geometry_.batches) {
        const float left = std::clamp(std::floor(batch.clip.min.x * scale), 0.0f, static_cast<float>(extent.width));
        const float top = std::clamp(std::floor(batch.clip.min.y * scale), 0.0f, static_cast<float>(extent.height));
        const float right = std::clamp(std::ceil(batch.clip.max.x * scale), 0.0f, static_cast<float>(extent.width));
        const float bottom = std::clamp(std::ceil(batch.clip.max.y * scale), 0.0f, static_cast<float>(extent.height));
        if (right <= left || bottom <= top) continue;

        VkRect2D scissor{};
        scissor.offset = {static_cast<int32_t>(left), static_cast<int32_t>(top)};
        scissor.extent = {static_cast<uint32_t>(right - left), static_cast<uint32_t>(bottom - top)};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        const VkDescriptorSet set = resolve(batch.texture, viewport);
        if (set != bound) {
            pipeline_->bindDescriptorSet(cmd, set);
            bound = set;
        }
        vkCmdDrawIndexed(cmd, batch.indexCount, 1, batch.firstIndex, 0, 0);
    }

    VkRect2D full{};
    full.extent = extent;
    vkCmdSetScissor(cmd, 0, 1, &full);
}

}
