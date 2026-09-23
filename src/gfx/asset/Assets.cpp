#include "gfx/asset/Assets.hpp"

#include "gfx/vk/VkCtx.hpp"
#include "scene/DrawList.hpp"

#include <stdexcept>
#include <utility>

namespace cinder::gfx::asset {

Assets::Assets(const cinder::gfx::vk::VkCtx& ctx)
    : ctx_(ctx), pool_(ctx, MAX_TEXTURES, VK_FILTER_NEAREST) {
    registerTexture(Texture::white(ctx));
}

int Assets::registerTexture(Texture texture) {
    if (textures_.size() >= MAX_TEXTURES) {
        throw std::runtime_error("Texture limit reached (" + std::to_string(MAX_TEXTURES) + ")");
    }

    texture.setBinding(pool_.bind(texture.view()));

    textures_.push_back(std::move(texture));
    return static_cast<int>(textures_.size()) - 1;
}

int Assets::load(const std::string& path) {
    auto found = byPath_.find(path);
    if (found != byPath_.end()) return found->second;

    const int id = registerTexture(Texture::load(ctx_, path));
    byPath_.emplace(path, id);
    return id;
}

const Texture& Assets::get(int id) const {
    if (id < 0 || static_cast<std::size_t>(id) >= textures_.size()) {
        return textures_[cinder::scene::DrawList::WHITE];
    }
    return textures_[static_cast<std::size_t>(id)];
}

Assets::~Assets() { textures_.clear(); }

}
