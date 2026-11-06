#include "gfx/rhi/Pipeline.hpp"

#include "gfx/mtl/Ctx.hpp"
#include "gfx/mtl/Formats.hpp"
#include "gfx/mtl/GraphicsPipeline.hpp"
#include "gfx/mtl/MetalUtil.hpp"
#include "gfx/mtl/Presenter.hpp"
#include "gfx/rhi/PipelineBuilder.hpp"
#include "platform/Assets.hpp"
#include "platform/Files.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace cinder::gfx::rhi {
namespace {

id<MTLLibrary> sourceLibrary(id<MTLDevice> device, const std::string& program,
                             const char* stage) {
    const std::filesystem::path path =
            cinder::platform::shaderPath(program + "." + stage + ".metal");
    const std::string source = cinder::platform::readTextFile(path);
    NSError* error = nil;
    MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
    id<MTLLibrary> library = [device
            newLibraryWithSource:[NSString stringWithUTF8String:source.c_str()]
                         options:options error:&error];
    if (library == nil) throw cinder::gfx::mtl::error("Could not compile Metal shader", error);
    return library;
}

id<MTLLibrary> library(id<MTLDevice> device, const std::string& program,
                       const char* stage) {
    const std::filesystem::path path = cinder::platform::shaderPath(program + ".metallib");
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
    id<MTLDevice> device = cinder::gfx::mtl::bridge<id<MTLDevice>>(presenter.ctx().device());
    id<MTLLibrary> vertexLibrary = library(device, desc.program, "vs");
    id<MTLLibrary> fragmentLibrary = library(device, desc.program, "ps");
    id<MTLFunction> vertex = [vertexLibrary newFunctionWithName:@"VSMain"];
    id<MTLFunction> fragment = [fragmentLibrary newFunctionWithName:@"PSMain"];
    if (vertex == nil || fragment == nil) {
        throw cinder::gfx::mtl::error("Metal shader is missing VSMain or PSMain");
    }

    MTLRenderPipelineDescriptor* pipeline = [[MTLRenderPipelineDescriptor alloc] init];
    pipeline.vertexFunction = vertex;
    pipeline.fragmentFunction = fragment;
    pipeline.colorAttachments[0].pixelFormat = static_cast<MTLPixelFormat>(
            cinder::gfx::mtl::pixelFormat(presenter.colorFormat()));

    if (desc.vertexStride > 0) {
        MTLVertexDescriptor* vertices = [[MTLVertexDescriptor alloc] init];
        vertices.layouts[1].stride = desc.vertexStride;
        vertices.layouts[1].stepFunction = MTLVertexStepFunctionPerVertex;
        for (const VertexAttribute& attribute : desc.attributes) {
            vertices.attributes[attribute.location].format = static_cast<MTLVertexFormat>(
                    cinder::gfx::mtl::vertexFormat(attribute.format));
            vertices.attributes[attribute.location].offset = attribute.offset;
            vertices.attributes[attribute.location].bufferIndex = 1;
        }
        pipeline.vertexDescriptor = vertices;
    }

    if (desc.pass == PassKind::Scene && (desc.depthTest || desc.depthWrite)) {
        pipeline.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
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
    if (state == nil) throw cinder::gfx::mtl::error("Could not create Metal pipeline", error);

    void* depthHandle = nullptr;
    if (desc.depthTest || desc.depthWrite) {
        MTLDepthStencilDescriptor* depth = [[MTLDepthStencilDescriptor alloc] init];
        depth.depthCompareFunction = desc.depthTest ? MTLCompareFunctionLess
                                                    : MTLCompareFunctionAlways;
        depth.depthWriteEnabled = desc.depthWrite;
        id<MTLDepthStencilState> depthState = [device newDepthStencilStateWithDescriptor:depth];
        depthHandle = cinder::gfx::mtl::retain(depthState);
    }

    return std::make_unique<GraphicsPipeline>(
            cinder::gfx::mtl::retain(state), depthHandle, desc.pushConstantBytes,
            desc.pushConstantStages, desc.winding);
}

std::unique_ptr<GraphicsPipeline> PipelineBuilder::build() {
    return createPipeline(presenter_, desc_);
}

}
