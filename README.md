# Cinder

A C++20 game engine with Vulkan and Metal rendering, Lua scripting, a retained UI
framework, and a separate editor. Games live in project folders containing a
`.cinder` descriptor, `Config/`, `Content/`, and `Source/`.

## Build and run

Use CMake 3.24 or newer, Ninja, and a C++20 compiler. Dependencies are fetched by
CMake. Shader compilation requires `dxc`; the default macOS shader configuration
also requires `spirv-cross`. Xcode's Metal toolchain compiles `.metallib` files when
available; otherwise the Metal backend can load the generated `.metal` source.

```sh
cmake -S . -B build -G Ninja
cmake --build build
./build/editor samples/sandbox2d
```

Metal is the default backend on macOS; Vulkan is the default elsewhere. Select one
explicitly with `-DCINDER_BACKEND=mtl` or `-DCINDER_BACKEND=vk`. Metal requires macOS;
Vulkan needs an installed Vulkan loader and driver. `CINDER_SHADER_FORMATS` controls
shader outputs independently of the selected backend.

The player is built on demand and never links the editor:

```sh
cmake --build build --target player
./build/player samples/sandbox3d
cmake --build build --target package_game
```

`package_game` stages the player, engine assets, shaders, and the project selected
by `CINDER_PACKAGE_PROJECT` under `build/dist/`.

## Source map

| Area | Responsibility |
| --- | --- |
| `platform` | Windows, input, files, asset paths, logging, GLFW lifetime |
| `reflect`, `scene`, `components` | Property metadata, node trees, transforms, built-in scene nodes |
| `serial` | Scene, JSON, and INI encoding |
| `lua`, `script` | Lua primitives, scene bindings, script execution |
| `physics` | Geometry, collision detection, contact solving, simulation |
| `text` | Font loading, shaping, text layout, CPU glyph atlases |
| `ui/core`, `ui/widgets`, `ui/docking` | Widget primitives, controls, dock layouts |
| `ui/framework` | Application lifecycle, input routing, popups, drag and drop |
| `gfx/rhi`, `gfx/vk`, `gfx/mtl` | Rendering interface and backend implementations |
| `gfx/asset`, `gfx/pass`, `gfx` | GPU assets, scene passes, rendering orchestration |
| `core` | Engine orchestration, game loop, project configuration, launch options |
| `dev` | Editor application, panels, play sessions, selection, undo, manipulators |
| `editor`, `player` | Executable entry points |

`engine` owns runtime code; `engine_dev` adds editor code. The allowed include graph
is defined in `cmake/AssertLayers.cmake` and checked for C++, headers, and
Objective-C++ sources.

The UI application implementation is grouped by responsibility:
`Application.cpp` owns activation, frame dispatch, and painting;
`ApplicationInput.cpp` owns focus and event routing;
`ApplicationPopups.cpp` owns popups and tooltips;
`ApplicationDragDrop.cpp` owns drag sessions. Their private helpers live in
`ApplicationInternals.hpp`; clients include `Application.hpp`.

Executable arguments are parsed by `core/LaunchOptions`, with a separate option
set for the editor, player, and gallery. Parsing retains the existing permissive
behavior, including ignored unknown flags and numeric prefixes. `dev/runEditor`
composes the editor and runs its frame loop. `platform/GlfwSession` balances GLFW
acquisition and release on scope exit, including exceptions; a `Window` owns its
own session so it can also be used independently.

Build configuration is separated into `cmake/Backend.cmake` (selection and generated
backend header), `Dependencies.cmake`, `Shaders.cmake`, `Targets.cmake`,
`Packaging.cmake`, and `Testing.cmake`. The root `CMakeLists.txt` assembles them.

## Verification

```sh
ctest --test-dir build --output-on-failure
cmake --build build --target graphics_smoke
./build/graphics_smoke
```

CTest runs the headless unit suite, include-layer checks and their fixtures, shader
validation, and a check that the shipping player contains no editor code. It builds
the player as a test fixture. `graphics_smoke` requires a window and GPU; it drives
editor selection, transforms, undo/redo, play transitions, docking, resizing, and
captures into the build directory.

`ui_gallery` is an interactive widget test bench. Both the gallery and editor
accept `--frames`, `--capture-window`, and `--input-script` for reproducible runs.
Editor and player accept `--scene` and `--capture`; the latter captures the scene
render target. The editor also accepts `--play`, and the gallery accepts `--lowdpi`
and `--text-gamma`.

See [TODO.md](TODO.md) for the roadmap and deliberate limitations, and
[CLAUDE.md](CLAUDE.md) for detailed subsystem contracts and development workflows.
