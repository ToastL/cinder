#pragma once

#include "Gfx/RHI/GlyphRegion.hpp"
#include "Gfx/RHI/Handles.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace cinder::Gfx::RHI {

class Ctx;
class GpuBuffer;
class TexturePool;

class GlyphPages {
public:
    GlyphPages(const Ctx& ctx, std::uint32_t size);
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
        void* texture = nullptr;
        std::unique_ptr<TexturePool> pool;
        TextureBinding binding;
        bool uploaded = false;
    };

    Page& page(std::size_t index);

    const Ctx& ctx_;
    std::uint32_t size_ = 0;
    std::vector<Page> pages_;
};

}
