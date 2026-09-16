#include "gfx/vk/GraphicsPipelineBuilder.hpp"

#include "gfx/vk/Shaders.hpp"
#include "gfx/vk/VkCtx.hpp"
#include "gfx/vk/VkUtil.hpp"

#include <utility>

namespace cinder::gfx::vk {

GraphicsPipelineBuilder::GraphicsPipelineBuilder(const VkCtx& ctx, VkRenderPass renderPass,
                                                 VkDescriptorSetLayout descriptorSetLayout)
    : ctx_(ctx), renderPass_(renderPass), descriptorSetLayout_(descriptorSetLayout) {}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::shaders(std::string vert, std::string frag) {
    vert_ = std::move(vert);
    frag_ = std::move(frag);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::pushConstants(uint32_t bytes) {
    pushConstantBytes_ = bytes;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::vertexStride(uint32_t stride) {
    vertexStride_ = stride;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::attribute(uint32_t location, VkFormat format,
                                                            uint32_t offset) {
    VkVertexInputAttributeDescription description{};
    description.location = location;
    description.binding = 0;
    description.format = format;
    description.offset = offset;
    attributes_.push_back(description);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::depthTest() {
    depth_ = true;
    depthWrite_ = true;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::depthRead() {
    depth_ = true;
    depthWrite_ = false;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::alphaBlend() {
    alphaBlend_ = true;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::frontFace(VkFrontFace face) {
    frontFace_ = face;
    return *this;
}

std::unique_ptr<GraphicsPipeline> GraphicsPipelineBuilder::build() {
    VkPushConstantRange push{};
    push.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push.offset = 0;
    push.size = pushConstantBytes_;

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &descriptorSetLayout_;
    if (pushConstantBytes_ > 0) {
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &push;
    }

    VkPipelineLayout layout = VK_NULL_HANDLE;
    check(vkCreatePipelineLayout(ctx_.device(), &layoutInfo, nullptr, &layout),
          "vkCreatePipelineLayout");

    const VkShaderModule vertModule = shaders::fromFile(ctx_, vert_);
    const VkShaderModule fragModule = shaders::fromFile(ctx_, frag_);

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName = "main";

    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = vertexStride_;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    if (vertexStride_ > 0) {
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &binding;
        vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes_.size());
        vertexInput.pVertexAttributeDescriptions = attributes_.data();
    }

    VkPipelineInputAssemblyStateCreateInfo assembly{};
    assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    assembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport{};
    viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.depthClampEnable = VK_FALSE;
    raster.rasterizerDiscardEnable = VK_FALSE;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.lineWidth = 1.0f;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = frontFace_;
    raster.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.sampleShadingEnable = VK_FALSE;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = depth_ ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = depthWrite_ ? VK_TRUE : VK_FALSE;
    if (depth_) depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState attachment{};
    attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    attachment.blendEnable = alphaBlend_ ? VK_TRUE : VK_FALSE;
    if (alphaBlend_) {
        attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachment.colorBlendOp = VK_BLEND_OP_ADD;
        attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.logicOpEnable = VK_FALSE;
    blend.attachmentCount = 1;
    blend.pAttachments = &attachment;

    const VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{};
    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates = dynamicStates;

    VkGraphicsPipelineCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.stageCount = 2;
    info.pStages = stages;
    info.pVertexInputState = &vertexInput;
    info.pInputAssemblyState = &assembly;
    info.pViewportState = &viewport;
    info.pRasterizationState = &raster;
    info.pMultisampleState = &multisample;
    info.pDepthStencilState = &depthStencil;
    info.pColorBlendState = &blend;
    info.pDynamicState = &dynamic;
    info.layout = layout;
    info.renderPass = renderPass_;
    info.subpass = 0;

    VkPipeline handle = VK_NULL_HANDLE;
    const VkResult result =
            vkCreateGraphicsPipelines(ctx_.device(), VK_NULL_HANDLE, 1, &info, nullptr, &handle);

    vkDestroyShaderModule(ctx_.device(), fragModule, nullptr);
    vkDestroyShaderModule(ctx_.device(), vertModule, nullptr);

    check(result, "vkCreateGraphicsPipelines");
    return std::make_unique<GraphicsPipeline>(ctx_, layout, handle);
}

}
