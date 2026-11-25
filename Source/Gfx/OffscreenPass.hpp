#pragma once

#include "Gfx/RHI/Fwd.hpp"
#include "Gfx/RHI/Handles.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include <cstdint>

namespace cinder::Gfx {

class OffscreenPass {
public:
    OffscreenPass(cinder::Gfx::RHI::Presenter& presenter, cinder::Gfx::RHI::Frame& frame,
                  const glm::mat4& viewProjection, cinder::Gfx::RHI::TextureBinding sceneDepth,
                  glm::uvec2 sceneSize);

    cinder::Gfx::RHI::Commands commands() const;
    std::uint32_t frameInFlight() const;
    const glm::mat4& viewProjection() const { return viewProjection_; }
    cinder::Gfx::RHI::TextureBinding sceneDepth() const { return sceneDepth_; }
    glm::uvec2 sceneSize() const { return sceneSize_; }

    void begin(const cinder::Gfx::RHI::RenderTarget& target);
    void end();

private:
    cinder::Gfx::RHI::Presenter& presenter_;
    cinder::Gfx::RHI::Frame& frame_;
    const glm::mat4& viewProjection_;
    cinder::Gfx::RHI::TextureBinding sceneDepth_;
    glm::uvec2 sceneSize_;
};

}
