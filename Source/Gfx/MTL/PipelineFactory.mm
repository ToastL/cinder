#include "Gfx/RHI/Pipeline.hpp"
#include "Gfx/RHI/PipelineBuilder.hpp"

#include "Gfx/MTL/Ctx.hpp"
#include "Gfx/MTL/Formats.hpp"
#include "Gfx/MTL/GraphicsPipeline.hpp"
#include "Gfx/MTL/MetalUtil.hpp"
#include "Gfx/MTL/Presenter.hpp"
#include "Platform/Assets.hpp"
#include "Platform/Files.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace cinder::Gfx::RHI {
namespace {

id<MTLLibrary> sourceLibrary(id<MTLDevice> device, const std::string& program,
                             const char* stage) {
    const std::filesystem::path path =
            cinder::Platform::shaderPath(program + "." + stage + ".metal");
    const std::string source = cinder::Platform::readTextFile(path);
    NSError* error = nil;
    MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
    id<MTLLibrary> library = [device
            newLibraryWithSource:[NSString stringWithUTF8String:source.c_str()]
                         options:options error:&error];
    if (library == nil) throw cinder::Gfx::MTL::error("Could not compile Metal shader", error);
    return library;
}

id<MTLLibrary> library(id<MTLDevice> device, const std::string& program,
                       const char* stage) {
    const std::filesystem::path path = cinder::Platform::shaderPath(program + ".metallib");
    NSError* error = nil;
    NSString* file = [NSString stringWithUTF8String:path.string().c_str()];
    id<MTLLibrary> loaded = [device newLibraryWithURL:[NSURL fileURLWithPath:file]
                                                error:&error];
    if (loaded != nil) return loaded;
    return sourceLibrary(device, program, stage);
}

}

std::unique_ptr<GraphicsPipeline> createPipeline(const Presenter& presenter,
                                                 const PipelineDesc& desc) {
    const cinder::Gfx::MTL::PoolScope pool;
    id<MTLDevice> device = cinder::Gfx::MTL::bridge<id<MTLDevice>>(presenter.ctx().device());
    id<MTLLibrary> vertexLibrary = library(device, desc.program, "vs");
    id<MTLLibrary> fragmentLibrary = library(device, desc.program, "ps");
    id<MTLFunction> vertex = [vertexLibrary newFunctionWithName:@"VSMain"];
    id<MTLFunction> fragment = [fragmentLibrary newFunctionWithName:@"PSMain"];
    if (vertex == nil || fragment == nil) {
        throw cinder::Gfx::MTL::error("Metal shader is missing VSMain or PSMain");
    }

    MTLRenderPipelineDescriptor* pipeline = [[MTLRenderPipelineDescriptor alloc] init];
    pipeline.vertexFunction = vertex;
    pipeline.fragmentFunction = fragment;
    pipeline.colorAttachments[0].pixelFormat = static_cast<MTLPixelFormat>(
            cinder::Gfx::MTL::pixelFormat(presenter.colorFormat()));

    if (desc.vertexStride > 0) {
        MTLVertexDescriptor* vertices = [[MTLVertexDescriptor alloc] init];
        vertices.layouts[cinder::Gfx::MTL::VERTEX_BUFFER_SLOT].stride = desc.vertexStride;
        vertices.layouts[cinder::Gfx::MTL::VERTEX_BUFFER_SLOT].stepFunction = MTLVertexStepFunctionPerVertex;
        for (const VertexAttribute& attribute : desc.attributes) {
            vertices.attributes[attribute.location].format = static_cast<MTLVertexFormat>(
                    cinder::Gfx::MTL::vertexFormat(attribute.format));
            vertices.attributes[attribute.location].offset = attribute.offset;
            vertices.attributes[attribute.location].bufferIndex = cinder::Gfx::MTL::VERTEX_BUFFER_SLOT;
        }
        pipeline.vertexDescriptor = vertices;
    }

    if (desc.pass == PassKind::Scene) {
        pipeline.depthAttachmentPixelFormat = static_cast<MTLPixelFormat>(
                cinder::Gfx::MTL::pixelFormat(presenter.ctx().depthFormat()));
    }

    MTLRenderPipelineColorAttachmentDescriptor* color = pipeline.colorAttachments[0];
    color.blendingEnabled = desc.blend;
    if (desc.blend) {
        color.rgbBlendOperation = MTLBlendOperationAdd;
        color.alphaBlendOperation = MTLBlendOperationAdd;
        color.sourceRGBBlendFactor = desc.premultiplied
                ? MTLBlendFactorOne : MTLBlendFactorSourceAlpha;
        color.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        color.sourceAlphaBlendFactor = MTLBlendFactorOne;
        color.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    }

    NSError* error = nil;
    id<MTLRenderPipelineState> state = [device newRenderPipelineStateWithDescriptor:pipeline
                                                                              error:&error];
    if (state == nil) throw cinder::Gfx::MTL::error("Could not create Metal pipeline", error);

    void* depthHandle = nullptr;
    if (desc.depthTest || desc.depthWrite) {
        MTLDepthStencilDescriptor* depth = [[MTLDepthStencilDescriptor alloc] init];
        depth.depthCompareFunction = desc.depthTest ? MTLCompareFunctionLess
                                                    : MTLCompareFunctionAlways;
        depth.depthWriteEnabled = desc.depthWrite;
        id<MTLDepthStencilState> depthState = [device newDepthStencilStateWithDescriptor:depth];
        depthHandle = cinder::Gfx::MTL::retain(depthState);
    }

    return std::make_unique<GraphicsPipeline>(
            cinder::Gfx::MTL::retain(state), depthHandle, desc.pushConstantBytes,
            desc.pushConstantStages, desc.winding, desc.cullBack);
}

std::unique_ptr<GraphicsPipeline> PipelineBuilder::build() { return createPipeline(presenter_, desc_); }

}
