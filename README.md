# Cinder

A C++20 game engine with Vulkan and Metal rendering, C++ game code that the editor
builds and hot-reloads, a retained UI framework, and a separate editor. Games live in
project folders containing a `.cinder` descriptor, `Config/`, `Content/`, and
`Source/`, where the game's C++ lives.

## Build and run

Cinder builds with its own tool, `cbt`, which lives in `Source/Programs/CinderBuildTool/`. You need
a C++20 compiler (Xcode's on macOS). Every library is vendored in `Source/ThirdParty/`, so nothing is
downloaded. Shader compilation requires `dxc` from the Vulkan SDK; the default macOS shader formats
also require `spirv-cross`. Xcode's Metal toolchain compiles `.metallib` files when available;
otherwise the Metal backend loads the generated `.metal` source.

```sh
./Setup.sh                                   # builds Binaries/macOS/cbt
./Binaries/macOS/cbt build                   # editor, player, packager, ui_gallery, tests
./Binaries/macOS/Development/editor Samples/sandbox2d
```

Metal is the default backend on macOS; Vulkan is the default elsewhere. Pick one with
`--backend mtl` or `--backend vk`, a configuration with `--config Debug|Development|Shipping`, and
shader outputs with `--shader-formats`, independently of the backend. Output goes to
`Binaries/<Platform>/<Config>/` and `Intermediate/`.

Building the editor also builds `player` and `packager`. The editor compiles a project's C++ through
`cbt game` when it opens it; the player loads that library and never links the editor:

```sh
B=Binaries/macOS/Development
$B/editor Samples/sandbox3d --frames 2
$B/player Samples/sandbox3d
$B/packager Samples/sandbox3d --config shipping
```

`packager` stages the player, engine data, shaders, the project's content and its compiled game
library under `<project>/Saved/Builds/<Platform>/`. The editor's Platforms menu runs the same binary.

For Xcode, run `./Binaries/macOS/cbt generate xcode` and open `Cinder.xcodeproj`: its targets call
`cbt`, its schemes run the binaries, and an index-only target gives Xcode the include paths. For
clangd, run `./Binaries/macOS/cbt generate compile-commands`.

## Source map

Every area is a folder under `Source/`, the include root.

| Area | Responsibility |
| --- | --- |
| `platform` | Windows, input, files, asset paths, logging, GLFW lifetime |
| `reflect`, `scene`, `components` | Property metadata, actors and components, transforms, built-in actors and components |
| `serial` | Scene, JSON, and INI encoding |
| `physics` | Geometry, collision detection, contact solving, simulation |
| `text` | Font loading, shaping, text layout, CPU glyph atlases |
| `ui/core`, `ui/widgets`, `ui/docking` | Widget primitives, controls, dock layouts |
| `ui/framework` | Application lifecycle, input routing, popups, drag and drop |
| `gfx/rhi`, `gfx/vk`, `gfx/mtl` | Rendering interface and backend implementations |
| `gfx/asset`, `gfx/pass`, `gfx` | GPU assets, scene passes, rendering orchestration |
| `core` | Engine orchestration, game loop, project configuration, game library loading |
| `game` | The API game code includes: actor and component classes plus input, time, drawing, physics queries |
| `dev` | Editor application, panels, play sessions, selection, undo, manipulators, game code builds and hot reload, packaging |
| `editor`, `player`, `packager` | Executable entry points |
| `Programs/CinderBuildTool` | `cbt`, the build tool |
| `ThirdParty` | Vendored dependencies; versions and licences in its `README.md` |

Each module's `<Name>.Build.cpp` sits in its folder and declares its dependencies. The
allowed include graph is derived from those rules and checked for C++, headers, and
Objective-C++ sources by `cbt check`. `dev` holds the editor and never reaches the player.

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

## Verification

```sh
./Binaries/macOS/cbt test
./Binaries/macOS/cbt build graphics_smoke
./Binaries/macOS/Development/graphics_smoke
```

`cbt test` runs the headless unit suite, the include-layer check and its fixtures, shader
validation, a check that the shipping player contains no editor code, and a hot-reload test that
compiles real game C++. `graphics_smoke` requires a window and GPU; it drives editor selection,
transforms, undo/redo, play transitions, docking, resizing, and captures into `Intermediate/`.

`ui_gallery` is an interactive widget test bench. Both the gallery and editor
accept `--frames`, `--capture-window`, and `--input-script` for reproducible runs.
Editor and player accept `--scene` and `--capture`; the latter captures the scene
render target. The editor also accepts `--play`, and the gallery accepts `--lowdpi`
and `--text-gamma`.

See [TODO.md](TODO.md) for the roadmap and deliberate limitations, and
[CLAUDE.md](CLAUDE.md) for detailed subsystem contracts and development workflows.
