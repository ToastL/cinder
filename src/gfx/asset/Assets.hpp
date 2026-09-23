#pragma once

#include "gfx/rhi/Backend.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace cinder::gfx::asset {

class Assets {
public:
    static constexpr uint32_t MAX_TEXTURES = 256;

    explicit Assets(const cinder::gfx::vk::VkCtx& ctx);
    ~Assets();

    Assets(const Assets&) = delete;
    Assets& operator=(const Assets&) = delete;

    int load(const std::string& path);
    const cinder::gfx::rhi::Texture& get(int id) const;

private:
    int registerTexture(cinder::gfx::rhi::Texture texture);

    const cinder::gfx::vk::VkCtx& ctx_;
    cinder::gfx::rhi::TexturePool pool_;
    std::vector<cinder::gfx::rhi::Texture> textures_;
    std::unordered_map<std::string, int> byPath_;
};

}
