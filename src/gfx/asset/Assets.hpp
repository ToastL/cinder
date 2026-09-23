#pragma once

#include "gfx/asset/Texture.hpp"
#include "gfx/vk/TexturePool.hpp"

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
    const Texture& get(int id) const;

private:
    int registerTexture(Texture texture);

    const cinder::gfx::vk::VkCtx& ctx_;
    cinder::gfx::vk::TexturePool pool_;
    std::vector<Texture> textures_;
    std::unordered_map<std::string, int> byPath_;
};

}
