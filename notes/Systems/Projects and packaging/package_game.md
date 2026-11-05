---
type: tool
system: "[[Projects and packaging]]"
source: "cmake/PackageGame.cmake"
---

`cmake --build build --target package_game` builds `player` and runs `cmake/PackageGame.cmake`, staging `build/dist/<name>/` (`<name>` = the `.cinder` stem):

- `player`, the compiled shaders, `engine/lua/*.lua`, `engine/fonts/`
- `project/` with the `.cinder`, `Config/`, `Content/`, `Source/` — never `Saved/` or anything else

`CINDER_PACKAGE_PROJECT` picks the project (default `samples/sandbox2d`).

- Reads the descriptor with CMake's `string(JSON)`. Refuses a project whose `targetPlatforms` doesn't list the host; runs the host's `preBuildSteps` before staging and `postBuildSteps` after, through `sh -c` (`cmd /c` on Windows) in the project folder, with `$(ProjectDir)`, `$(EngineDir)`, `$(StageDir)` expanded. A failing step stops the package.
- **`targetPlatforms` picks the shader formats staged:** macOS `*.spv` + `*.metallib`, iOS `*.metallib`, Linux/Android `*.spv`, Windows `*.dxil`, none named → all three. A missing format warns.
