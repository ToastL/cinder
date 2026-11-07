#include "Gfx/RHI/Pipeline.hpp"
#include "Gfx/RHI/PipelineBuilder.hpp"

#include "Gfx/VK/Formats.hpp"
#include "Gfx/VK/GraphicsPipeline.hpp"
#include "Gfx/VK/Presenter.hpp"
#include "Gfx/VK/Shaders.hpp"
#include "Gfx/VK/Ctx.hpp"
#include "Gfx/VK/VkUtil.hpp"

#include <utility>
#include <vector>

namespace cinder::Gfx::RHI {

using cinder::Gfx::VK::check;
namespace shaders = cinder::Gfx::VK::shaders;

namespace {

class ModuleGuard {
public:
    ModuleGuard(VkDevice device, VkShaderModule module) : device_(device), module_(module) {}
    ~ModuleGuard() { vkDestroyShaderModule(device_, module_, nullptr); }
    ModuleGuard(const ModuleGuard&) = delete;
    ModuleGuard& operator=(const ModuleGuard&) = delete;
    VkShaderModule get() const { return module_; }

private:
    VkDevice device_;
    VkShaderModule module_;
};

class LayoutGuard {
public:
    LayoutGuard(VkDevice device, VkPipelineLayout layout) : device_(device), layout_(layout) {}
    ~LayoutGuard() {
        if (layout_ != VK_NULL_HANDLE) vkDestroyPipelineLayout(device_, layout_, nullptr);
    }
    LayoutGuard(const LayoutGuard&) = delete;
    LayoutGuard& operator=(const LayoutGuard&) = delete;
    VkPipelineLayout get() const { return layout_; }
    VkPipelineLayout release() { return std::exchange(layout_, VK_NULL_HANDLE); }

private:
    VkDevice device_;
    VkPipelineLayout layout_;
};

}

std::unique_ptr<GraphicsPipeline> createPipeline(const Presenter& presenter,
                                                 const PipelineDesc& desc) {
    const cinder::Gfx::RHI::Ctx& ctx = presenter.ctx();

    VkPushConstantRange push{};
    push.stageFlags = toVk(desc.pushConstantStages);
    push.offset = 0;
    push.size = desc.pushConstantBytes;

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    const VkDescriptorSetLayout setLayout = ctx.textureLayout();
    layoutInfo.pSetLayouts = &setLayout;
    if (desc.pushConstantBytes > 0) {
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &push;
    }

    VkPipelineLayout created = VK_NULL_HANDLE;
    check(vkCreatePipelineLayout(ctx.device(), &layoutInfo, nullptr, &created), "vkCreatePipelineLayout");
    LayoutGuard layout(ctx.device(), created);

    const ModuleGuard vertModule(ctx.device(), shaders::fromFile(ctx, desc.program + ".vs"));
    const ModuleGuard fragModule(ctx.device(), shaders::fromFile(ctx, desc.program + ".ps"));

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule.get();
    stages[0].pName = "VSMain";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule.get();
    stages[1].pName = "PSMain";

    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = desc.vertexStride;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> attributes;
    attributes.reserve(desc.attributes.size());
    for (const VertexAttribute& attribute : desc.attributes) {
        VkVertexInputAttributeDescription description{};
        description.location = attribute.location;
        description.binding = 0;
        description.format = toVk(attribute.format);
        description.offset = attribute.offset;
        attributes.push_back(description);
    }

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    if (desc.vertexStride > 0) {
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &binding;
        vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
        vertexInput.pVertexAttributeDescriptions = attributes.data();
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
    raster.cullMode = desc.cullBack ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
    raster.frontFace = toVk(desc.winding);
    raster.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.sampleShadingEnable = VK_FALSE;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = desc.depthTest ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = desc.depthWrite ? VK_TRUE : VK_FALSE;
    if (desc.depthTest) depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState attachment{};
    attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    attachment.blendEnable = desc.blend ? VK_TRUE : VK_FALSE;
    if (desc.blend) {
        attachment.srcColorBlendFactor =
                desc.premultiplied ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_SRC_ALPHA;
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
    info.layout = layout.get();
    info.renderPass = presenter.renderPass(desc.pass);
    info.subpass = 0;

    VkPipeline handle = VK_NULL_HANDLE;
    check(vkCreateGraphicsPipelines(ctx.device(), VK_NULL_HANDLE, 1, &info, nullptr, &handle),
          "vkCreateGraphicsPipelines");
    return std::make_unique<GraphicsPipeline>(ctx, layout.release(), handle);
}

std::unique_ptr<GraphicsPipeline> PipelineBuilder::build() { return createPipeline(presenter_, desc_); }

}
