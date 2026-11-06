---
type: system
status: live
layer: "—"
source: "Setup.sh, Source/Programs/CinderBuildTool/"
started: 2026-09-09
last-update: 2026-11-06
---

## About

**What it is:** the build. One tool, [[cbt]] (CinderBuildTool), builds the engine, the executables, the shaders and game code, from rules written in C++ next to each module — UnrealBuildTool's model. There is no CMake. See [[Decision - Own build tool]].

**Build:**
```sh
./Setup.sh                                   # bootstraps Binaries/macOS/cbt
./Binaries/macOS/cbt build                   # editor (+ player, packager), ui_gallery, tests
./Binaries/macOS/cbt build --backend vk --config Debug
./Binaries/macOS/cbt test
./Binaries/macOS/cbt generate xcode          # Cinder.xcodeproj that only calls cbt
```
Output: `Binaries/<Platform>/<Config>/` and `Intermediate/<Platform>/<Config>/`. `--backend` is `mtl` by default on macOS and `vk` elsewhere. Nothing is downloaded.

**Build tools (not vendored):** `dxc` (LunarG Vulkan SDK — not on Homebrew), `spirv-cross` (`brew install spirv-cross`), and the Metal Toolchain (`xcodebuild -downloadComponent MetalToolchain`), invoked through `xcrun`.

## Code
[[cbt]], [[Build targets]], [[Dependencies]], [[Shader pipeline]], [[Layer graph]]

## Tasks
- [ ] Shader hot reload (shaders compile at build time; editing one needs a rebuild)
- [ ] Remember `--backend` between runs (`Intermediate/BuildSettings.ini`)
- [ ] Build on Linux — the rules have the X11 cases, nothing has run there

## Notes
- The user builds their own `Binaries/`; verify with `--output <scratch>`. See [[Verification recipes]].
- 2026-09-23: Shaders moved from GLSL to HLSL, one DXC frontend for every target.
- 2026-11-06: CMake replaced by [[cbt]], all 7 phases; the repo root took a project's shape (`Source/`, `Content/`, `Shaders/`, `Samples/`, `Tests/`). Verified on mtl and vk: `cbt test` 6/6, `graphics_smoke` and selftest ALL PASS, gallery capture byte-identical to CMake's.
