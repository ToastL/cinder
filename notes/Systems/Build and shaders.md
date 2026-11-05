---
type: system
status: live
layer: "—"
source: "CMakeLists.txt, cmake/"
started: 2026-09-09
last-update: 2026-09-26
---

## About

**What it is:** the CMake build: targets, pinned dependencies, the HLSL shader pipeline, and the layer check that keeps `src/` a strict DAG.

**Build:**
```sh
cmake -S . -B build -G Ninja && cmake --build build
cmake -S . -B build-mtl -G Ninja -DCINDER_BACKEND=mtl && cmake --build build-mtl
ctest --test-dir build --output-on-failure
```
`CINDER_BACKEND` is `mtl` by default on macOS and `vk` elsewhere. The first configure needs network.

**Build tools (not fetched):** `dxc` (LunarG Vulkan SDK — not on Homebrew), `spirv-cross` (`brew install spirv-cross`), and the Metal Toolchain (`xcodebuild -downloadComponent MetalToolchain`), invoked through `xcrun`.

## Code
[[Build targets]], [[Dependencies]], [[Shader pipeline]], [[Layer graph]]

## Tasks
- [ ] Shader hot reload (shaders compile at build time; editing one needs a rebuild)

## Notes
- The user rebuilds `build/` themselves; verify in a scratch tree. See [[Verification recipes]].
- 2026-09-23: Shaders moved from GLSL to HLSL, one DXC frontend for every target.
