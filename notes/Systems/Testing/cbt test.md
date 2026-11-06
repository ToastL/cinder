---
type: tool
system: "[[Testing]]"
source: "Source/Programs/CinderBuildTool/Tests.cpp, Tests/"
---

`cbt test [checks]` builds `editor` and `tests`, then runs, each reported passed or FAILED:
- **`layers`** / **`layer_fixtures`** — the [[Layer graph]] over `Source/`, and the 25 allowed/forbidden include fixtures.
- **`shaders`** — every shader artifact present, SPIR-V magic, `spirv-val`. See [[Shader pipeline]].
- **`shipping`** — `nm -a player`: no `cinder::dev` symbols.
- **`unit`** — doctest cases: scene, transform, reflect, serial (codec, text, INI, JSON), scene files, project, assets, log, launch options, loop, game modules, physics, picking, editor camera, gizmo lines, manipulator, history, editor panels, packager, gfx formats, view camera, text, text layout, text field, UI (batcher, dock, editors, input, layout, popup, tree), input events.
- **`hot_reload`** — compiles a real game library through `cbt game`, swaps it, and packages one as Shipping. Takes seconds.

`scene_files_test` re-saves every `.scene`, `Game.ini` and descriptor under `Samples/` and `Tests/selftest/` byte-identically. `physics_test` pins stepping bit for bit. `GameLoop::advance()`/`alpha()` are public so `loop_test` drives the loop without a window.
