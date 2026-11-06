---
type: concept
system: "[[Build and shaders]]"
source: "Source/Programs/CinderBuildTool/Build.cpp, Shaders/"
---

Shaders are HLSL, one `Shaders/<name>.hlsl` per program with `VSMain` and `PSMain`. `GraphicsPipelineBuilder::shader` takes the program name.

**DXC is the one frontend; the target picks the output.** `cbt`'s `--shader-formats` defaults to `spirv,msl` on macOS and `spirv` elsewhere. SPIR-V is always built.

| Format | How |
|---|---|
| SPIR-V | `dxc -spirv` → `<name>.vs.spv` / `.ps.spv` |
| DXIL | `dxc` → `.vs.dxil` / `.ps.dxil` — unsigned off Windows, a syntax check only |
| MSL | `spirv-cross --msl` on the SPIR-V → `.metal`, then `xcrun metal`/`metallib` → one `<name>.metallib` + `.json` reflection |

- `--msl-decoration-binding` keeps MSL indices equal to SPIR-V bindings; `--rename-entry-point` keeps `VSMain`/`PSMain`.
- One source serves every target: `[[vk::binding]]`/`[[vk::location]]` **alongside** `register(...)`. Textures are `[[vk::combinedImageSampler]]` pairs. Push constants are `[[vk::push_constant]] ConstantBuffer<Push>`.
- Matrices stay `column_major` (glm's layout), applied as `mul(matrix, vector)`.
- Output goes to `Binaries/<Platform>/<Config>/Shaders/`; the Metal module cache to `Intermediate/`. The `shaders` check in [[cbt test]] checks every artifact and runs `spirv-val` when present.
- No Metal Toolchain → the build emits `.metal` and skips metallib; [[Metal backend]] compiles the source at runtime.
