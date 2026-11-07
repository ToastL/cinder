#include "Gfx/FrameTargets.hpp"

#include "Gfx/RHI/Backend.hpp"

namespace cinder::Gfx {

void FrameTargets::recreate(const cinder::Gfx::RHI::Presenter& presenter, glm::uvec2 extent,
                            uint32_t count) {
    clear();
    frames_.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        frames_.push_back(presenter.createTarget(extent));
    }
}

void FrameTargets::clear() { frames_.clear(); }

}
