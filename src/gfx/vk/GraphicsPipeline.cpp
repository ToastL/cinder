#include "gfx/vk/GraphicsPipeline.hpp"

#include "gfx/vk/VkCtx.hpp"

#include <glm/gtc/type_ptr.hpp>

namespace cinder::gfx::vk {

GraphicsPipeline::GraphicsPipeline(const VkCtx& ctx, VkPipelineLayout layout, VkPipeline handle)
    : ctx_(ctx), layout_(layout), handle_(handle) {}

void GraphicsPipeline::bind(VkCommandBuffer cmd) const {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, handle_);
}

void GraphicsPipeline::push(VkCommandBuffer cmd, uint32_t offset, const glm::mat4& value) const {
    vkCmdPushConstants(cmd, layout_, VK_SHADER_STAGE_VERTEX_BIT, offset, MATRIX_BYTES,
                       glm::value_ptr(value));
}

void GraphicsPipeline::push(VkCommandBuffer cmd, VkShaderStageFlags stages, uint32_t size,
                            const void* data) const {
    vkCmdPushConstants(cmd, layout_, stages, 0, size, data);
}

void GraphicsPipeline::bindDescriptorSet(VkCommandBuffer cmd, VkDescriptorSet set) const {
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout_, 0, 1, &set, 0, nullptr);
}

GraphicsPipeline::~GraphicsPipeline() {
    vkDestroyPipeline(ctx_.device(), handle_, nullptr);
    vkDestroyPipelineLayout(ctx_.device(), layout_, nullptr);
}

}
