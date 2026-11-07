#pragma once

#include "Gfx/RHI/Format.hpp"
#include "Gfx/RHI/Handles.hpp"

#include <volk.h>

#include <glm/mat4x4.hpp>

namespace cinder::Gfx::RHI { class Ctx; }

namespace cinder::Gfx::RHI {

class GraphicsPipeline {
public:
    static constexpr uint32_t MATRIX_BYTES = 16 * sizeof(float);

    GraphicsPipeline(const cinder::Gfx::RHI::Ctx& ctx, VkPipelineLayout layout,
                     VkPipeline handle);
    ~GraphicsPipeline();

    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

    void bind(Commands cmd) const;
    void push(Commands cmd, uint32_t offset, const glm::mat4& value) const;
    void push(Commands cmd, ShaderStages stages, uint32_t size, const void* data) const;
    void bindTexture(Commands cmd, TextureBinding texture) const;


private:
    const cinder::Gfx::RHI::Ctx& ctx_;
    VkPipelineLayout layout_;
    VkPipeline handle_;
};

}
