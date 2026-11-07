#pragma once

#include "gfx/rhi/GlyphRegion.hpp"
#include "gfx/rhi/Handles.hpp"
#include "gfx/vk/TexturePool.hpp"
#include "gfx/vk/VkImages.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace cinder::gfx::rhi { class Ctx; }

namespace cinder::gfx::rhi {

class GpuBuffer;

class GlyphPages {
public:
    GlyphPages(const cinder::gfx::rhi::Ctx& ctx, std::uint32_t size);
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

    const cinder::gfx::rhi::Ctx& ctx_;
    std::uint32_t size_ = 0;
    std::vector<Page> pages_;
};

}
