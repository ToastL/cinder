#include "Gfx/MTL/GraphicsPipeline.hpp"

#include "Gfx/MTL/Bindings.hpp"
#include "Gfx/MTL/Commands.hpp"
#include "Gfx/MTL/MetalUtil.hpp"

#include <cstring>
#include <stdexcept>
#include <string>

#include <glm/gtc/type_ptr.hpp>

namespace cinder::Gfx::RHI {

GraphicsPipeline::GraphicsPipeline(void* pipeline, void* depthState,
                                   std::uint32_t pushConstantBytes,
                                   ShaderStages pushConstantStages, Winding winding, bool cullBack)
    : pipeline_(pipeline), depthState_(depthState),
      pushConstantBytes_((pushConstantBytes + 15u) & ~15u),
      pushConstantStages_(pushConstantStages), winding_(winding), cullBack_(cullBack) {}

void GraphicsPipeline::bind(Commands cmd) const {
    auto& state = unwrap(cmd);
    id<MTLRenderCommandEncoder> encoder =
            cinder::Gfx::MTL::bridge<id<MTLRenderCommandEncoder>>(state.encoder);
    [encoder setRenderPipelineState:
                     cinder::Gfx::MTL::bridge<id<MTLRenderPipelineState>>(pipeline_)];
    if (depthState_ != nullptr) {
        [encoder setDepthStencilState:
                         cinder::Gfx::MTL::bridge<id<MTLDepthStencilState>>(depthState_)];
    }
    [encoder setFrontFacingWinding:winding_ == Winding::Clockwise
                                          ? MTLWindingClockwise
                                          : MTLWindingCounterClockwise];
    [encoder setCullMode:cullBack_ ? MTLCullModeBack : MTLCullModeNone];
    state.pushConstantBytes = pushConstantBytes_;
    if (pushConstantBytes_ > state.pushConstants.size()) {
        throw std::runtime_error("Metal push constant block exceeds " + std::to_string(state.pushConstants.size()) + " bytes");
    }
    std::memset(state.pushConstants.data(), 0, pushConstantBytes_);
}

void GraphicsPipeline::updatePushConstants(Commands cmd, ShaderStages stages) const {
    if (pushConstantBytes_ == 0) return;
    auto& state = unwrap(cmd);
    id<MTLRenderCommandEncoder> encoder =
            cinder::Gfx::MTL::bridge<id<MTLRenderCommandEncoder>>(state.encoder);
    if (stages == ShaderStages::Vertex || stages == ShaderStages::Both) {
        [encoder setVertexBytes:state.pushConstants.data()
                         length:pushConstantBytes_
                        atIndex:cinder::Gfx::MTL::PUSH_CONSTANT_SLOT];
    }
    if (stages == ShaderStages::Fragment || stages == ShaderStages::Both) {
        [encoder setFragmentBytes:state.pushConstants.data()
                           length:pushConstantBytes_
                          atIndex:cinder::Gfx::MTL::PUSH_CONSTANT_SLOT];
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
            cinder::Gfx::MTL::bridge<id<MTLRenderCommandEncoder>>(state.encoder);
    [encoder setFragmentTexture:cinder::Gfx::MTL::bridge<id<MTLTexture>>(value->texture)
                        atIndex:cinder::Gfx::MTL::TEXTURE_SLOT];
    [encoder setFragmentSamplerState:
                     cinder::Gfx::MTL::bridge<id<MTLSamplerState>>(value->sampler)
                                 atIndex:cinder::Gfx::MTL::TEXTURE_SLOT];
}

GraphicsPipeline::~GraphicsPipeline() {
    cinder::Gfx::MTL::release(depthState_);
    cinder::Gfx::MTL::release(pipeline_);
}

}
