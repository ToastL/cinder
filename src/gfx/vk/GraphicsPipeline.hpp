#pragma once

#include <volk.h>

#include <glm/mat4x4.hpp>

namespace cinder::gfx::vk {

class VkCtx;

class GraphicsPipeline {
public:
    static constexpr uint32_t MATRIX_BYTES = 16 * sizeof(float);

    GraphicsPipeline(const VkCtx& ctx, VkPipelineLayout layout, VkPipeline handle);
    ~GraphicsPipeline();

    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

    void bind(VkCommandBuffer cmd) const;
    void push(VkCommandBuffer cmd, uint32_t offset, const glm::mat4& value) const;
    void bindDescriptorSet(VkCommandBuffer cmd, VkDescriptorSet set) const;

    VkPipelineLayout layout() const { return layout_; }

private:
    const VkCtx& ctx_;
    VkPipelineLayout layout_;
    VkPipeline handle_;
};

}
