---
type: concept
system: "[[Build and shaders]]"
source: "cmake/Shaders.cmake, engine/shaders/"
---

Shaders are HLSL, one `engine/shaders/<name>.hlsl` per program with `VSMain` and `PSMain`. `GraphicsPipelineBuilder::shader` takes the program name.

**DXC is the one frontend; the target picks the output.** `CINDER_SHADER_FORMATS` defaults to `spirv;msl` on macOS, `spirv;dxil` on Windows, `spirv` on Linux. SPIR-V is always built.

| Format | How |
|---|---|
| SPIR-V | `dxc -spirv` → `<name>.vs.spv` / `.ps.spv` |
| DXIL | `dxc` → `.vs.dxil` / `.ps.dxil` — unsigned off Windows, a syntax check only |
| MSL | `spirv-cross --msl` on the SPIR-V → `.metal`, then `xcrun metal`/`metallib` → one `<name>.metallib` + `.json` reflection |

- `--msl-decoration-binding` keeps MSL indices equal to SPIR-V bindings; `--rename-entry-point` keeps `VSMain`/`PSMain`.
- One source serves every target: `[[vk::binding]]`/`[[vk::location]]` **alongside** `register(...)`. Textures are `[[vk::combinedImageSampler]]` pairs. Push constants are `[[vk::push_constant]] ConstantBuffer<Push>`.
- Matrices stay `column_major` (glm's layout), applied as `mul(matrix, vector)`.
- Output goes to `${CMAKE_BINARY_DIR}/shaders/`, gitignored. `ctest`'s `shaders_are_valid` checks every artifact and runs `spirv-val` when present.
- No Metal Toolchain → the build emits `.metal` and skips metallib; [[Metal backend]] compiles the source at runtime.
