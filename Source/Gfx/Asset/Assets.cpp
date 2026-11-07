#include "Gfx/Asset/Assets.hpp"

#include "Gfx/RHI/Backend.hpp"
#include "Scene/DrawList.hpp"

#include <stdexcept>
#include <utility>

namespace cinder::Gfx::Asset {

Assets::Assets(const cinder::Gfx::RHI::Ctx& ctx)
    : ctx_(ctx), pool_(ctx, MAX_TEXTURES, cinder::Gfx::RHI::SamplerFilter::Nearest) {
    registerTexture(cinder::Gfx::RHI::Texture::white(ctx));
}

int Assets::registerTexture(cinder::Gfx::RHI::Texture texture) {
    if (textures_.size() >= MAX_TEXTURES) {
        throw std::runtime_error("Texture limit reached (" + std::to_string(MAX_TEXTURES) + ")");
    }

    texture.setBinding(pool_.bind(texture));

    textures_.push_back(std::move(texture));
    return static_cast<int>(textures_.size()) - 1;
}

int Assets::load(const std::string& path) {
    auto found = byPath_.find(path);
    if (found != byPath_.end()) return found->second;

    const int id = registerTexture(cinder::Gfx::RHI::Texture::load(ctx_, path));
    byPath_.emplace(path, id);
    return id;
}

const cinder::Gfx::RHI::Texture& Assets::get(int id) const {
    if (id < 0 || static_cast<std::size_t>(id) >= textures_.size()) {
        return textures_[cinder::Scene::DrawList::WHITE];
    }
    return textures_[static_cast<std::size_t>(id)];
}

Assets::~Assets() = default;

}
