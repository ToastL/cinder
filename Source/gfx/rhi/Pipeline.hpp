#pragma once

#include "gfx/rhi/Format.hpp"
#include "gfx/rhi/Fwd.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace cinder::gfx::rhi {

struct VertexAttribute {
    std::uint32_t location = 0;
    VertexFormat format = VertexFormat::Float3;
    std::uint32_t offset = 0;
};

struct PipelineDesc {
    std::string program;
    PassKind pass = PassKind::Scene;
    std::uint32_t pushConstantBytes = 0;
    ShaderStages pushConstantStages = ShaderStages::Vertex;
    std::uint32_t vertexStride = 0;
    std::vector<VertexAttribute> attributes;
    bool depthTest = false;
    bool depthWrite = false;
    bool blend = false;
    bool premultiplied = false;
    Winding winding = Winding::CounterClockwise;
};

std::unique_ptr<GraphicsPipeline> createPipeline(const Presenter& presenter,
                                                 const PipelineDesc& desc);

}
