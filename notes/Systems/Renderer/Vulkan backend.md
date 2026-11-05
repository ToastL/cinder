---
type: module
system: "[[Renderer]]"
source: "src/gfx/vk/"
---

The default backend off macOS (`-DCINDER_BACKEND=vk`). VMA for memory, volk for loading. Files: `Ctx`, `Swapchain`, `FrameSync`, `Presenter`, `RenderTarget`, `DepthBuffer`, `Texture`, `TexturePool`, `GpuBuffer`, `GraphicsPipeline`, `PipelineFactory`, `GlyphPages`, `Vk*` helpers.

**Conventions:**
- every call goes through `vk(result, "vkFunctionName")` from `VkUtil.hpp`
- structs zero-initialized (`VkFooCreateInfo info{};`), **every count set by hand**
- buffers and images through VMA; `GpuBuffer` is always host-visible and persistently mapped
- every Vulkan-owning type is move-only with a destructor and no `destroy()`. The context is declared **first** in its owner and destroyed **last**; holders keep a reference, never a copy. Declaration order *is* the destruction contract.

`RenderTarget` has its own sampler, 1-set pool and set — not `Assets`' pool, which has no `FREE_DESCRIPTOR_SET` and would leak a set per resize. See [[Frame structure]], [[Vulkan on Apple Silicon]].
