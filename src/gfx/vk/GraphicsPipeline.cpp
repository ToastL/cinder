#include "gfx/vk/GraphicsPipeline.hpp"

#include "gfx/vk/Bindings.hpp"
#include "gfx/vk/Formats.hpp"
#include "gfx/vk/Commands.hpp"
#include "gfx/vk/VkCtx.hpp"

#include <glm/gtc/type_ptr.hpp>

namespace cinder::gfx::rhi {

GraphicsPipeline::GraphicsPipeline(const cinder::gfx::vk::VkCtx& ctx, VkPipelineLayout layout,
                                   VkPipeline handle)
    : ctx_(ctx), layout_(layout), handle_(handle) {}

void GraphicsPipeline::bind(Commands cmd) const {
    vkCmdBindPipeline(unwrap(cmd), VK_PIPELINE_BIND_POINT_GRAPHICS, handle_);
}

void GraphicsPipeline::push(Commands cmd, uint32_t offset, const glm::mat4& value) const {
    vkCmdPushConstants(unwrap(cmd), layout_, VK_SHADER_STAGE_VERTEX_BIT, offset, MATRIX_BYTES,
                       glm::value_ptr(value));
}

void GraphicsPipeline::push(Commands cmd, ShaderStages stages, uint32_t size,
                            const void* data) const {
    vkCmdPushConstants(unwrap(cmd), layout_, toVk(stages), 0, size, data);
}

void GraphicsPipeline::bindTexture(Commands cmd, TextureBinding texture) const {
    const VkDescriptorSet set = unwrap(texture);
    vkCmdBindDescriptorSets(unwrap(cmd), VK_PIPELINE_BIND_POINT_GRAPHICS, layout_, 0, 1, &set, 0, nullptr);
}

GraphicsPipeline::~GraphicsPipeline() {
    vkDestroyPipeline(ctx_.device(), handle_, nullptr);
    vkDestroyPipelineLayout(ctx_.device(), layout_, nullptr);
}

}
