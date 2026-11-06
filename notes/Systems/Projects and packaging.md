---
type: system
status: live
layer: "core, dev"
source: "src/core/ProjectConfig.hpp, cmake/PackageGame.cmake, src/dev/Packager.hpp"
started: 2026-09-11
last-update: 2026-09-26
---

## About

**What it is:** a game is a project folder in Unreal's shape, opened by `editor` and played by `player`. `samples/` holds three: `sandbox2d`, `sandbox3d`, `physics`. Nothing in them is compiled.

**Play a project:**
```sh
./build/editor samples/sandbox3d --frames 2 && ./build/player samples/sandbox3d
```
`player` takes the folder or its `.cinder` file, `--scene`, `--frames`, `--capture`. With no argument it looks for `project/` next to itself (the packaged layout).

**Package:** `./build/packager samples/sandbox3d --config shipping` → `Saved/Builds/<Platform>/`. Or the editor's Platforms menu.

## Code
[[Project folder]], [[Project descriptor]], [[Game.ini]], [[packager]], [[Editor packager]]

## Tasks
- [ ] Asset bundling into an archive, not loose files
- [ ] Crash handler + log file
- [ ] Half the descriptor (`modules`, `plugins`, ...) is read by nothing
- [ ] Packaged player finds itself through `argv[0]`

## Notes
- 2026-09-26: Editor packager landed (branch `option-platform`). Only the host desktop platform is enabled.
