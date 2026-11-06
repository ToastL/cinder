#pragma once

#include "gfx/rhi/Fwd.hpp"

#include <glm/vec2.hpp>

#include <memory>
#include <vector>

namespace cinder::gfx {

class FrameTargets {
public:
    void recreate(const cinder::gfx::rhi::Presenter& presenter, glm::uvec2 extent,
                  uint32_t count);
    void clear();
    cinder::gfx::rhi::RenderTarget& at(uint32_t frame) const { return *frames_[frame]; }

private:
    std::vector<std::unique_ptr<cinder::gfx::rhi::RenderTarget>> frames_;
};

}
