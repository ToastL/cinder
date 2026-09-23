#include "gfx/vk/GraphicsPipeline.hpp"

#include "gfx/vk/Commands.hpp"
#include "gfx/vk/VkCtx.hpp"

#include <glm/gtc/type_ptr.hpp>

namespace cinder::gfx::vk {

GraphicsPipeline::GraphicsPipeline(const VkCtx& ctx, VkPipelineLayout layout, VkPipeline handle)
    : ctx_(ctx), layout_(layout), handle_(handle) {}

void GraphicsPipeline::bind(rhi::Commands cmd) const {
    vkCmdBindPipeline(unwrap(cmd), VK_PIPELINE_BIND_POINT_GRAPHICS, handle_);
}

void GraphicsPipeline::push(rhi::Commands cmd, uint32_t offset, const glm::mat4& value) const {
    vkCmdPushConstants(unwrap(cmd), layout_, VK_SHADER_STAGE_VERTEX_BIT, offset, MATRIX_BYTES,
                       glm::value_ptr(value));
}

void GraphicsPipeline::push(rhi::Commands cmd, VkShaderStageFlags stages, uint32_t size,
                            const void* data) const {
    vkCmdPushConstants(unwrap(cmd), layout_, stages, 0, size, data);
}

void GraphicsPipeline::bindDescriptorSet(rhi::Commands cmd, VkDescriptorSet set) const {
    vkCmdBindDescriptorSets(unwrap(cmd), VK_PIPELINE_BIND_POINT_GRAPHICS, layout_, 0, 1, &set, 0, nullptr);
}

GraphicsPipeline::~GraphicsPipeline() {
    vkDestroyPipeline(ctx_.device(), handle_, nullptr);
    vkDestroyPipelineLayout(ctx_.device(), layout_, nullptr);
}

}
