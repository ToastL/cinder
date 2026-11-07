#pragma once

#include "Gfx/RHI/Handles.hpp"

#include <glm/mat4x4.hpp>

namespace cinder::Gfx::Pass {

class DrawPass {
public:
    virtual ~DrawPass() = default;

    virtual void beginFrame() = 0;
    virtual void record(cinder::Gfx::RHI::Commands cmd, uint32_t frameInFlight,
                        const glm::mat4& viewProjection) = 0;
};

}
