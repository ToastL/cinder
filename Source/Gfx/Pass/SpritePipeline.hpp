#pragma once

#include "Gfx/RHI/Fwd.hpp"

#include "Gfx/RHI/Handles.hpp"
#include "Gfx/RHI/Backend.hpp"

#include <memory>

namespace cinder::Gfx::Pass {

class SpritePipeline {
public:
    static constexpr uint32_t FLOATS_PER_VERTEX = 9;
    static constexpr uint32_t VERTEX_STRIDE = FLOATS_PER_VERTEX * sizeof(float);
    static constexpr uint32_t UV_OFFSET = 3 * sizeof(float);
    static constexpr uint32_t COLOR_OFFSET = 5 * sizeof(float);

    explicit SpritePipeline(const cinder::Gfx::RHI::Presenter& presenter);

    void bind(cinder::Gfx::RHI::Commands cmd, const glm::mat4& viewProjection) const;
    void bindTexture(cinder::Gfx::RHI::Commands cmd,
                     cinder::Gfx::RHI::TextureBinding texture) const {
        pipeline_->bindTexture(cmd, texture);
    }

private:
    std::unique_ptr<cinder::Gfx::RHI::GraphicsPipeline> pipeline_;
};

}
