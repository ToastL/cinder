#include "gfx/rhi/PipelineBuilder.hpp"

#include <utility>

namespace cinder::gfx::rhi {

PipelineBuilder::PipelineBuilder(const Presenter& presenter, PassKind pass)
    : presenter_(presenter) {
    desc_.pass = pass;
}

PipelineBuilder& PipelineBuilder::shader(std::string program) {
    desc_.program = std::move(program);
    return *this;
}

PipelineBuilder& PipelineBuilder::pushConstants(std::uint32_t bytes, ShaderStages stages) {
    desc_.pushConstantBytes = bytes;
    desc_.pushConstantStages = stages;
    return *this;
}

PipelineBuilder& PipelineBuilder::vertexStride(std::uint32_t stride) {
    desc_.vertexStride = stride;
    return *this;
}

PipelineBuilder& PipelineBuilder::attribute(std::uint32_t location, VertexFormat format,
                                            std::uint32_t offset) {
    desc_.attributes.push_back(VertexAttribute{location, format, offset});
    return *this;
}

PipelineBuilder& PipelineBuilder::depthTest() {
    desc_.depthTest = true;
    desc_.depthWrite = true;
    return *this;
}

PipelineBuilder& PipelineBuilder::depthRead() {
    desc_.depthTest = true;
    desc_.depthWrite = false;
    return *this;
}

PipelineBuilder& PipelineBuilder::alphaBlend() {
    desc_.blend = true;
    return *this;
}

PipelineBuilder& PipelineBuilder::premultipliedBlend() {
    desc_.blend = true;
    desc_.premultiplied = true;
    return *this;
}

PipelineBuilder& PipelineBuilder::frontFace(Winding winding) {
    desc_.winding = winding;
    return *this;
}

}
