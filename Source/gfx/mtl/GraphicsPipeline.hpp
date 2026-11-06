#pragma once

#include "gfx/rhi/Format.hpp"
#include "gfx/rhi/Handles.hpp"

#include <cstdint>

#include <glm/mat4x4.hpp>

namespace cinder::gfx::rhi {

class GraphicsPipeline {
public:
    static constexpr std::uint32_t MATRIX_BYTES = 16 * sizeof(float);

    GraphicsPipeline(void* pipeline, void* depthState, std::uint32_t pushConstantBytes,
                     ShaderStages pushConstantStages, Winding winding);
    ~GraphicsPipeline();

    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

    void bind(Commands cmd) const;
    void push(Commands cmd, std::uint32_t offset, const glm::mat4& value) const;
    void push(Commands cmd, ShaderStages stages, std::uint32_t size, const void* data) const;
    void bindTexture(Commands cmd, TextureBinding texture) const;

private:
    void updatePushConstants(Commands cmd, ShaderStages stages) const;

    void* pipeline_ = nullptr;
    void* depthState_ = nullptr;
    std::uint32_t pushConstantBytes_ = 0;
    ShaderStages pushConstantStages_ = ShaderStages::Vertex;
    Winding winding_ = Winding::CounterClockwise;
};

}
