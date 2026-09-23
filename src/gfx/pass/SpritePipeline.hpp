#pragma once

#include "gfx/rhi/Handles.hpp"
#include "gfx/vk/GraphicsPipeline.hpp"

#include <memory>

namespace cinder::gfx::pass {

class SpritePipeline {
public:
    static constexpr uint32_t FLOATS_PER_VERTEX = 9;
    static constexpr uint32_t VERTEX_STRIDE = FLOATS_PER_VERTEX * sizeof(float);

    SpritePipeline(const cinder::gfx::vk::VkCtx& ctx, VkRenderPass renderPass);

    void bind(cinder::gfx::rhi::Commands cmd, const glm::mat4& viewProjection) const;
    VkPipelineLayout layout() const { return pipeline_->layout(); }

private:
    std::unique_ptr<cinder::gfx::vk::GraphicsPipeline> pipeline_;
};

}
