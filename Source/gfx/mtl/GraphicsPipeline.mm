#include "gfx/mtl/GraphicsPipeline.hpp"

#include "gfx/mtl/Bindings.hpp"
#include "gfx/mtl/Commands.hpp"
#include "gfx/mtl/MetalUtil.hpp"

#include <cstring>
#include <stdexcept>

#include <glm/gtc/type_ptr.hpp>

namespace cinder::gfx::rhi {

GraphicsPipeline::GraphicsPipeline(void* pipeline, void* depthState,
                                   std::uint32_t pushConstantBytes,
                                   ShaderStages pushConstantStages, Winding winding)
    : pipeline_(pipeline), depthState_(depthState),
      pushConstantBytes_((pushConstantBytes + 15u) & ~15u),
      pushConstantStages_(pushConstantStages), winding_(winding) {}

void GraphicsPipeline::bind(Commands cmd) const {
    auto& state = unwrap(cmd);
    id<MTLRenderCommandEncoder> encoder =
            cinder::gfx::mtl::bridge<id<MTLRenderCommandEncoder>>(state.encoder);
    [encoder setRenderPipelineState:
                     cinder::gfx::mtl::bridge<id<MTLRenderPipelineState>>(pipeline_)];
    if (depthState_ != nullptr) {
        [encoder setDepthStencilState:
                         cinder::gfx::mtl::bridge<id<MTLDepthStencilState>>(depthState_)];
    }
    [encoder setFrontFacingWinding:winding_ == Winding::Clockwise
                                          ? MTLWindingClockwise
                                          : MTLWindingCounterClockwise];
    state.pushConstantBytes = pushConstantBytes_;
    if (pushConstantBytes_ > state.pushConstants.size()) {
        throw std::runtime_error("Metal push constant block exceeds 4096 bytes");
    }
    std::memset(state.pushConstants.data(), 0, pushConstantBytes_);
}

void GraphicsPipeline::updatePushConstants(Commands cmd, ShaderStages stages) const {
    if (pushConstantBytes_ == 0) return;
    auto& state = unwrap(cmd);
    id<MTLRenderCommandEncoder> encoder =
            cinder::gfx::mtl::bridge<id<MTLRenderCommandEncoder>>(state.encoder);
    if (stages == ShaderStages::Vertex || stages == ShaderStages::Both) {
        [encoder setVertexBytes:state.pushConstants.data() length:pushConstantBytes_ atIndex:0];
    }
    if (stages == ShaderStages::Fragment || stages == ShaderStages::Both) {
        [encoder setFragmentBytes:state.pushConstants.data() length:pushConstantBytes_ atIndex:0];
    }
}

void GraphicsPipeline::push(Commands cmd, std::uint32_t offset, const glm::mat4& value) const {
    auto& state = unwrap(cmd);
    if (offset + MATRIX_BYTES > state.pushConstantBytes) {
        throw std::runtime_error("Metal push constant write is out of range");
    }
    std::memcpy(state.pushConstants.data() + offset, glm::value_ptr(value), MATRIX_BYTES);
    updatePushConstants(cmd, pushConstantStages_);
}

void GraphicsPipeline::push(Commands cmd, ShaderStages stages, std::uint32_t size,
                            const void* data) const {
    auto& state = unwrap(cmd);
    if (size > state.pushConstantBytes) {
        throw std::runtime_error("Metal push constant write is out of range");
    }
    std::memcpy(state.pushConstants.data(), data, size);
    updatePushConstants(cmd, stages);
}

void GraphicsPipeline::bindTexture(Commands cmd, TextureBinding texture) const {
    auto* value = unwrap(texture);
    if (value == nullptr) return;
    auto& state = unwrap(cmd);
    id<MTLRenderCommandEncoder> encoder =
            cinder::gfx::mtl::bridge<id<MTLRenderCommandEncoder>>(state.encoder);
    [encoder setFragmentTexture:cinder::gfx::mtl::bridge<id<MTLTexture>>(value->texture)
                        atIndex:0];
    [encoder setFragmentSamplerState:
                     cinder::gfx::mtl::bridge<id<MTLSamplerState>>(value->sampler) atIndex:0];
}

GraphicsPipeline::~GraphicsPipeline() {
    cinder::gfx::mtl::release(depthState_);
    cinder::gfx::mtl::release(pipeline_);
}

}
