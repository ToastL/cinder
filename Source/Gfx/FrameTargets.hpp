#pragma once

#include "Gfx/RHI/Fwd.hpp"

#include <glm/vec2.hpp>

#include <memory>
#include <vector>

namespace cinder::Gfx {

class FrameTargets {
public:
    void recreate(const cinder::Gfx::RHI::Presenter& presenter, glm::uvec2 extent,
                  uint32_t count);
    void clear();
    cinder::Gfx::RHI::RenderTarget& at(uint32_t frame) const { return *frames_[frame]; }

private:
    std::vector<std::unique_ptr<cinder::Gfx::RHI::RenderTarget>> frames_;
};

}
