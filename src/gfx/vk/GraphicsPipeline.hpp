#pragma once

#include "gfx/rhi/Format.hpp"
#include "gfx/rhi/Handles.hpp"

#include <volk.h>

#include <glm/mat4x4.hpp>

namespace cinder::gfx::vk { class VkCtx; }

namespace cinder::gfx::rhi {

class GraphicsPipeline {
public:
    static constexpr uint32_t MATRIX_BYTES = 16 * sizeof(float);

    GraphicsPipeline(const cinder::gfx::vk::VkCtx& ctx, VkPipelineLayout layout,
                     VkPipeline handle);
    ~GraphicsPipeline();

    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

    void bind(Commands cmd) const;
    void push(Commands cmd, uint32_t offset, const glm::mat4& value) const;
    void push(Commands cmd, ShaderStages stages, uint32_t size, const void* data) const;
    void bindTexture(Commands cmd, TextureBinding texture) const;


private:
    const cinder::gfx::vk::VkCtx& ctx_;
    VkPipelineLayout layout_;
    VkPipeline handle_;
};

}
