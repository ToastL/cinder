#pragma once

#include "gfx/rhi/Handles.hpp"
#include "gfx/vk/TexturePool.hpp"
#include "gfx/vk/VkImages.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace cinder::gfx::vk { class VkCtx; }

namespace cinder::gfx::rhi {

class GpuBuffer;

struct GlyphRegion {
    std::size_t page = 0;
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::uint64_t offset = 0;
};

class GlyphPages {
public:
    GlyphPages(const cinder::gfx::vk::VkCtx& ctx, std::uint32_t size);
    ~GlyphPages();

    GlyphPages(const GlyphPages&) = delete;
    GlyphPages& operator=(const GlyphPages&) = delete;

    std::size_t count() const { return pages_.size(); }
    TextureBinding binding(std::size_t index);
    bool uploaded(std::size_t index);
    void forgetUploads();

    void upload(Uploads cmd, const GpuBuffer& staging, const std::vector<GlyphRegion>& regions);

private:
    struct Page {
        cinder::gfx::vk::Allocated image;
        VkImageView view = VK_NULL_HANDLE;
        std::unique_ptr<TexturePool> pool;
        TextureBinding set;
        bool uploaded = false;
    };

    Page& page(std::size_t index);

    const cinder::gfx::vk::VkCtx& ctx_;
    std::uint32_t size_ = 0;
    std::vector<Page> pages_;
};

}
