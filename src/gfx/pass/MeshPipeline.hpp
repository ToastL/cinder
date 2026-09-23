#pragma once

#include "gfx/rhi/Handles.hpp"
#include "gfx/vk/GraphicsPipeline.hpp"

#include <memory>

namespace cinder::gfx::pass {

class MeshPipeline {
public:
    MeshPipeline(const cinder::gfx::vk::VkCtx& ctx, VkRenderPass renderPass);

    void bind(cinder::gfx::rhi::Commands cmd, const glm::mat4& viewProjection) const;
    void pushModel(cinder::gfx::rhi::Commands cmd, const glm::mat4& model) const;
    VkPipelineLayout layout() const { return pipeline_->layout(); }

private:
    std::unique_ptr<cinder::gfx::vk::GraphicsPipeline> pipeline_;
};

}
