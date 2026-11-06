---
type: tool
system: "[[Build and shaders]]"
source: "Source/Programs/CinderBuildTool/, Setup.sh"
---

**cbt** (CinderBuildTool) is the one build tool. Commands: `build [targets]`, `test [checks]`, `check`, `game <project>`, `generate xcode|compile-commands`, `clean`, `modules`; options `--config`, `--backend`, `--shader-formats`, `--output`, `-j`.

- **Bootstrap:** `Setup.sh` compiles the tool plus every `*.Build.cpp`/`*.Target.cpp` under `Source/` and `Tests/` into `Binaries/<Platform>/cbt`, with the repo root baked in. On every start `cbt` rebuilds itself when a source, a rule file or `Setup.sh` changed, or a rule file was added or removed.
- **Rules are code:** `CINDER_MODULE(name, lambda)` beside each module's code; `CINDER_TARGET` for executables and game modules. Public, private and link dependencies, defines, include paths, frameworks; conditions on `TargetInfo` (platform, config, backend).
- **Actions:** compiles, shader steps, links, generated `Backend.hpp`/`BuildId.cpp`. Rerun when an output is missing, an input or depfile header is newer, or the command line changed (`Actions.txt`). Parallel, dependency-ordered. No static libraries: executables link module objects directly with `-export_dynamic`.
- **BuildId:** hash of `Source/` (ThirdParty included), version, backend, config and compiler → `Engine.modules` + `BuildId.cpp`; game code gets it as `CINDER_MODULE_ABI`, so stale game code recompiles. See [[Game code]].
- **Game code:** `cbt game <project>` — default module `Project.<name>`, or the project's own rules compiled into a library cbt loads; output `Saved/Binaries/<name>.dylib`.
- **Xcode:** External Build System targets running `cbt xcode <target> $(CONFIGURATION) $(ACTION)`; Debug/Development/Shipping; shared schemes; an index-only native target. `projectDirPath` must stay empty, or Xcode cannot resolve the shared schemes ("not currently configured for the build action").
