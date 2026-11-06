---
type: tool
system: "[[Projects and packaging]]"
source: "src/packager/main.cpp, src/dev/Staging.cpp"
---

`packager <project> [--platform macOS] [--config development|shipping] [--out dir]` stages a runnable game. The editor's Platforms menu runs the same binary. Defaults: the host platform, Development, and `<project>/Saved/Builds/<Platform>/<name>/`.

`dev/Staging` does the work:
1. loads the project's descriptor through `ProjectConfig` and refuses a platform not in `targetPlatforms`;
2. runs `preBuildSteps` through `sh -c` in the project folder, with `$(ProjectDir)`, `$(EngineDir)` and `$(StageDir)` expanded;
3. builds the project's C++ with `ModuleBuilder` into `Saved/Binaries/Package-<Config>/` (`-O2`);
4. copies `player`, the shader formats the targets need, `engine/fonts/`, and `project/` with the `.cinder`, `Config/`, `Content/` and `Binaries/<Platform>/<name>.dylib`. It never copies `Source/` or `Saved/`;
5. for Shipping, keeps the unstripped library in `Saved/Symbols/<Platform>/` and strips the shipped one;
6. runs `postBuildSteps`.

`player` is built together with the editor and copied, never compiled at package time. See [[Editor packager]].
