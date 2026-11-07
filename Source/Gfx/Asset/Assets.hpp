#pragma once

#include "Gfx/RHI/Backend.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace cinder::Gfx::Asset {

class Assets {
public:
    static constexpr uint32_t MAX_TEXTURES = 256;

    explicit Assets(const cinder::Gfx::RHI::Ctx& ctx);
    ~Assets();

    Assets(const Assets&) = delete;
    Assets& operator=(const Assets&) = delete;

    int load(const std::string& path);
    const cinder::Gfx::RHI::Texture& get(int id) const;

private:
    int registerTexture(cinder::Gfx::RHI::Texture texture);

    const cinder::Gfx::RHI::Ctx& ctx_;
    cinder::Gfx::RHI::TexturePool pool_;
    std::vector<cinder::Gfx::RHI::Texture> textures_;
    std::unordered_map<std::string, int> byPath_;
};

}
