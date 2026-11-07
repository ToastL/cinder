#pragma once

#include "Gfx/RHI/GlyphRegion.hpp"
#include "Gfx/RHI/Handles.hpp"
#include "Gfx/VK/TexturePool.hpp"
#include "Gfx/VK/VkImages.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace cinder::Gfx::RHI { class Ctx; }

namespace cinder::Gfx::RHI {

class GpuBuffer;

class GlyphPages {
public:
    GlyphPages(const cinder::Gfx::RHI::Ctx& ctx, std::uint32_t size);
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
        cinder::Gfx::VK::Allocated image;
        VkImageView view = VK_NULL_HANDLE;
        std::unique_ptr<TexturePool> pool;
        TextureBinding set;
        bool uploaded = false;
    };

    Page& page(std::size_t index);

    const cinder::Gfx::RHI::Ctx& ctx_;
    std::uint32_t size_ = 0;
    std::vector<Page> pages_;
};

}
