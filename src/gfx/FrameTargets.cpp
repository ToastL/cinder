#include "gfx/FrameTargets.hpp"

#include "gfx/vk/Presenter.hpp"
#include "gfx/vk/RenderTarget.hpp"

namespace cinder::gfx {

void FrameTargets::recreate(const cinder::gfx::rhi::Presenter& presenter, glm::uvec2 extent,
                            uint32_t count) {
    clear();
    frames_.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        frames_.push_back(presenter.createTarget(extent));
    }
}

void FrameTargets::clear() { frames_.clear(); }

}
