#pragma once

#include "gfx/rhi/Format.hpp"
#include "gfx/rhi/Fwd.hpp"
#include "gfx/rhi/Pipeline.hpp"

#include <memory>
#include <string>

namespace cinder::gfx::rhi {

class PipelineBuilder {
public:
    PipelineBuilder(const Presenter& presenter, PassKind pass);

    PipelineBuilder& shader(std::string program);
    PipelineBuilder& pushConstants(std::uint32_t bytes, ShaderStages stages = ShaderStages::Vertex);
    PipelineBuilder& vertexStride(std::uint32_t stride);
    PipelineBuilder& attribute(std::uint32_t location, VertexFormat format, std::uint32_t offset);
    PipelineBuilder& depthTest();
    PipelineBuilder& depthRead();
    PipelineBuilder& alphaBlend();
    PipelineBuilder& premultipliedBlend();
    PipelineBuilder& frontFace(Winding winding);

    std::unique_ptr<GraphicsPipeline> build();

private:
    const Presenter& presenter_;
    PipelineDesc desc_;
};

}
