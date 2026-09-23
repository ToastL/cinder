#pragma once

#include "gfx/rhi/Fwd.hpp"

#include "gfx/rhi/Handles.hpp"
#include "gfx/vk/GraphicsPipeline.hpp"

#include <memory>

namespace cinder::gfx::pass {

class SpritePipeline {
public:
    static constexpr uint32_t FLOATS_PER_VERTEX = 9;
    static constexpr uint32_t VERTEX_STRIDE = FLOATS_PER_VERTEX * sizeof(float);

    explicit SpritePipeline(const cinder::gfx::rhi::Presenter& presenter);

    void bind(cinder::gfx::rhi::Commands cmd, const glm::mat4& viewProjection) const;
    void bindTexture(cinder::gfx::rhi::Commands cmd,
                     cinder::gfx::rhi::TextureBinding texture) const {
        pipeline_->bindTexture(cmd, texture);
    }

private:
    std::unique_ptr<cinder::gfx::rhi::GraphicsPipeline> pipeline_;
};

}
