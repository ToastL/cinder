#pragma once

#include "gfx/rhi/Handles.hpp"

#include <cstdint>
#include <string>

namespace cinder::gfx::rhi {

class Ctx;

class Texture {
public:
    static Texture load(const Ctx& ctx, const std::string& path);
    static Texture white(const Ctx& ctx);

    Texture(const Ctx& ctx, const unsigned char* pixels, std::uint32_t width,
            std::uint32_t height);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;

    void* handle() const { return handle_; }
    std::uint32_t width() const { return width_; }
    std::uint32_t height() const { return height_; }

    TextureBinding binding() const { return binding_; }
    void setBinding(TextureBinding binding) { binding_ = binding; }

private:
    void* handle_ = nullptr;
    TextureBinding binding_;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
};

}
