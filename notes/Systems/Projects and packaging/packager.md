---
type: tool
system: "[[Projects and packaging]]"
source: "Source/packager/main.cpp, Source/dev/Staging.cpp"
---

`packager <project> [--platform macOS] [--config development|shipping] [--out dir]` stages a runnable game. The editor's Platforms menu runs the same binary. Defaults: the host platform, Development, and `<project>/Saved/Builds/<Platform>/<name>/`.

`dev/Staging` does the work:
1. loads the project's descriptor through `ProjectConfig` and refuses a platform not in `targetPlatforms`;
2. runs `preBuildSteps` through `sh -c` in the project folder, with `$(ProjectDir)`, `$(EngineDir)` and `$(StageDir)` expanded;
3. builds the project's C++ with `cbt game --config <Config>` into `Saved/Binaries/Package-<Config>/`;
4. copies `player`, `Engine/Shaders/` with the formats the targets need, `Engine/Content/Fonts/`, and `Project/` with the `.cinder`, `Config/`, `Content/` and `Binaries/<Platform>/<name>.dylib`. It never copies `Source/` or `Saved/`;
5. for Shipping, keeps the unstripped `player` and library in `Saved/Symbols/<Platform>/` and strips the shipped ones;
6. runs `postBuildSteps`.

When `cbt` sits next to it, the packager first runs `cbt build player --config <Config>`, so a Shipping package ships a Shipping `player`; otherwise it copies the `player` next to itself. See [[Editor packager]].
