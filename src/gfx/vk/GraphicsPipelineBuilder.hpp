#pragma once

#include "gfx/vk/GraphicsPipeline.hpp"

#include <memory>
#include <string>
#include <vector>

namespace cinder::gfx::vk {

class VkCtx;

class GraphicsPipelineBuilder {
public:
    GraphicsPipelineBuilder(const VkCtx& ctx, VkRenderPass renderPass,
                            VkDescriptorSetLayout descriptorSetLayout);

    GraphicsPipelineBuilder& shaders(std::string vert, std::string frag);
    GraphicsPipelineBuilder& pushConstants(uint32_t bytes);
    GraphicsPipelineBuilder& vertexStride(uint32_t stride);
    GraphicsPipelineBuilder& attribute(uint32_t location, VkFormat format, uint32_t offset);
    GraphicsPipelineBuilder& depthTest();
    GraphicsPipelineBuilder& depthRead();
    GraphicsPipelineBuilder& alphaBlend();
    GraphicsPipelineBuilder& frontFace(VkFrontFace face);

    std::unique_ptr<GraphicsPipeline> build();

private:
    const VkCtx& ctx_;
    VkRenderPass renderPass_;
    VkDescriptorSetLayout descriptorSetLayout_;

    std::string vert_;
    std::string frag_;
    uint32_t pushConstantBytes_ = 0;
    uint32_t vertexStride_ = 0;
    std::vector<VkVertexInputAttributeDescription> attributes_;
    bool depth_ = false;
    bool depthWrite_ = false;
    bool alphaBlend_ = false;
    VkFrontFace frontFace_ = VK_FRONT_FACE_COUNTER_CLOCKWISE;
};

}
