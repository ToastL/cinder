# CLAUDE.md

Guidance for Claude Code (claude.ai/code) when working in this repository.

## What this is

A from-scratch C++20 game engine: Vulkan and Metal renderers, GLFW windowing, and game code in C++ —
each project's `Source/` builds into a library the editor hot-reloads.
The long-term target is a Unity/Unreal-shaped editor workflow — select an actor, edit its fields, hit Play, hit
Stop, land back where you started. `TODO.md` is the authoritative roadmap; read it before proposing
architectural work, since it records what is deliberately deferred and what is out of scope.

The repo holds the engine only. A game is a **project folder** — a `.cinder` file, `Config/`,
`Content/` and `Source/` — that the editor opens and the player plays; see *Projects and packaging*.
`Samples/` holds three example projects. The repo root has a project's shape too: `Source/`,
`Content/`, `Binaries/`, `Intermediate/`.

## Commands

Everything is built by **`cbt`**, CinderBuildTool — our own build tool, shaped like UnrealBuildTool and
living in `Source/Programs/CinderBuildTool/`. There is no CMake. Bootstrap it once:

```bash
./Setup.sh
```

then build:

```bash
./Binaries/macOS/cbt build
```

That builds `editor`, `ui_gallery` and `tests` — and `player` and `packager`, because the editor target
requires them, so building the editor always leaves a matching runtime and packager next to it.
Packaging copies that `player`, the way Godot's export templates work. Output goes to
`Binaries/<Platform>/<Config>/` — the executables, `Shaders/` and `Engine.modules` — with objects and
generated files in `Intermediate/<Platform>/<Config>/`. Both are gitignored. Name targets to build only
those: `cbt build player packager`.

| Option | |
|---|---|
| `--config Debug\|Development\|Shipping` | `-O0 -g`, `-O2 -g` (the default), `-O2 -DNDEBUG` |
| `--backend mtl\|vk` | the graphics backend: `mtl` is the default on macOS, `vk` elsewhere |
| `--shader-formats spirv,msl,dxil` | defaults to `spirv,msl` on macOS and `spirv` elsewhere |
| `--output <dir>` | where `Binaries/` and `Intermediate/` go; the repo root by default |
| `-j <n>` | parallel jobs; all cores by default |

A Metal build needs no `DYLD_LIBRARY_PATH` and no Vulkan SDK at runtime, and it still compiles the
SPIR-V, because the shader formats are independent of the backend. The commands below write `B` for
`Binaries/macOS/Development`.

```bash
DYLD_LIBRARY_PATH=/opt/homebrew/lib $B/ui_gallery --frames 10 --capture-window gallery.png
```

`ui_gallery` is the test bench for the new UI, linked against the engine alone: every widget, live, on
one side of a splitter, and everything the UI renderer can draw on the other. `--capture-window` writes
the swapchain image, UI included, which `--capture` never does; `--input-script <file>` replays input
instead of the OS's (see *The UI framework*); `--lowdpi` turns off framebuffer scaling so text can be
judged at 1x on a Retina display, and `--text-gamma` tunes glyph coverage.

```bash
./Binaries/macOS/cbt test
```

`cbt test` builds `editor` and `tests` and runs six checks, reporting each as passed or failed:
`layers` checks first-party includes against the layer graph, `layer_fixtures` exercises allowed and
forbidden include fixtures, `shaders` asserts every program produced every artifact, `shipping` runs
`nm` over `player` to prove no `cinder::Dev` symbol reached it, `unit` is the doctest suite, and
`hot_reload` compiles real game C++ and takes seconds. Name checks to run only those:
`cbt test unit hot_reload`. `cbt check` runs the layer check alone.

The optional graphical integration check builds only when requested and needs a working window and
GPU:

```bash
./Binaries/macOS/cbt build graphics_smoke
DYLD_LIBRARY_PATH=/opt/homebrew/lib $B/graphics_smoke
```

It runs the editor's own UI and drives it with injected input: it clicks an actor in the Outliner, drags
a move gizmo in the Scene view, undoes and redoes with ⌘Z, saves a scene, plays and stops with ⌘P,
cycles Play/Pause/Step/Resume/Stop and a game-side quit, re-docks the console onto the Scene
stack, resizes the window and scene targets, and captures both the window and the composited view. Its
output stays in `Intermediate/macOS/<Config>/graphics-smoke/`; it never saves over a sample project.
On other platforms omit the Apple-specific library path.

```bash
DYLD_LIBRARY_PATH=/opt/homebrew/lib $B/editor Samples/sandbox2d
```

`editor` is the **dev build** — the same engine plus the editor UI, which is our own (see *The UI
framework*): one window of docked tabs, the Outliner on the left, the Scene view in the middle with
the console under it and Details on the right, over a toolbar carrying the File, Edit, Window, Play and
Platforms menus. Drag a tab to another stack's middle or edge to re-dock it, close it, and reopen it from
the Window menu. It opens a project in **Edit mode**: the scene is loaded and drawn, and no game code
runs. Play/Pause/Step/Stop are on the toolbar and on ⌘P, ⌘⇧P and ⌘⌥P; ⌘S saves the scene, and ⌘Z and
⌘⇧Z undo and redo edits. In the Scene view 1, 2 and 3 pick the move, rotate and scale gizmos. It takes
the project folder or its `.cinder` file, then `--scene <path>` (relative to `Content/`), `--play`
(start in Play), `--frames <n>`, `--capture <png>`, `--capture-window <png>` and `--input-script
<file>`; a script can also `close` the window to exercise the unsaved-changes prompt. `--frames 90
--capture out.png` runs headless-ish and writes a screenshot. See *The dev tools* below for why this
is a second executable rather than a flag.

```bash
$B/editor Samples/sandbox3d --frames 2 && $B/player Samples/sandbox3d
```

`player` is the generic runtime: it loads the project's game library and plays the scene at once. It
takes the same `--scene`, `--frames` and `--capture`; with no project argument it looks for `Project/`
next to its own executable, which is the packaged layout. It never compiles anything, so a project's
C++ must have been built — opening it in the editor does that, and so does `cbt game <project>`.

```bash
$B/packager Samples/sandbox3d --config shipping
```

Stages a runnable game in `Samples/sandbox3d/Saved/Builds/macOS/sandbox3d/` — see *Projects and
packaging*. `--platform` and `--out` pick the target and the folder.

Note that `--capture` reads back the **scene render target**, not the swapchain, so the editor's own
UI never appears in a capture; `--capture-window` is the one that shows it. That is deliberate: it is the game's picture, not the editor's. In
the editor that target is the size of the Scene panel, not the window, and in Edit mode its view is
the editor camera's — which starts as a copy of a perspective scene camera, so an unattended `--frames
--capture` still writes the game's frame. An orthographic scene starts from a perspective view framing
the same rectangle instead. See *The editor camera*.

**IDEs only call `cbt`.** `cbt generate xcode` writes `Cinder.xcodeproj` at the root: one External
Build System target per `cbt` target, each running `cbt xcode <target> $(CONFIGURATION) $(ACTION)`;
Debug, Development and Shipping configurations; shared schemes that run the binaries, the editor on
`Samples/sandbox3d`; and an `Index` target that is never built, carrying the real include paths and
defines so Xcode can index the code. Xcode compiles nothing itself. A scheme's Run uses the Debug
binary and Profile the Development one. `cbt generate compile-commands` writes `compile_commands.json`
at the root for clangd, including `cbt`'s own sources and the rule files. Both are gitignored;
regenerate them when files are added. `cbt clean` deletes one configuration's `Binaries/` and
`Intermediate/`.

The third-party code is vendored in `Source/ThirdParty/` — glfw, glm, VMA, stb, volk, Vulkan-Headers,
doctest, FreeType and HarfBuzz, each with its licence, a `.Build.cpp`, and a row in
`Source/ThirdParty/README.md` giving its version. Nothing is fetched and nothing needs installing:
FreeType is built with zlib, bzip2, PNG, Brotli and HarfBuzz switched off, so it never finds Homebrew's
copies, and HarfBuzz is compiled from its single-file `src/harfbuzz.cc`.
The shader compilers are the exception: they are *build tools*, taken from `DXC` and `SPIRV_CROSS`
when set, else found in `$VULKAN_SDK/bin`, then on `PATH`. Xcode builds with a minimal `PATH`, so
`cbt generate xcode` records the `PATH` it ran with into each target's command as `--path`;
regenerate the project after installing a tool somewhere new. Shaders are HLSL, one `Shaders/<name>.hlsl` per program holding `VSMain` and `PSMain`,
and `GraphicsPipelineBuilder::shader` takes the program name.

### How `cbt` works

- **Bootstrap.** `Setup.sh` runs one compiler command that builds the tool's sources together with
  every rule file in `Source/` and `Tests/` into `Binaries/<Platform>/cbt`, the way UnrealBuildTool
  compiles every `Build.cs` first. The repo root is baked in as `CBT_ROOT`. On every start `cbt`
  compares itself with its sources, the rule files and `Setup.sh`, and with the set of rule files on
  disk; when anything changed it reruns `Setup.sh` and restarts itself.
- **Rules are code.** A module is a folder with a `<Name>.Build.cpp` beside its code:

  ```cpp
  #include "CinderBuildTool/Rules.hpp"
  CINDER_MODULE(scene, [](cbt::ModuleRules& m, const cbt::TargetInfo&) {
      m.publicDependencies({"reflect", "platform"});
  });
  ```

  Its sources are the `.cpp`, `.mm` and `.c` files in its folder, minus nested module folders, rule
  files and anything it `exclude`s. Rules set public, private and link dependencies, include paths,
  defines, frameworks, system libraries and ObjC++ flags, and may test `TargetInfo` — the platform, the
  configuration, the backend, the version and the BuildId — which is how `Gfx/RHI` picks Metal or
  Vulkan. A public dependency is seen by everyone who depends on you, a private one by you alone, and a
  link dependency is linked in without reaching your includes — Unreal's model.
  A `<Name>.Target.cpp` (`CINDER_TARGET`) declares what to produce: an executable or a game module, its
  root modules, whether it exports its symbols, its defines, and the targets it requires.
- **The layer graph is the rules.** `cbt check` and the `layers` test derive the allowed includes from
  each module's dependencies, evaluated for both backends; `Source/ThirdParty/` and
  `Source/Programs/` are outside the graph.
- **Actions.** A build is a list of actions — one compile per source, the shader steps, the generated
  `Gfx/RHI/Backend.hpp` and `BuildId.cpp`, one link per target — run in parallel in dependency order.
  An action reruns when an output is missing, when an input or a header from its `-MMD` depfile is
  newer, or when its command line differs from the signature `Actions.txt` recorded, so changing a flag
  or a define rebuilds exactly what it touches.
- **No static libraries.** Engine modules compile once, into `Intermediate/<Platform>/<Config>/Modules/`,
  and each executable links the objects it needs directly. A target's own modules compile under
  `Targets/<target>/`, with the target's defines.

### Shaders

**DXC is the one frontend, and the target decides the backend.** `cbt` compiles each program to the
formats in `--shader-formats`; `spirv` is always built, because it is the only format the engine loads
today and MSL is derived from it.

| Format | How |
|---|---|
| SPIR-V | `dxc -spirv` -> `<name>.vs.spv` / `<name>.ps.spv` |
| DXIL | `dxc` with no `-spirv` -> `<name>.vs.dxil` / `<name>.ps.dxil` |
| MSL | `spirv-cross --msl` on the SPIR-V -> `<name>.<stage>.metal`, then `xcrun metal`/`metallib` -> one `<name>.metallib` per program holding both entry points, plus a `<name>.<stage>.json` reflection dump |

`--msl-decoration-binding` makes MSL resource indices equal the SPIR-V binding numbers and
`--rename-entry-point` keeps `VSMain`/`PSMain`, so a Metal backend reads the binding map rather than
guessing it. DXIL cannot be *signed* without `dxil.dll`, which is Windows-only, so DXIL built
anywhere else is unsigned, rejected by D3D12 outside developer mode, and a syntax check only.
`metal` and `metallib` are invoked through `xcrun` rather than resolved to absolute
paths, because the Metal Toolchain lives in a version-stamped cryptex mount that moves when it
updates; when it is absent the build emits `.metal` source and skips the metallib step instead of
failing. Install it with `xcodebuild -downloadComponent MetalToolchain`. Its module cache lives in
`Intermediate/`, never next to the shipped shaders.

One HLSL source serves every target: bindings, varyings and vertex inputs carry explicit
`[[vk::binding]]` and `[[vk::location]]` **alongside** `register(t0)`/`register(s0)`/`register(b0)`,
and DXC ignores whichever set does not apply. Textures are `[[vk::combinedImageSampler]]` pairs, so
the Vulkan descriptor layouts are combined image samplers, while D3D and Metal see the separate SRV
and sampler they want. Push constants are `[[vk::push_constant]] ConstantBuffer<Push>`, which is a
push-constant block in SPIR-V and cbuffer `b0` everywhere else. Matrices stay HLSL's default
`column_major`, which is glm's memory layout, applied as `mul(matrix, vector)`. Editing a shader
needs a rebuild, not just a restart. DXC is not on Homebrew; install the LunarG Vulkan SDK if it is
missing. `spirv-cross` is `brew install spirv-cross`.

**Compiled shaders live in `Binaries/<Platform>/<Config>/Shaders/`, not beside their HLSL.** The
`shaders` test asserts every program produced every artifact its formats call for, checks the SPIR-V
magic number, and runs `spirv-val` when it can find it.

`player`, `editor` and `ui_gallery` parse their arguments through `Core/LaunchOptions`, which rejects a
malformed number rather than reading a prefix of it. Their loops have diverged on purpose: the player
only ever calls `GameLoop::tick`, while the editor calls `PlaySession::tick`, which picks `tick`, `idle`
or `step`. `GameLoop` is the shared part.

## Vulkan on Apple Silicon

Two separate failures share one root cause: **Homebrew's `/opt/homebrew/lib` is not on the dyld
default search path, and both volk and the Vulkan loader `dlopen` by leaf name.**

- `volkInitialize()` fails because volk hardcodes `/usr/local/lib`, the Intel prefix. `Glfw::acquire`
  walks its own candidate list and falls back to `volkInitializeCustom`.
- The validation layer *enumerates* but `vkCreateInstance` returns `VK_ERROR_LAYER_NOT_PRESENT`,
  because the layer manifest's `library_path` is a bare `libVkLayer_khronos_validation.dylib`.
  `RHI::Ctx` retries without the layer and prints a notice.

To actually get validation, put the prefix on the dyld path:

```bash
DYLD_LIBRARY_PATH=/opt/homebrew/lib ./Binaries/macOS/Development/editor Samples/sandbox2d
```

That assumes a `--backend vk` build. **Do this whenever touching the renderer.** Without it you are running unvalidated — it has already
caught a missing `TRANSFER_SRC_BIT` that no test would have.

## Layout

The repo root has a project's shape and its folders are capitalized. Under `Source/`, a module's folder
is named after its namespace, with acronyms in capitals as Unreal writes them: `Gfx/RHI` is
`cinder::Gfx::RHI`, `UI/Core` is a layer of `cinder::UI`. Only `cinder` itself and helper namespaces such
as `keys`, `detail` or `images` stay lowercase, and so do target names: `cbt build editor player`.

```
cinder/
  Setup.sh          bootstraps cbt
  Content/Fonts/    Roboto and Roboto Mono, OFL-1.1, licences beside them
  Shaders/          HLSL, one program per file; compiled output goes to Binaries/
  Samples/          sandbox2d/, sandbox3d/ and physics/ — example projects
  Source/           the include root
    Reflect/ Platform/ Text/         leaves
    Scene/ Serial/ Components/       the world model
    Physics/                         rigid bodies, collision and the solver
    UI/Core/ UI/Framework/ UI/Widgets/ UI/Docking/  the UI framework the editor is built on
    Gfx/ Gfx/RHI/ Gfx/VK/ Gfx/MTL/ Gfx/Asset/ Gfx/Pass/  the renderer, its seam and two backends
    Core/ Game/                      the engine, module loading, and the API game code calls
    Dev/                             editor panels, gizmos, history, play session; never in the player
    Player/ Editor/ Packager/        the three executables
    Programs/CinderBuildTool/        cbt, the build tool
    ThirdParty/                      vendored dependencies; see its README.md
  Tests/
    selftest/                        a project whose C++ runs the engine smoke test
    modules/                         a game module cbt builds for the module tests
    gallery/ graphics/               ui_gallery and graphics_smoke
  Binaries/ Intermediate/            cbt's output; gitignored
  notes/                             the Obsidian vault
```

`Source/` is the only include root, so every include carries its layer: `#include "Scene/Actor.hpp"`.
Headers sit next to their sources, and every module's `.Build.cpp` sits in its folder.

## Layering

The layer graph is **acyclic and strictly layered**. Keep it that way — this is what lets the editor
depend on `Scene` + `Reflect` without dragging in Vulkan.

```
Reflect, Platform          -> leaves (no engine includes)
Text                       -> Platform
Scene                      -> Reflect
Serial                     -> Scene, Reflect, Platform
Components                 -> Scene, Reflect
Physics                    -> Scene, Reflect
UI/Core                    -> Text, Platform
UI/Framework               -> UI/Core, Text, Platform
UI/Widgets                 -> UI/Framework, UI/Core, Text, Platform
UI/Docking                 -> UI/Widgets, UI/Framework, UI/Core, Text, Platform
Gfx/RHI                    -> Platform
Gfx/VK, Gfx/MTL            -> Gfx/RHI, Platform          (only one compiles per binary)
Gfx/Asset                  -> Gfx/RHI, Scene
Gfx/Pass                   -> Gfx/Asset, Gfx/RHI, Scene, Platform
Gfx                        -> Gfx/Pass, Gfx/Asset, Gfx/RHI, UI/Core, Text, Scene, Platform
Core                       -> all of the above
Game                       -> Core and all of the above
Dev                        -> Core and all of the above (a separate target, see below)
```

Everything down to `Core` is the engine. **`Dev` is not** — it is its own module, and it holds
everything editor-only: the panels in `Dev/Panels/`, the Scene view with its editor camera and gizmos,
the selection, the undo history and the play session. The `Player` module depends on `Core`, and the
`Editor` module on `Dev`, so `Dev` is only ever linked into the editor, the packager and the tests.

**The editor's UI is the engine's own** — see *The UI framework* below. `Text` and the four `UI`
layers are part of `engine`, because the game will use them too; `UI` never includes `Scene`,
`Reflect` or `Gfx`. The `shipping` test runs `nm` over `player` to prove no `cinder::Dev` symbol reached it.

**`Platform` is the one leaf everything may reach for**, because `Platform/Log.hpp` lives there and
every layer logs, and `Platform/Assets.hpp` is the only thing that turns a name into a path. That is
the only reason `Serial` and `Gfx` have an edge to it, and neither header includes anything
from the engine, so the edges cost nothing and create no cycle.

**`Gfx` is split five ways and the seam that matters is `Gfx/RHI`.** It is a neutral leaf — formats,
`Commands`/`Uploads` handles, `TextureBinding`, the fluent `PipelineBuilder` — that *declares* the
types a backend must provide: `Ctx`, `GpuBuffer`, `GraphicsPipeline`, `Texture`, `TexturePool`,
`RenderTarget`, `GlyphPages`, `Presenter`. `Gfx/VK` **defines** them in namespace
`cinder::Gfx::RHI`, and `Gfx/MTL` defines the same names in Objective-C++; `cbt` compiles exactly
one of the two, because `Gfx/RHI`'s rules add a link dependency on `Gfx/MTL` or `Gfx/VK` by backend.
The choice is `cbt`'s `--backend`, not a runtime flag — the same reasoning that makes the dev tools a
separate module. No vtables: `Engine` still holds `RHI::Ctx ctx_` by
value in the same declaration slot.

**Both backends work.** `--backend vk` builds the Vulkan renderer; `--backend mtl` builds the native
Metal one, which is macOS-only and compiles its `.mm` files with `-fobjc-arc`, linking the Metal,
QuartzCore, Foundation, CoreGraphics and AppKit frameworks. The Metal
backend is about half the Vulkan one's size, because Metal has no allocator, no descriptor pools,
no layout barriers and no explicit frame synchronization — those concepts are deleted rather than
ported. Its headers are plain C++ holding `void*`, bridged inside the `.mm` files, because neutral
`.cpp` translation units include them through the selector.

Three places are where Metal genuinely differs from Vulkan, and each shaped the seam:
`beginScenePass`/`beginPresentPass` take `Frame&` rather than `const Frame&`, because Metal makes a
fresh encoder per pass while Vulkan reuses one command buffer; `Commands::handle` points at a
`CommandState` carrying the encoder, the current index buffer and a push-constant staging block,
because Metal has no index-buffer binding point and `drawIndexed` must find it; and
`TextureBinding::id` points at a texture/sampler pair, because Metal binds those separately where
Vulkan has one combined image sampler. `PipelineFactory` loads the `.metallib` phase 1 produced and
falls back to compiling the `.metal` source at runtime when it is absent.

`Gfx/RHI/Backend.hpp` is **generated** by `cbt` into `Intermediate/<Platform>/<Config>/Generated/` and
is one line — `#include "Gfx/<backend>/Backend.hpp"`. Everything above the backend includes only that,
so no first-party source names a backend. The layer check scans `Source/` only, so the generated header
is invisible to it and the checked graph stays a strict DAG. **Nothing outside a backend directory names a
Vulkan or Metal type**, and the one `<volk.h>` left outside it is `Platform/Glfw.cpp`'s loader bootstrap,
guarded by `CINDER_BACKEND_VK` — loader setup is the one place outside `Gfx/` that names a backend,
because it runs before `Gfx` exists. `Gfx/Asset` is what you draw with (`Assets` and `Mesh`; `Texture` lives in the backend, since almost all of it is staging, barriers and copies),
`Gfx/Pass` is how you draw it (`DrawPass` and its implementations, their pipelines, and `ViewCamera`), and
`Gfx` itself contains orchestration and frame resources: `Renderer`, `FrameTargets`, `UiRenderer`,
`CompositePipeline` and `RendererDrawList`. `RenderTarget` is per backend, and the neutral pieces both
backends share — image decoding, capture encoding, `GlyphRegion` — live in `Gfx/RHI`. Nothing in a subdirectory includes
its parent. `cbt check` and the `layers` test enforce these include directions. `UI` is split
the same way, and the same rule holds: `UI/Core` never includes `UI/Widgets`.

Four placements are load-bearing and were each chosen to kill a cycle: `Glfw`/`Window`/`Input` live
in `Platform`, not next to `Engine`; `DrawList` lives in `Scene`, so the scene graph never includes
`Gfx`; `PropValue` lives in `Scene`, not `Serial`, so an actor's attributes are scene data that the
serializer and Details each read without an edge to one another; and `Core/Module.hpp` — the
`Registrar` and the registration macros — lives in `Core`, not `Game`, so `Core/GameModule` can load a
library without including the layer above it.

`Game` sits on top of `Core` and is the one layer game code includes: `Game/Game.hpp` pulls in the actor and
component classes and declares free functions over the running engine. It forward-declares `Engine` rather than
including it, so a module needs only `Source/` and glm on its include path.

`Platform/Files` holds the shared binary-mode file I/O used by project loading, scene files, fonts,
module builds and editor history. `writeTextFile` writes a temporary file beside the target, checks the
stream, and renames it over the target, so a failed write never truncates a scene; callers still decide
whether to create parent directories. `Platform/Utf8` and `Platform/Strings` hold the UTF-8 and trimming
helpers every layer shares. `Serial/NumberText` holds locale-independent
numeric formatting shared by the text and INI writers; their parsing and wire formats are unchanged.
`Reflect::copyProps` copies through each property's existing writer, including clamping and notifications.

## Frame flow

`main` -> `Glfw::acquire()` -> `Engine(config)` -> `engine.openScene(path)` -> a loop.

`Engine`'s constructor **loads nothing and runs nothing**; the host decides what
happens next. The player calls `GameLoop::tick` every iteration. The editor calls
`PlaySession::tick`, which calls one of three `GameLoop` entry points:

- **`tick`** — the simulation:
  1. `Glfw::pollEvents()`
  2. minimized -> `Glfw::waitEvents()`, reset the clock, skip the frame
  3. `GameLoop::advance()` accumulates real time, clamped at `MAX_FRAME_TIME = 0.25s`
  4. `engine.update(fixedDt)` N times — fixed timestep; `scene.update` (which starts pending actors,
     runs due timers, then updates the tree), `physics.step`, then `input.consume()`
  5. `engine.render(alpha())` once — `renderer.beginFrame()`, `scene.render(alpha, draws)`,
     `renderer.drawFrame()`
- **`idle`** — Edit mode and Paused: steps 1–2, then it resets the clock, calls `input.consume()` and
  renders. Consuming on idle frames is load-bearing: without it, the key that pressed Play, or an
  Esc from Edit mode, would fire as `keyPressed` on the first play step.
- **`step`** — exactly one fixed update, then render. The Step button.

`Engine` is a library that gets ticked, not something that runs itself. **Do not move the loop into
`Engine`.** `GameLoop::advance()` and `alpha()` are public precisely so the tests can drive them
without a window; keep new loop logic in that shape.

The editor's loop also calls `ModuleHost::update` every frame and, between frames, applies a pending
hot reload — see *Game code*.

## The scene: actors and components

The world is **Unreal's object model**: a `Scene` holds **actors**, an actor owns **components**, and
transforms live in a hierarchy of **scene components**. A Box is a `StaticMeshActor` whose
`StaticMeshComponent` is its root, carrying a `Spinner` and a `Bobber` component beside it. The
engine's components draw, simulate and hold transforms, and a project's C++ adds the rest — actors and
components registered exactly like the engine's own (see *Game code*). The tree of nodes this replaced
was dropped on 2026-11-07.

- **`Scene/Object`** is what actors and components share: an id, a name, the scene, `enabled`, and the
  reflection hooks. `CINDER_PROPS(Object, void)` declares `enabled`, so it is the first prop of every
  class. **Actors and components share one id space**, so a selection, a connection, a timer or a
  scene file names either by the same kind of id.
- **`Scene/Actor`** (`CINDER_ACTOR(Type, Base)`) owns its components, holds attributes, and is attached
  to at most one parent actor — the attachment tree is what the Outliner shows and what ticking walks.
  Its hooks are `beginPlay`, `tick(dt)`, `endPlay`, `onAttributeChanged`, `onChildAdded`,
  `onChildRemoved`, `onComponentAdded`, `onTouched` and `onTouchEnded`. `addComponent<T>(name)` called
  in a constructor makes a **default component** — Unreal's `CreateDefaultSubobject` — and called later,
  or through `Scene::addComponent`, an instance component. `findComponent<T>()`, `findComponent(name)`
  and `findAttached(name)` look things up; `transform()` is the root's, or null.
- **`Scene/Component`** (`CINDER_COMPONENT(Type, Base)`) has an `owner()` and the hooks `beginPlay`,
  `tick(dt)`, `onRender(alpha, draws)`, `endPlay`, `onTouched` and `onTouchEnded`. `isDefault()` is
  computed, not stored: a component is a default when the actor class's default instance has one of the
  same name and class. Only instance components can be removed in the editor.
- **`Scene/SceneComponent`** owns a `Transform` and attaches to another scene component **of the same
  actor**. The **root** is the first unattached scene component in the actor's list, and
  `setRootComponent` moves one to the front. A scene component with no attach parent that is not the
  root is attached to the root implicitly, and a root composes with the root of the nearest attached
  parent actor that has one — so an actor with no root passes its parent's transform through.
- **`Scene/ClassRegistry`** is the class registry: a name, a kind (actor or component), a factory, the
  prop list and a default instance per class for delta encoding. Re-binding a name keeps its slot, so
  iteration order, and the editor's add menus, is stable across a reboot; `remove` empties a slot for a
  library being unloaded. Types are keyed by their **mangled name**, not `std::type_index`: on Apple's
  libc++ a library's hidden copy of a header-only class's `type_info` hashes by name while the engine's
  hashes by address, so an `Actor` created by game code would otherwise not be found.
- **`Scene/Scene`** owns every actor by id. `insert` takes an actor, an optional id and optional ids for
  its components — the codec's path — `spawn` builds one by class name, `addComponent` adds a component
  by class name, and `clone` copies an actor, its components, their attachments and the actors attached
  to it through the registry, with fresh ids. A `Ref` in the copies that names an object inside the
  copied subtree is pointed at that object's copy, and one naming anything outside is left alone, so a
  duplicated turret aims its own barrel. An object inserted without a name takes its class name.

Lifecycle rules that are easy to lose:

- Every inserted actor queues `beginPlay`, which runs at the top of the next `Scene::update`, disabled
  or not: **its components first, in order, then the actor**, Unreal's order. A component added to a
  started actor begins on the next update; an actor spawned during a begin is started in the same pass.
- A tick runs the actor, then each of its components, then the actors attached to it. A disabled actor
  skips itself, its components **and the actors attached to it** in `update` and `render`. A disabled
  scene component skips itself and the components attached to it; a disabled root takes the implicitly
  attached ones with it. Timers still fire.
- `destroy` is deferred to the end of `update`, for an actor or a component. Destroying an actor
  destroys its components and attached actors; destroying a scene component destroys the components
  attached to it. `endPlay` runs on started objects only, components last to first and then the actor.
  `destroyNow` tears down at once and exists for the editor, which never calls `update` in Edit mode;
  never call it from inside an update walk.
- `Scene::clear` emits nothing, so loading a scene is not a stream of destroy events, and it drops
  every connection and timer.

**Signals.** `Scene::emit(source, Signal, args)` is how the scene announces `AttributeChanged`,
`ChildAdded`, `ChildRemoved`, `ComponentAdded`, `Touched`, `TouchEnded` and `Destroying`. It calls the
source's own hook, then every handler that `Scene::connect(source, signal, owner, handler)` registered —
but only for **started** objects, so nothing reacts in Edit mode. `SignalArgs` carries the other actor,
the other component (the collider, for a touch) and a name. A connection is keyed by both ids and dies
with either. `Scene::after` and `every(owner, seconds, fn)` are timers on scene time, run at the top of
`Scene::update` after the starts, and owned by an actor or a component. Neither connections nor timers
outlive the scene, which is what makes unloading a game library safe. An attribute notifies only on a
real change — `sameAttribute` counts `2` and `2.0` as equal — and `loadAttributes` never notifies.

| Class | Kind | Props | |
|---|---|---|---|
| `Actor` | actor | — | a plain actor; give it components |
| `StaticMeshActor` | actor | — | default `StaticMeshComponent` root |
| `SpriteActor` | actor | — | default `SpriteComponent` root |
| `CameraActor` | actor | — | default `CameraComponent` root |
| `SceneComponent` | component | — | a transform and nothing else |
| `StaticMeshComponent` | scene component | `mesh`, `texture` | |
| `SpriteComponent` | scene component | `texture`, `size`, `color` | a quad in its local XY plane |
| `CameraComponent` | scene component | `projection`, `fov`, clip planes, `zoom`, `clearColor`, `virtualSize` | `projection` is all that makes a scene 2D |
| `RigidBodyComponent` | component | `motion`, `mass`, `gravityScale`, damping, `velocity`, `planar` | physics moves its actor's root; lives in `Physics` |
| `ColliderComponent` | scene component | `shape`, `size`, `friction`, `restitution` | a shape of its nearest body; see *Physics* |

A new class derives from `Actor`, `Component` or `SceneComponent`, declares its props with
`CINDER_ACTOR` or `CINDER_COMPONENT`, and is registered — engine classes in
`Components::registerBuiltins`, physics ones in `Physics::registerComponents`, game classes by
`CINDER_REGISTER` in their library. The serializer and the editor need no change.

### The scene file

`Serial/SceneCodec` writes version 5:

```
version 5
actors {
    StaticMeshActor {
        id 2
        name "Box"
        components {
            StaticMeshComponent {
                id 3
                position -6 0 -6
            }
            Spinner {
                id 4
                speed 0 0.5 0
            }
        }
    }
}
```

An item's block name is its class. An actor writes `id`, `name` (omitted when it is the class name),
its class props where they differ from the class default, `components`, `attributes`, then `children` —
the actors attached to it. A component writes `id`, `name`, its transform if it is a scene component,
its props, then `attached` — the scene components attached to it. **A default component is written
against the same component in the actor class's default instance**, Unreal's archetype, so a value the
constructor set is not repeated; an instance component is written against its class default. On load,
a default component the constructor already made is matched by name and class and filled in, and
`Actor::arrange` restores the file's order, so the root stays the root. The first top-level scene
component is the root. An unknown class is logged once and loaded as an `ActorPlaceholder` or a
`ComponentPlaceholder` that keeps the class name and its fields verbatim (`Archive::raw`, in file order)
and writes them back unchanged, so a scene saved while a game module is missing or failed to build loses
nothing. A component placeholder is a scene component that is never the root, so the components
attached under it keep their place; an actor placeholder's components and attached actors load as
ordinary ones, and cloning a placeholder makes another. Every token keeps its quoting, and only an
unquoted `{` opens a block, so an actor named `{` round-trips. A stray or missing `}` fails the load with
its line number before anything is cleared, numbers are parsed with `from_chars`, a malformed one is
logged and falls back to the default, and an id outside `1..Scene::MAX_ID`, or taken, is replaced with a
fresh one. A value an attribute cannot hold is logged and dropped. `VERSION` and `OLDEST` are both 5:
the node-tree format is not read, and the repo's scenes were converted once.

**A reference is an integer field**, `target 82`, delta-encoded against the class default, which is
always 0. Save writes it only while it resolves to a live object its field accepts, so a reference to a
deleted actor disappears on the next commit and a stale id can never land on a later actor that took the
same number. Load does not write references during the walk: it records each one with the id it names
in the file and maps every object's file id to the object it became, then sets them all once the walk
is done — which is what makes forward references work and what follows an object whose id was out of
range to its fresh one. On a duplicate id the object that kept it wins, and an id that names nothing in
the file loads as 0. A placeholder's raw fields are copied verbatim, so a reference inside an
unknown-class object is neither checked nor remapped.

## Edit mode and Play mode

A game is a `.scene` file — actors and components, some of them the project's own classes. There is no
entry point.

No game code runs until something calls `Scene::update`: `beginPlay` fires from `startPending()` at the
top of the first update, and hooks, connections and timers reach only started objects. `Scene::render`
does **not** wait for `beginPlay` — it draws every enabled component — so a scene that is never updated is still
fully drawn. That split *is* Edit mode: meshes, sprites and cameras draw, game components' `onRender`
draws too, and nothing else of the game runs. The one thing the editor draws differently is the view: Edit mode looks
through its own camera and shows the scene's cameras as gizmos — see *The editor camera*.

`Dev/PlaySession` owns the editor's state, `Edit | Playing | Paused`:

- **Play** saves the scene to text with `SceneCodec::save` and hands it to `Engine::loadScene`, which
  is `SceneCodec::load`: it parses and checks the version first and clears the scene only then, so a
  bad file leaves the scene as it was. Play therefore takes exactly the path the player takes
  from disk: every actor is rebuilt from text, connections and timers start empty, and a serialization
  bug shows up on Play rather than on Stop. Statics inside a game library are **not** reset by Play;
  they reset only when the library is reloaded.
- **Stop** hands the same text to `loadScene` and releases a locked cursor. Play and Stop are one
  operation; only whether the loop simulates afterwards differs.
- **Pause** switches to `GameLoop::idle`; **Step** runs one `GameLoop::step`.

Toolbar buttons and shortcuts only set a request, and `PlaySession::tick` applies it **between
frames**. The toolbar draws inside `Engine::render`, and Stop rebuilds every actor, which must never
happen while an actor's own code is on the stack. For the same reason, game code calling `Game::quit()`
only sets a flag. The player ends on it; `PlaySession` turns it into Stop, the way Unity ignores
`Application.Quit` in the editor.

Save is disabled during Play: the scene on screen is the running game, not the document.

## Rendering

`Renderer` runs 2 frames in flight; on Vulkan the backend's `Presenter` owns the two `VkRenderPass`es.
Both backends present with vsync (FIFO on Vulkan), meshes cull back faces, and the mesh shader lights
through the cofactor of the model matrix, so a non-uniformly scaled mesh keeps correct normals. The **scene pass** (color + depth) draws
an ordered `vector<unique_ptr<DrawPass>>` — `MeshPass` first, then `SpritePass` — into a
`RenderTarget`, and ends in `SHADER_READ_ONLY_OPTIMAL`. The **present pass** draws one fullscreen
triangle (`CompositePipeline`) sampling that target into the swapchain framebuffer.

`RenderTarget` is a VMA color image + `DepthBuffer` + framebuffer, plus its own sampler, 1-set
descriptor pool and descriptor set — deliberately *not* routed through `Assets`, whose pool has no
`FREE_DESCRIPTOR_SET` flag and would leak a set per resize. There is one target **per frame in
flight**; a single one would be cleared by frame N+1 while frame N's composite still sampled it.
`FrameTargets` owns the collection. The target uses `swapchain.format()` (sRGB) so the encode/decode round trip is identity; a UNORM
target would visibly brighten everything.

**The target follows the window unless the host embeds it.** `setViewportSize(w, h)`, in window
points, detaches it: both targets are rebuilt at `w × h` times the framebuffer scale, the cameras are
resized to `w × h`, and the present pass stops drawing the composite triangle — the Scene view draws
the target instead, as an image element the `UiRenderer` binds from the frame's own descriptor set.
Every rebuild sits behind a `vkDeviceWaitIdle`, so no set is freed while a frame still reads it, and a
size change rebuilds on the spot, so the set the panel draws with in that same frame is already the
new one. `player` never calls it and composites exactly as before.

The scene pass carries a second subpass dependency (`0 -> EXTERNAL`, color-write -> fragment-read)
that orders the composite's sample after the scene's writes. Any new pass that reads a previous
pass's output needs the same treatment.

`DrawPass` (`beginFrame` / `record`) is the extension point. It is named `DrawPass`, not `RenderPass`,
so that `renderPass` unambiguously means a `VkRenderPass`. A new pass means: implement the interface and
add it to `passes_` in the `Renderer` constructor in draw order; `Renderer::sprites()` and `meshes()`
hand the two built-in passes to `Game/Game` for immediate-mode drawing. A pass owns no camera: `record`
is handed the frame's view-projection.

**There is one camera, and 2D and 3D differ only in its projection** — Unity's model. `Renderer` owns
a `Gfx/Pass/ViewCamera`, and the scene's `CameraComponent` writes it every frame through `DrawList::camera` as a
`Scene::View`: the world matrix, `orthographic`, fov, clip planes, zoom and virtual size. A perspective
view uses the fov at the view's aspect. An orthographic one covers `virtualSize / zoom` world units —
or the view's own size in points when `virtualSize` is zero — centred on the camera and stretched to
the view. Both clip at `near` and `far`, so an orthographic camera at `z = 0` cannot see a sprite at
`z = 0`; `Samples/sandbox2d` puts its camera at `z = 10`. Every pass draws through the one matrix, so
meshes and sprites share a world, and a scene with several cameras draws through the last one rendered.

`Renderer::overrideCamera(view)` sets a second `ViewCamera` that the frame draws through while it is
set, and `releaseCamera` drops it. The scene keeps writing the first underneath, which is what makes a
release exact. `engine.screenToWorld` and `engine.screenSize` are bound by `Renderer` and read the
scene's camera, never the override; `screenToWorld` returns where the ray through a point meets the
`z = 0` plane, which under an orthographic camera looking down -Z is just the point under the cursor.

The world is **Y-up** through either projection: both flip Y in the projection matrix rather than with
a negative viewport height. A `SpriteComponent` is a quad of `size` in its own local XY plane, pushed through
its world matrix, so it moves, turns and scales in 3D like a mesh, with the texture's first row along
its +Y edge. The sprite pipeline tests depth without writing it (`GraphicsPipelineBuilder::depthRead`):
`MeshPass` has already written depth, so a mesh in front hides a sprite, while sprites still layer among
themselves in submission order. `engine.drawSprite`, `drawSpriteRegion` and `drawRect` place a
rectangle on `z = 0` with `x, y` at its bottom-left.

Conventions used throughout, follow them:

- every call that returns a `VkResult` goes through `check(result, "vkFunctionName")` from `Gfx/VK/VkUtil.hpp`,
  except `Ctx::waitIdle`, which logs instead because destructors call it
- structs are zero-initialized (`VkFooCreateInfo info{};`) and **every count is set by hand**
- buffers and images allocate through VMA (`ctx.allocator()`); `GpuBuffer` is always host-visible
  and persistently mapped
- every Vulkan-owning type is move-only with a destructor and no `destroy()` method. `RHI::Ctx` is
  declared **first** in its owner and destroyed **last**; everything holding Vulkan handles refers to it
  by reference or a non-owning `const Ctx*`, never a copy and never an owning pointer. Declaration order *is* the destruction
  contract.

`Assets` hands out `int` texture handles (`DrawList::WHITE == 0`), caches by path, and never frees;
`MAX_TEXTURES = 256` is backed by a fixed-size descriptor pool.

**Components never store those handles**, because a handle means nothing on the next run.
`SpriteComponent::texture` and `StaticMeshComponent::texture` / `mesh` are *names* — a path relative to `Content/`,
or a primitive such as `"cube"` — resolved through `DrawList::textureHandle` / `meshHandle` on first
draw and cached until `propChanged` says the name changed. `RendererDrawList` caches the handle by
name, so a thousand sprites on one texture resolve its path once. A missing file logs once and draws white;
an unknown mesh logs once and draws a cube. The clear colour and the 2D virtual size are `CameraComponent`
props, pushed to the renderer every frame through `DrawList::background` and `camera`, so a scene
file carries them.

**The UI draws in the present pass**, after the composite, through
`Gfx/UiRenderer` — never as a `DrawPass`, because every `DrawPass` records in the scene pass, which has
depth. `Renderer::setUiPaint` installs a hook that `beginFrame` calls with a fresh `UI::ElementList`,
in window points, every frame. Paint runs before the frame fence, so it fills CPU
arrays only: `UiRenderer::prepare`, recorded before the scene pass, batches the list with the GPU-free
`UI::batch`, fills that frame's growable host-visible buffers and uploads the dirty rectangles of the
list's glyph atlas into R8 pages — barriers from `SHADER_READ_ONLY` so the glyphs already there survive,
and dirty rectangles cleared at record time, because the `OUT_OF_DATE` path paints without recording.
One pipeline draws everything with a mode per vertex: solid, textured, glyph coverage with a gamma
term, a rounded-box SDF for fills and borders, and an opaque image for the scene target, so the Scene
view is never blended by its own alpha. Output is premultiplied; a non-sRGB swapchain gets a
manual encode, and a format change rebuilds the pipeline. Only elements that sample a texture break a
batch, so boxes, lines and text on one glyph page draw in one call. `pixelsPerPoint` is the swapchain
width over the window's width in points, measured whenever the swapchain is rebuilt and never
in between, so a live resize cannot rasterize glyphs at a stream of fractional sizes.

`Renderer::capture(path)` delegates target readback and PNG encoding to `Gfx/Capture`.
`requestWindowCapture(path)` copies the swapchain image of the next frame instead, adding `TRANSFER_SRC`
to the swapchain the first time it is asked. Both stall the device — debug tools, not
per-frame features.

## Physics

**The physics engine is our own**, like the renderer and the coming UI framework — not Jolt, not Box2D.
It is `Source/Physics/`: a layer over `Scene` and `Reflect` with no Vulkan and no window in it, so all of
it runs headlessly in `cbt test`.

`Engine` owns a `Physics::World` and steps it in `Engine::update`, after `Scene::update` and before
`input.consume()`. Edit mode never calls `Engine::update`, so **physics never runs while you edit** —
the same split that keeps game code from running, at no cost in code.

Two component classes, registered by `Physics::registerComponents`:

- **`RigidBodyComponent`** is what makes an actor physical, and physics moves the actor's **root
  component**: `motion` (`static | kinematic | dynamic`), `mass`, `gravityScale`,
  `linearDamping`, `angularDamping`, `velocity`, `angularVelocity` and `planar`. **`planar` is all that
  makes physics 2D**: it locks z travel and x/y turning, so a 2D scene is the same engine with three
  degrees of freedom removed — the rule the renderer already follows, where `projection` is all that
  makes a scene 2D. Velocity is a prop like any
  other, so a scene file carries a starting velocity, Details shows it live during Play, and
  `body->setVelocity(0, 5, 0)` works from game code — with no code in the editor. It is a plain
  component, not a scene component: its pose is the root's, and an actor with no root is not simulated.
- **`ColliderComponent`** is a shape, a scene component: `shape` (`box | sphere | capsule`), `size`, `friction` and `restitution`. The
  material lives on the shape, not the body, because a static floor is a collider with no body and
  still has to be rough or bouncy; a pair combines as `sqrt(fA * fB)` and `max(eA, eB)`. Its own transform
  offsets it from the body, and `size` is the box the shape fits in — a sphere takes the largest of the
  three; a capsule stands along its own Y, as wide as the larger of x and z and as tall as y, so
  `1 2 1` is a capsule of length 2 and `1 1 1` collapses to a sphere. The default `1 1 1` box is
  exactly the unit cube a `StaticMeshComponent` draws.

**A collider belongs to the body of its own actor, or else of the nearest attached-parent actor that
has one**, which is Unreal's welding rule: every shape under one body is one rigid compound. A collider
with no body above it is static, so a floor is a `StaticMeshActor` with a `ColliderComponent` beside its
mesh and nothing else — Unity's and Unreal's behaviour. A collider takes its root's transform, so a
unit collider on a mesh scaled `24 1 24` is the mesh's slab.

`World::step` **walks the scene** every step rather than keeping a registry, which is what makes
spawning, destroying, disabling and reattaching a body need no bookkeeping: a disabled actor and the
actors attached to it are skipped exactly as `Scene::update` skips them, a disabled component is
skipped on its own, and `Scene::clear` on Stop empties the world by
construction. The walk is in tree order and nothing iterates an `unordered_map`, so a scene steps
identically every run; `Tests/physics_test` pins that bit for bit.

`World` gathers `BodyState` values, owns scene traversal, integration, sleeping and contact events,
and delegates contact preparation, warm starting and impulse solving to `ContactSolver`. The solver
owns its contacts and carried impulses, operates on a span of the current step's body states, and
exposes read-only contacts for island sleeping. Begin it only after body gathering is complete;
never grow the state vector while the solver is using it. Constants, ordering and equations are
unchanged.

The step, in order:

1. **Place** — a body whose root's matrix is not the one physics last wrote has been moved by something
   else — a script, Details, or a parent moving — so it teleports there, keeping its velocity. A
   kinematic body always takes its pose from its root and derives its velocity from how far it moved.
2. **Geometry** — each collider's world shape, from its matrix, through `geometryOf`. `Dev/GizmoLines`
   draws the wireframes from the same function, so what is drawn is what collides.
3. **Weigh** — a body's mass is split between its colliders by volume, giving a centre of mass and a
   full inertia tensor through the parallel axis theorem. **A body turns about its centre of mass**,
   not its origin, so a collider hung off to one side behaves like the weight it is.
4. **Integrate velocities** — gravity, then damping.
5. **Detect** — `Physics::Broadphase` is a bounding volume tree, rebuilt from the frame's bounds every
   step by splitting the widest axis at the median, and every shape queries it for the shapes near it;
   a pair is tested once, when at least one side is dynamic. Sphere–sphere and sphere–box are closed
   form, and a capsule is a segment with a radius: against a sphere or another capsule it is the
   closest point on that segment, and against a box it is the closest point found by walking segment
   and box in turn — with the segment clipped to the face when it lies flat on one, so a capsule on
   the ground rests on two points rather than rocking on one. Box–box is a separating axis test over
   the 15 axes, where the winning face becomes a reference face and the other box's incident face is
   clipped against its sides, giving up to four points; a winning edge axis gives one point from the two
   edges' closest approach. **Every point carries a feature id**, which is what lets the next step
   recognise it.
6. **Solve** — sequential impulses, `ITERATIONS` passes of friction then normal. A contact keeps an
   accumulated normal impulse clamped to push only, and two tangent impulses clamped to Coulomb's
   `friction * normal`. **Impulses are carried across steps by feature id** — warm starting — which is
   what makes a stack stand rather than sag. Restitution above `BOUNCE_THRESHOLD` goes into the velocity
   pass, while **penetration is corrected in a second pass of pseudo-velocities** (split impulse), so
   pushing bodies apart never adds real momentum. Baumgarte in the velocity pass shook a ten-crate stack
   apart; that is why it is split.
7. **Rest** — a body that has been slower than `SLEEP_LINEAR` and `SLEEP_ANGULAR` for `SLEEP_TIME`
   is ready to sleep, but it only sleeps with the **island** it belongs to: contacts between dynamic
   bodies are unioned, and an island sleeps when every member is ready — otherwise one crate in a
   stack would freeze while the rest settled. A sleeping body is solved as if it were static and is
   never integrated, so a settled stack is bit-for-bit still. It wakes when its root is moved, its
   velocity written, an impulse or force applied, a contact with anything that is moving, or a contact
   it had disappears — which is what catches the floor being deleted under it.
8. **Advance** — integrate the centre of mass and the orientation by velocity plus drift, then write
   the pose back
   as the root's local position and the Euler angles in the order `Transform::local()` composes them
   (Y, then X, then Z). The world keeps the quaternion, so a tumbling body simulates exactly through
   ±90° pitch even though the angles Details shows flip there.

Three tolerances in `Collide.cpp` are load-bearing, and each was found watching a stack fall over.
Clipping keeps a corner within `CLIP_SLACK` of a side plane, and a contact within `TOUCH_SLACK` of the
reference face, because two boxes resting squarely put four corners exactly on those planes, where
float noise drops one and the lopsided support tips the box. `FACE_BIAS` makes the second box's face
beat the first's by a margin before it becomes the reference face, because on an exact tie the choice
flapped from step to step, changing every feature id and throwing the warm start away.

**Game code reaches physics directly.** `RigidBodyComponent::applyImpulse`, `applyForce` and `applyTorque` are
**accumulators**, applied at the next step and cleared there, so the order they are called in never
matters; `Game::raycast` returns an optional `RayHit`, and `Game::world()` reads and writes gravity.

Contacts reach game code the way scene changes do: `World` tracks which collider pairs touch, compares
that with the last step's, and emits `Signal::Touched` and `TouchEnded` through `Scene::emit` on both
**actors** and on both **colliders** — Unreal's `OnActorHit` and `OnComponentHit` — so their
`onTouched` hooks run, and any connection on them fires. The actor is the one that owns the body when
there is one and the collider's own actor when there is not, so a floor with no body still answers;
`SignalArgs` carries the other actor and the other collider. A `RayHit` reports the same actor and the
collider it hit.

`Config/Game.ini` carries `[Physics] gravity` as three numbers, read by `ProjectConfig` and pushed into
the world by `Engine`; a project that says nothing falls at 9.81 units a second squared.

A raycast does **not** use the tree: it walks the scene and tests every collider, because the tree
belongs to the last step and game code can cast a ray at any time, after anything has moved. One ray
costs the walk either way; many rays per frame are what would justify keeping the tree valid.

`Samples/physics` is the sample: a floor, a ramp and a step as static colliders, balls, capsules, a
stack of crates, and a `Dropper` that drops more balls onto it. `--scene Scenes/planar.scene` opens the 2D half —
an orthographic camera over sprites with `planar` bodies, where a click drops another box. `MeshPass` carries three primitives — `cube`, `sphere` and `capsule`, the last two matching what a
sphere and a `1 2 1` capsule collider cover; `Picking` still treats every `StaticMeshComponent` as a cube.

What is missing is tracked in `TODO.md`: joints, CCD, sensors, collision layers and convex hulls. So is the one that is not physics' fault — nothing interpolates
transforms, so at `fixedHz` 60 on a 120 Hz display a falling body visibly steps.

## The UI framework

**The editor runs on a retained UI of our own, shaped like Unreal's Slate and named our own way.** Widgets are objects that persist between frames; each frame the `Application` asks the
root for its desired size bottom-up (`Widget::prepass`), arranges children top-down
(`arrangeChildren` hands out a `Geometry`), and paints into the renderer's `UI::ElementList`. The same
paint builds the `HitTester`, so input always routes against what was last drawn. Everything in
`Text` and `UI` runs headlessly: `Tests/ui_harness.hpp` drives a real `Application` with a
`HeadlessPlatform` clock and clipboard.

Widgets are built with `UI::make<T>()`, which returns the widget's `Args`; chained setters fill them,
`[child]` sets the content, `+ T::slot()` adds a slot, and the `Args` convert to `std::shared_ptr<T>`
— or to any base — by building the widget. `.assign(ptr)` keeps a handle, the way `SAssignNew` does:

```cpp
auto toolbar = UI::make<HorizontalBox>()
    + HorizontalBox::slot().autoWidth().padding(4)
    [
        UI::make<Button>().text("Play").onClicked([&] { session.requestPlay(); return Reply::handled(); })
    ]
    + HorizontalBox::slot().fill(1)
    [
        UI::make<Label>().assign(title_).text([&] { return history.title(); })
    ];
```

A widget declares `struct Args : UI::Args<Args, Widget>` with `UI_ATTR`, `UI_ARG`, `UI_EVENT`,
`UI_CONTENT` and `UI_SLOTS`, each taking an optional default, and implements `construct(const Args&)`.
`UI::Args` adds visibility, enabled, tooltip and cursor to every widget. An `Attribute<T>` holds a value
or a getter called when read, which is how a label follows `history.title()` without being told.
`auto x = UI::make<T>()...` is the `Args`, not the widget; name the type or call `.build()`. Slot types
live at namespace scope, because clang cannot use a nested class's member initializers inside the
class that encloses it.

**Input goes through `Platform::Input`'s event queue, not its edge flags.** `setRecording(true)`
switches the queue on; the host drains it with `takeEvents()` in the UI paint hook and hands the batch
to `Application::processEvents`. Keys arrive with repeats and modifiers — ⌘ is
`modifiers::PRIMARY` on macOS and Control elsewhere — characters arrive separately, cursor moves
coalesce, and losing window focus synthesizes releases for the UI and the game alike. `consume()`
never touches the queue: it runs on every idle frame, which is all of Edit mode. Routing is Slate's:

- the mouse bubbles from the widget under the pointer up its path until a `Reply` says handled, or
  goes straight to the widget that captured it; a press focuses the deepest focusable widget and
  clears focus when there is none, which is how a text field commits when you click elsewhere;
- keys bubble from the focused widget, and only a key nobody handled reaches the global
  `CommandList`s, so a text field keeps ⌘Z for itself;
- hover is recomputed after every paint, a second press within `DOUBLE_CLICK_TIME` and
  `DOUBLE_CLICK_DISTANCE` is a double click, and `Reply::detectDrag` fires `onDragDetected` once the
  pointer passes `DRAG_THRESHOLD`;
- disabled widgets are still hit, so they can show tooltips, but no handler of theirs runs.

`Platform/InputScript` replays a text file of timed input — `12 move 40 60`, `13 click left`,
`14 type Crate`, `15 tap enter`, `20 capture out.png`, `21 quit` — through `Input::inject` with the
OS's events ignored. It is how a change to a panel is verified without a human at the keyboard, and
`graphics_smoke` drives the whole editor the same way.

**The editor panels live in `Dev/Panels/`**, namespace `cinder::Dev::Panels`, over the units that
know nothing about any UI: `Toolbar` (the File, Edit, Window, Play and Platforms menus, play controls,
save, undo and redo, Build and the module status, the dirty marker, the ⌘ shortcuts as a `CommandList`,
and the modal Save / Don't Save / Cancel and packaging `Dialog`s, which swallow every key), `SceneView` (a `Viewport`
whose client drives `EditorCamera`, `Picking`, `Manipulation` and `GizmoLines`), `Outliner`,
`Details`, `Console` and the docked `Layout`. The host's
paint hook runs, in order: `processEvents` on the drained queue, each panel's `update`, `paint`, then
`SceneView::afterPaint`, which sets the camera override and the game's input suppression — keyboard
while the Scene has focus, mouse while it is hovered or holds the capture, both while the cursor is
locked. A panel's `update` is also where log lines queued by the sink join the console, so nothing
edits the widget tree while it is being painted. `SceneView` is not the viewport's client itself: the
widget would own it and it would own the widget, so a small forwarding client breaks the cycle.

**Popups belong to the `Application`, not to a widget.** `pushPopup(content, anchor, options)` places
the content below, beside or at a point next to an anchor rectangle, flipped and clamped into the
window, and paints it above the whole tree, so a popup is never clipped by the panel that opened it. A
press outside every popup closes them all and goes nowhere else — it cannot pick in the Scene view on
its way — while a press in a lower popup closes the ones above it, and a press on a popup's `owner`,
such as the combo that opened it, is routed normally so the owner can toggle. An Escape that nothing
handled closes the top popup, and focus returns to whatever held it before. A popup opened during a
press keeps the focus it took. Every menu, combo list, submenu, context menu and colour picker is one of
these. **Tooltips** are the `Application`'s too: after `TOOLTIP_DELAY` over the deepest widget with
`toolTipText`, disabled ones included, it draws the text beside the cursor, unhittable; a press hides it
until the pointer leaves that widget. Widgets that open popups hold a `UI::PopupHandle`, which remembers
the `Application` they opened on, so tests can ask `isOpen()` outside a frame, and dismisses the popup
when the widget is destroyed — so Undo or a hot reload rebuilding Details never leaves a dropdown
calling into a dead combo box. Anything that reads the hit grid during paint — a
`tick` — finds it empty, because paint rebuilds it; a menu row therefore anchors its submenu to the
rectangle it last painted.

`Menu` is built by `MenuBuilder` — entries, checks, headings, separators, submenus opened by hover
after `SUBMENU_DELAY` or by the arrow keys, and `command(list, command)`, which takes its label,
shortcut, enabled state and check from the `CommandList`, so a menu entry and a shortcut can never
disagree. `MenuAnchor` opens any widget as a popup, `ComboButton` and `ComboBox` sit on it, and
`MenuBar` opens on press and follows the pointer across titles, so press, drag and release picks.
`SpinBox` keeps its value exact: a drag adds `step` per point from the threshold on (Shift ten times,
Alt a tenth), a click types into it, and what is typed is evaluated as arithmetic; an edit that
changes nothing writes nothing, it clamps only to bounds it was given, and it shows three decimals but
edits the shortest text that reads back as the same float. `ColorPicker` works in sRGB-encoded HSV
over a linear value, keeping the hue through greys, and its square is a mesh of per-vertex colours
fine enough that interpolating in linear space does not show. `Shortcut::label()` spells modifiers out
(`Shift+Cmd+P`) until shaping falls back to a symbol font, because Roboto has no ⌘.

**A `TreeView` builds only the rows that fit.** Items are ids, never pointers, so Play, Stop and Undo
rebuilding every actor cannot invalidate a row: each frame it flattens the expanded items through
`treeItemsSource` and `onGetChildren`, realizes the slice in view — reusing the row a visible item
already has — and drops the rest, so a thousand items cost a dozen widgets. A `ListView` is a
`TreeView` with no children callback. Selection is the tree's own, or the panel's when
`isItemSelected` is bound, which is how the Outliner follows `Selection` with nothing to keep in
sync. A press on the expander arrow only expands; a press on the row selects and arms a drag; a double
click expands or calls `onMouseButtonDoubleClick`; a right press selects and opens
`onContextMenuOpening` at the pointer; arrow keys move the selection, expand and collapse, and
`reveal` expands an item's ancestors and scrolls it into view.

**Drag and drop is the `Application`'s, like popups.** `Reply::beginDragDrop(operation)` from
`onDragDetected` starts it; while it runs, moves route `onDragEnter`/`onDragOver`/`onDragLeave` to the
widgets under the pointer and the release routes `onDrop`, with the operation's `decorator()` painted
at the cursor, clamped into the window and never hit-tested. Escape or losing the window cancels, and
either way `onDropped(accepted)` tells the operation what happened. A tree answers `onCanAcceptDrop`
with the zone it will take — above, onto or below a row, or the empty space below them — and paints
that as an outline or a line. `Application::navigate` is Tab: it walks the widgets that take keyboard
focus in paint order, backwards with Shift, and stays inside the popup the focus is in. `setFocus`,
`pushPopup` and the dismissals set the thread's current `Application` themselves, so a panel may call
them from its `update`, outside the paint.

**Docking is a layout tree, not widget surgery.** `UI/Docking`'s `TabManager` holds a `LayoutNode`
tree of splits and stacks, and every change — a drop, a close, a reopen — edits that tree and rebuilds
the widgets from it, carrying the splitter sizes over first, so nothing has to reparent a live
`Splitter`. A tab's panel is spawned once and cached by id, so re-docking the Scene view keeps its
camera and its render target. Dragging a tab carries a `TabDragDrop`; the stack under the pointer
answers with the side it would take — its middle joins the stack, an edge splits it — and paints that
as a translucent overlay. Closing the last tab of a stack collapses the stack, and a split left with
one child is replaced by that child. `fillWindowMenu` builds the Window menu: a check per registered
tab, disabled for a tab that may not close, such as the Scene. A tab bar keeps its `DockTab` widgets
across an activation, because rebuilding them mid-press would destroy the widget whose drag was just
armed.

Styles come from a `Theme` of named entries — `"Button"`, `"Button.Primary"`, `"Label.Mono"`,
`"Color.Primary"` — built by `UI::defaultTheme()` in the colours of Unreal's dark editor. Colours are
linear `UI::Color`s authored as sRGB hex; the swapchain encodes them.

## The dev tools

**The dev tools are a separate module, not a runtime flag.** The `player` module never depends on
`Dev` and the `editor` module does, so nothing editor-only is in the shipping binary. There is no
`--dev`: to get the tools, run `editor`. `tests` depends on `Dev` as well, so `History`, `Picking`,
`EditorCamera`, `Manipulator` and the panels themselves are tested headlessly.

The seam is `Renderer::setUiPaint(std::function<void(UI::ElementList&)>)`: `Gfx` knows how to draw an
element list and nothing about panels, `Application` or `Scene`. `Dev/EditorShell` owns the panels and
the UI application, and installs a hook that drains `Input`'s event queue into `Application::processEvents`, updates each panel, paints the
widget tree into the list, and lets the Scene view apply its camera override and input suppression.
The editor and `graphics_smoke` both build their UI from it, so the smoke test drives the editor as it
ships. `player` never installs one, and what remains in the shipping binary is an empty `std::function`, a
zero viewport size, an empty camera override and a couple of branches per frame. That is the whole
cost of the seam.

`Input` installs its GLFW callbacks in its constructor, and `Engine` declares `input_` **before**
`renderer_`; keep that order, because the window has to exist and its callbacks have to be installed
before anything else listens.

The editor's layout is not persisted between runs yet: `TabManager` holds it in memory, and writing
it to the project's `Saved/` is the "editor layout persisted between runs" item in `TODO.md`.

### The Scene view

`Dev/Panels/SceneView` is the "Scene" tab: a `UI::Viewport` widget whose client it drives. Each frame
the widget's arrange step hands its size to `Renderer::setViewportSize` and its top-left to
`Input::setViewportOrigin`, and its paint draws `TextureRef::viewport()` — the frame's scene target,
opaque — with the gizmo strokes over it. The order is load-bearing: `setViewportSize` may rebuild the
targets, so the image is drawn after it.

`Input::mouseX`/`mouseY` subtract that origin, so `engine.mousePosition()` and `screenToWorld` work
in the panel's own points, top-left at zero, exactly as they do across the player's whole window.

The panel also owns input routing, through `Input::setSuppressed`, applied in `afterPaint`. The game
gets the **keyboard while the Scene view has focus** and the **mouse while it is hovered** or holds the
mouse capture. A locked cursor gives the game both and turns the UI's mouse off with
`Application::setMouseEnabled(false)`: GLFW still reports a virtual cursor while disabled, and the UI
would otherwise hover whatever panel it wanders over. Entering `Playing` focuses the Scene view, so
Play and Resume hand the game the keyboard without a click on the scene first.

In Edit mode a **left click** on the image picks. `Dev/Picking` is a CPU raycast that knows nothing
about the UI: the click unprojects through the editor camera and hits every active
`StaticMeshComponent` as the unit cube `Mesh::cube` is — which is every mesh there is — every active
`SpriteComponent` as the quad it draws, in its own plane and axes, and every active `CameraComponent`
within 10 points of where it projects, taking the nearest. `pick` returns the component it hit, and the
Scene view selects that component's actor, as Unreal's viewport does. A tie goes to the later component
in render order, so of two sprites in one plane the one drawn on top wins. A disabled actor hides itself
and the actors attached to it, as it does from `Scene::render`. A click is a press and
release that stays inside `Application::DRAG_THRESHOLD`; past it, a left drag still looks around. A hit
selects with `reveal`, so the Outliner expands the actor's ancestors and scrolls to its row; a miss
clears the selection. A press on a gizmo handle comes first: it drags the handle, and its release never
picks — see *Transform gizmos*.

`Dev/Panels/Layout` builds the default dock layout — the Outliner left, the Scene view in the middle
with the console under it, Details right — as a `TabManager` layout of splits and stacks, and hands
the toolbar the Window menu that reopens a closed tab.

### The editor camera

In Edit mode the Scene view looks through `Dev/EditorCamera`, not through the scene's `CameraComponent`.
`SceneView::afterPaint` hands its view to `Renderer::overrideCamera` on every Edit frame and calls
`releaseCamera` in every other state, so Play and Pause show the game's camera. The override covers the whole frame,
sprites included, so a 2D scene is flown around exactly like a 3D one — Unity's Scene view with 2D
mode off.

The editor camera is always perspective. It is seeded once, on the first Edit frame, from the last
active `CameraComponent` in render order — the one whose `DrawList::camera` call wins — taking its pose, fov
and clip planes. From a perspective camera that is the game's frame exactly. From an orthographic one
it backs away along the camera's forward axis until its fov frames the orthographic rectangle at the
near plane, pushes its far plane back by the same distance, and flies at half that distance per
second, so a 640-unit view does not crawl at 5 units a second; anything beyond the near plane looks
slightly smaller than it will in Play. Stop does not reset it: like Unity's Scene view, it stays where
you left it. It has no roll and is not saved. `Tests/editor_camera_test` pins both seeds.

Its controls are not read from `Input` — they are an interaction with a panel, and `Input` belongs to
the game — and they only act on the Scene image: the mouse comes from the widget's own events and the
keys from `Application::keyHeld`, which the UI fills from the event queue. **WASD** fly, **Q/E** go down
and up and **Shift** goes faster whenever the Scene view has focus, with no button held. Hold the
**right button** to look around, and turn the wheel while looking to change fly speed. A left drag that
does not start on a handle also looks around; drag the **middle button**, or left with **Alt**, to pan;
scroll with no button held to dolly. A press captures the mouse, so the drag keeps working past the
panel's edge, and focuses the Scene view, so fly keys never land in the console.

The Scene view draws every active `CameraComponent` while editing. A perspective camera is a wireframe frustum:
the near and far rectangles, the four edges joining them, and dimmer lines from the camera to the near
corners. An orthographic camera is its near rectangle alone — every cross-section of its box is that
rectangle, so it is exactly what the camera sees. The corners come from `ViewCamera::corners()` at the
panel's size — the size Play renders at — and a test pins them to the clip volume of both
projections. `Dev/GizmoLines` turns them into strokes in panel points, which the panel paints into the
element list as anti-aliased lines and convex fills. They are projected through the editor camera and clipped in clip space against the near plane and the four sides before the divide, so a
corner behind the editor camera cannot fold across the image. The sides are inset by `INSET`, so a camera the editor is looking straight through — as
it is right after seeding — draws nothing, rather than a frame along the border that float noise
leaves half-drawn. Being UI, the gizmos draw over geometry, cost the player nothing, and never appear
in a `--capture`.

`selectionStrokes` outlines the selection the same way, in orange: the twelve edges of a
`StaticMeshComponent`'s cube, a camera's gizmo, a sprite's quad or a collider's wireframe, all through
the editor camera — every component of a selected actor, or the one selected component.

### Transform gizmos

In Edit mode a spatial selection gets a gizmo, drawn over it from `manipulatorStrokes`. The Scene
view's tool row picks the tool — **Move**, **Rotate** or **Scale**, or **1**, **2** and **3** while the
Scene view has focus — and, for Move, **World** or **Local** axes, toggled with **X**. The tools are not
Unity's W/E/R because WASD and Q/E fly the editor camera with no button held; the panel takes those
keys from its own `onKeyDown`.

`Dev/GizmoGeometry` owns handle geometry and hit testing; `Dev/Manipulator` owns drag state. Neither
knows about the UI:

- `gizmoFor` places the gizmo at the scene component's world position, `GIZMO_POINTS` long on screen at that depth,
  so it keeps its size at any distance. There is none behind the editor camera, or under a parent whose
  matrix cannot be inverted.
- `shapes` is **the one handle geometry** — world-space points per handle — that `drawManipulator`
  draws and `hitHandle` measures in screen points, so what is drawn is exactly what can be grabbed. An
  axis within about 15° of the view ray and a plane handle within about 15° of edge-on are left out,
  which is what hides the Z arrow in a 2D scene. Ties go to the earlier shape: the centre square, then
  the planes, then the axes.
- `Manipulation` is one drag. It holds the scene component's **id**, as `Selection` does, and the transform as it
  was at the press, and computes every frame from that start rather than from the last frame, so
  snapping is exact and cancelling is a write of the start values.

**Move** has an arrow per axis, a square per plane, and a camera-facing square in the centre that drags
in the plane facing the view. An axis drag takes the point on the axis nearest the mouse ray, and a plane
drag intersects the ray with the plane. The travel is a world-space vector, taken into the parent's
space through the inverse of the parent's world matrix — applied as a direction, so a zero travel adds
exactly zero. The parent is whatever `SceneComponent::parentTransform` names — the attach parent, the
root, or the parent actor's root — and the identity at the top.

**Rotate's rings are the Euler axes, not world or local ones** — Blender's Gimbal orientation.
`Transform::local()` is T·Ry·Rx·Rz·S, so the Y ring turns about the parent's Y, the X ring about X after
yaw, and the Z ring about the component's own Z, and a ring changes exactly one component of `rotation`.
Nothing is decomposed back into angles, which is where a gizmo picks up flips near ±90° pitch and float
noise in the angles it did not touch; the price is that the rings are not perpendicular once a component
is pitched. A ring facing the camera follows the mouse round its plane, and one nearly edge-on turns with
the mouse along its screen tangent. Only a ring's front half can be grabbed, unless the ring mostly
faces the camera — perspective would otherwise put half of a face-on ring beside the centre of the view
"behind" the pivot. A mirrored parent negates the angle.

**Scale** is always local: a world-axis scale on a rotated component needs shear, which `Transform` cannot
hold. An axis handle multiplies its component by the mouse's travel along the axis over the gizmo's
length; the centre square multiplies all three by the drag right and up.

Holding **Ctrl — ⌘ on macOS** — snaps the change, not the value: `MOVE_SNAP` units along each dragged
axis, `ROTATE_SNAP_DEGREES`, and factors in steps of `SCALE_SNAP`. **Esc** cancels a drag.

The panel hit-tests on hover and on the press, through the editor camera as it was last drawn. A press
on a handle starts the drag, stops the left button looking around and keeps the release from picking,
until the capture is released. Every write goes through `PropDef`, as
Details' writes do, and calls `History::touch`, so a drag is one undo step labelled like "Move Box" —
the actor's name for its root, the component's for any other —
and a cancelled drag saves to the same text and records nothing. While the mouse is still where it was
pressed, a drag writes the start values themselves: arm64 fuses multiply-adds, so `cross(v, v)` is not
exactly zero, and a recomputed angle would move a click by an ulp and record a step. Leaving Edit mode
ends a drag.

### The Outliner and Details

`Dev/Selection` holds one **id**, never a pointer — an actor's or a component's, since they share one id
space. Play and Stop rebuild every object through `loadScene`, and ids are what `SceneCodec`
round-trips, so a selection survives both. `resolve` returns null — and forgets the id — once the object
is gone or marked destroyed, so an actor spawned during Play drops out of the selection on Stop.
`actor` resolves to the selected actor or the selected component's owner, and `placed` to the selected
scene component or the selected actor's root, which is what the gizmo moves.

`Dev/Panels/Outliner` is a `TreeView` over actor **ids** — the attachment tree, Unreal's World
Outliner — so it builds only the rows in view and nothing it holds can dangle: every row reads its actor
back out of the scene each frame. A row is the actor's name followed, dimmed, by its class — and an
actor disabled itself or through a parent is dimmed unless it is selected. A row is highlighted when the
selection is that actor or one of its components, so picking in the Scene view and clicking a row are
the same state with nothing to keep in sync. A click selects and a click on empty space clears. **"+"**
lists every registered actor class and spawns one attached to the selected actor, or at the root.
Right-clicking a row offers Add Actor, Duplicate (`Scene::clone`) and Delete; dragging a row onto
another attaches it, cycles refused, and dropping it below the rows makes it a root. Delete or
Backspace removes the selected actor while the tree has focus — a focused text field keeps the key for
itself. A filter turns the tree into a flat list of the actors whose name or class matches, with no
expanders.

Delete uses `destroyNow`, because Edit mode never runs the `Scene::update` that
flushes a deferred destroy. A spawned or duplicated actor becomes the selection. During Play the same
edits act on the running game and are discarded by Stop.

`Dev/Panels/Details` builds its rows from the prop lists, with no per-class widget code: the actor's
name, class and id; the actor's props; **Components**, the actor's components as buttons — the root,
the scene components under it indented by attachment, then the components that are not scene
components — where a click selects one, an **x** removes an instance component (a default cannot be
removed) and **Add Component** lists every registered component class and adds one, attached under the
selected scene component when there is one; then the edited component's Transform and props — the
selected component, or the root when the actor itself is selected — and the actor's attributes, each
section an `ExpandableArea` of name | value rows. The widget comes from the `PropType` and the
`PropHint`: a `SpinBox` for numbers, a `VectorInputBox` for vectors, a `CheckBox`, a `TextBox`, a
`ComboBox` filled from `PropDef::options()` for enums, a `ColorBlock` that opens a picker for
`CINDER_PROP_COLOR`, and for a `Ref` a reference field. Every write goes through `PropDef`, so clamping, in-place vector writes and
`propChanged` behave exactly as they do from the serializer. `step` is the drag speed; bounds are
applied only when the prop declares them, and a spin box never rounds a value to the three decimals it
shows. `CINDER_PROP_ANGLE` — `Transform.rotation` — is dragged in degrees and written back in radians,
so game code and scene files never see degrees. Text commits on Enter or focus loss, not per keystroke,
so a texture path does not try to load every prefix of itself.

A **reference field** is a combo showing the target's name — `Actor › Component` for a component — or
None, a **Pick** eyedropper and a **Go** button that selects the target with `reveal`. The combo's menu
lists None and every object in tree order that `PropDef::accepts`, with the current one checked. Pick arms
`Dev/ReferencePick`, which `Selection` owns so every panel already has it: while it is armed, a click in
the Scene view and a click on an Outliner row are offered to it instead of selecting — the component
hit, then its actor, then the actor's other components, so an actor field takes the actor and a
component field the component — and the selection never moves. The Scene view shows a crosshair; a miss
in the Scene view or empty space in the Outliner cancels, and so do Esc (the toolbar's `Cancel Pick`
command), any change of selection, leaving Edit mode and `Details::rebuild`, which matters after a hot
reload because the armed callback holds a `PropDef` from the old library. The menu has no filter yet.

The panel holds the selected **id** and nothing else, and rebuilds its rows only when the selection,
the actor's components or its attribute names change, so a drag is not fighting a fresh widget tree
every frame.

The **Attributes** section lists each attribute with a remove button, and **Add Attribute…** opens a
popup for a name and a type. Every attribute number is edited as a float, integers included:
`TextLoad` reads `40` back as an integer and `40.5` as a float, so an integer drag could never move a
saved `40` to `40.5`. Writes go through `Actor::setAttribute`, so an edit during Play fires the game's
changed signals.

Every edit either panel or a gizmo makes is undoable and marks the scene dirty — see *Undo, redo and unsaved
changes*.

### Undo, redo and unsaved changes

`Dev/History` is the editor's undo stack, and it stores **whole scenes**, not commands: after every
finished edit it saves the scene to text with `SceneCodec::save` — the same text Play snapshots — and
undo loads the previous text back with `SceneCodec::load`. Nothing has to describe an edit to make it
undoable: a prop, an attribute, an insert, a delete, a reparent and a console line are all the same
kind of step, which is what makes a new `CINDER_PROP` undoable with zero editor code. It costs one
scene save per finished edit and one load per undo.

Panels and gizmos never push steps. They call `touch(label, selection)` when they write, and
`Dev/Panels/Toolbar` calls `settle(app.isInteracting())` in its `update`, which commits only once
nothing holds the mouse capture, no text is being edited and no drag-and-drop is in flight. A drag that
writes on forty frames is therefore one step, carrying the label and selection of its first frame, and
an edit that saves to the same text as before records nothing. `MAX_STEPS` is 100.

Undo calls the codec directly, not `Engine::loadScene`. History is enabled in Edit mode alone, where no
game code has started: during Play, `touch`, `settle` and the buttons do nothing, and Stop discards Play's edits anyway.
Ids round-trip through the codec, so the selection stored with the step, and the Outliner's open rows,
which the tree keys by id, come back with it.

The document is the text of the last commit, and it is **dirty** while it differs from the text last
saved or opened — so undoing back to the saved state is clean again. Save commits a pending edit and
writes that text, and the toolbar shows the scene as `main.scene*` while it is dirty. Closing the window
does not end the loop directly: `Editor/main.cpp` clears GLFW's close flag and calls
`Toolbar::requestClose`, which confirms at once when the scene is clean and otherwise opens a Save /
Don't Save / Cancel modal, and the loop runs until `closeConfirmed()`. ⌘Z and ⌘⇧Z route globally like
the other shortcuts, but an active text field claims ⌘Z for its own undo first.

### Logging and the console

Everything prints through `Platform/Log.hpp` — `logInfo` / `logError`, printf-style and
`__attribute__((format))`-checked. Both always write to stdout/stderr, and additionally to a
`LogSink` if one is installed. `Dev/Panels/Console` installs that sink in its constructor and clears it in its destructor, which is how `[build]`/`[module]`/`[vk]`/`[serial]` output reaches the panel. The sink receives
the line **without its trailing newline**. `Log` itself stays in `engine`: the sink is the seam a
shipping build will use for a crash log file, so it is not a dev-only facility.

The console is a log view: a Clear button, a line count, and up to
`MAX_LINES` lines, errors in `Color.ConsoleError`. Console commands registered by game code are a
follow-up in `TODO.md`.

## Game code

A project's settings are data, not code: `ProjectConfig::load` reads `Config/Game.ini` — `[Game]`
`title`, `startScene`, `fixedHz` and `[Window]` `width`, `height` — and runs nothing. `startScene` is
the scene both executables open; `--scene` overrides it. `ProjectConfig::name` is the `.cinder` stem,
and it names the project's library.

**Game code is C++**, in the project's `Source/`, compiled into one shared library per project — Godot's
GDExtension and Unreal's game modules, built the way UnrealBuildTool builds them: the editor drives the
compiler itself, with no per-project build files. There is no entry point. A project defines **actor
and component classes**, and the scene places them. Most behaviour is a component that acts on its
owner, the way Unreal's `ActorComponent`s do; an actor subclass is for an object that is a thing in its
own right, and builds its default components in its constructor.

```cpp
#include "Game/Game.hpp"

class Spinner final : public cinder::Scene::Component {
public:
    CINDER_COMPONENT(Spinner, cinder::Scene::Component) { CINDER_PROP(speed_); }

    void tick(float dt) override {
        cinder::Scene::Transform* box = owner()->transform();
        if (box == nullptr) return;
        const glm::vec3 rotation = box->rotation() + speed_ * dt;
        box->setRotation(rotation.x, rotation.y, rotation.z);
    }

private:
    glm::vec3 speed_{0.0f, 1.0f, 0.0f};
};

CINDER_REGISTER(Spinner);
```

A game class is registered exactly like `StaticMeshComponent`, so the serializer saves its props by
delta against its default, Details edits them, undo covers them, and the Outliner's "+" or Details'
**Add Component** creates it — with no code outside the class. Settings are typed props; attributes stay
as free-form data on any actor.

### What game code calls

- **The scene** is plain C++: `Scene::spawn<T>(parent)` and `spawn("Class", parent)`, `addComponent`,
  `clone`, `find`, `byId`, `actor`, `component`, `destroy`, `Actor::attachTo`, `findAttached`,
  `findComponent<T>()`, `addComponent<T>()`, attributes, and each class's own setters. A
  `Scene::Ref<T>` prop names another actor or component, set in Details, and `get(*scene())` resolves it.
- **`Game/Game.hpp`** declares free functions over the running engine, found through
  `Engine::current()`: `time`, `quit`, `keyDown`/`keyPressed`/`keyReleased`, `mouseDown`/`Pressed`/
  `Released`, `mousePosition`, `mouseDelta`, `scroll`, `setCursorLocked`, `loadTexture`, `textureSize`,
  `screenSize`, `screenToWorld`, `drawSprite`, `drawSpriteRegion`, `drawRect`, `newCube`, `drawMesh`,
  `raycast`, `world()` for gravity, and `random`. `Game::quit()` only sets a flag: the player ends on
  it and the editor turns it into Stop. `time` is scene time, so it stops while paused and advances
  one fixed step per Step. `screenToWorld` takes window points relative to the view and
  returns the point on the `z = 0` plane, through the scene's camera, never the editor's.
- **Events** are actor and component hooks — `beginPlay`, `tick(dt)`, `onRender(alpha, draws)`,
  `endPlay`, `onAttributeChanged`, `onChildAdded`, `onChildRemoved`, `onComponentAdded`, `onTouched`,
  `onTouchEnded` — plus `Scene::connect` to hear another object and `Scene::after`/`every` for timers;
  see *The scene*. A connection or timer belongs to the scene, never to the library, which is what makes
  unloading safe.

### Modules

- **`Core/Module.hpp`.** `CINDER_REGISTER(T)` appends to a list behind a hidden-visibility inline
  function, so each library has its own. `CINDER_MODULE_ENTRY()` defines the two exports,
  `cinder_module_abi` and `cinder_module_register`; `cbt` generates the one file that expands it.
  Registration goes through a `Registrar`, which refuses a name the engine or another class already owns.
- **`Core/GameModule`** `dlopen`s a library and refuses it unless its `cinder_module_abi` matches the
  engine's **BuildId** — see below. `enlist` registers its classes;
  the destructor calls `ClassRegistry::remove` for each — dropping cached default instances, which live in
  the library's code — and only then `dlclose`s. `locate` finds `Binaries/<Platform>/<name>.dylib`
  (packaged) or `Saved/Binaries/<name>.dylib` (the editor's build); `.so` on Linux.
- **The engine is exported to libraries.** `editor`, `player` and `tests` are targets with
  `exportSymbols()`: they link every engine object directly and pass `-export_dynamic` (`-rdynamic` on
  Linux), so any engine symbol a library calls exists; libraries link with `-undefined dynamic_lookup`.
  A Shipping package strips `player`, which keeps the exported symbols and drops the rest.
- **The BuildId** is a hash of the engine's sources under `Source/` — `ThirdParty/` included, and
  `Dev/`, `Editor/`, `Packager/` and `Programs/` left out, since game code never links them — its
  version, backend, configuration and compiler. `cbt` computes it only when it plans an engine build;
  `cbt game` reads it from `Engine.modules`. `cbt build` writes it to
  `Binaries/<Platform>/<Config>/Engine.modules` and compiles it into the engine through a generated
  `BuildId.cpp`, and `cbt game` passes it to game code as `CINDER_MODULE_ABI`. Because it is part of
  every game compile command, game code built against an older engine is recompiled by the next
  `cbt game` rather than failing with a missing symbol; a library that is never rebuilt is refused and
  the scene loads with placeholders.
- **`player`** loads the library before it opens the scene, and clears the scene before unloading it.

### Building and hot reload

- **`cbt game <project>`** builds a project's C++ as a game-module target. A project with no rule files
  gets one module, `Project.<name>`, holding every `.cpp` under `Source/` and depending on `Game`; a
  project with `*.Build.cpp` files under `Source/` has them compiled into a library `cbt` loads first,
  and a project module named like an engine module is an error. Objects go to
  `Saved/Intermediate/<Platform>/<Config>/` with `-MMD` depfiles, next to a generated
  `ModuleEntry.cpp`; the library goes to `Saved/Binaries/<name>.dylib` (`.so` on Linux), or to
  `--library <file>`. It writes `compile_commands.json` at the project root for clangd. It needs the
  engine's `Engine.modules`, so the engine must be built for the same configuration first.
- **`Dev/GameBuild`** finds `cbt` at `Binaries/<Platform>/cbt`, one folder above the running
  executable, and spells the `cbt game` and `cbt build` command lines for the executable's own
  configuration and backend. The editor and the packager compile nothing themselves.
- **`Dev/Process`** runs a child through `posix_spawn` in its own process group with default signal
  handlers, reads its output by line, and can terminate the whole group. Its pipes are close-on-exec, so
  a child never inherits another child's output. `ModuleHost`, `Staging` and the packager use it to run
  `cbt` and build steps, and `Dev/BackgroundProcess` runs one on a worker thread for `ModuleHost` and
  `Packager`, queueing its lines for the main thread.
- **`Dev/ModuleHost`** is the editor's owner of all this. The editor runs `cbt game` and loads the
  library synchronously **before the scene opens**, so the scene's classes exist when it loads.
  **Build** on the toolbar and **⌘B** run it in the background; `update` relays its output as `[build]`
  lines on the main thread, counts compiles and errors from it, and marks a reload pending when the
  library was relinked. Cancelling sends `SIGTERM` to `cbt`'s whole process group. The editor applies it between frames, in Edit mode only and
  never mid-interaction — during Play it waits for Stop:
  1. copy the library to `Saved/Binaries/Loaded/<name>-<n>.dylib`, because `dlopen` caches by path;
  2. `dlopen` the copy and check its ABI;
  3. save the scene to text and `Scene::clear()` it, running the old classes' destructors while their
     code is still loaded;
  4. destroy the old `GameModule`, then enlist the new one and load the text back.

  Any failure leaves the old library loaded. Selection, history and the Outliner's rows survive,
  because they are all keyed by id or text; `Details::rebuild` runs after a swap, because its rows
  hold `PropDef`s that lived in the old library. Statics in a library reset on reload and nowhere else.
- **`Tests/hot_reload_test`** compiles a real two-version library through `cbt game` and swaps it;
  `cbt test` runs it as its own `hot_reload` check because it takes seconds.

### Attributes

Per-actor free-form data: a `PropRec` on `Actor`, saved as an `attributes { ... }` block and edited in
Details. `Scene/Attributes` decides what one may hold — a number, string, bool, or a sequence of 2–4
numbers — and what a name may be: letters, digits and `_`. A change emits `Signal::AttributeChanged`
only when the value really changes, and `loadAttributes` never notifies.

### The selftest

`Tests/selftest` is a project whose `SelfTest` component checks, from inside a game library, the engine
behaviour game code relies on — scene editing, reflection, attributes, signals, timers, classes created
by name at runtime, and physics — and prints `ALL PASS`. The editor builds it: `$B/editor
Tests/selftest --frames 2`, then `$B/player Tests/selftest`. It needs a window, so it is not part
of `cbt test`.

## Reflection

There is no runtime reflection in C++, so props are declared with a macro that stringises the field
name — the identifier *is* the serialization key:

```cpp
CINDER_COMPONENT(CameraComponent, cinder::Scene::SceneComponent) {
    CINDER_PROP(projection_);
    CINDER_PROP_S(fov_, 1.0f, 179.0f, 1.0f);
    CINDER_PROP_R(zoom_, 0.05f, 20.0f);
}
```

`CINDER_PROP_R` adds a clamp range, `CINDER_PROP_S` a range and a drag step, and `CINDER_PROP_COLOR`
marks a `vec3`/`vec4` as a colour through `PropHint`, and `CINDER_PROP_ANGLE` marks radians to show in
degrees. Only Details reads the step and the hint; neither changes how a prop is written or saved, and
neither a colour nor an angle is clamped.

**A reference is a prop too.** `Scene::Ref<T>` holds an actor's or a component's id and nothing else, and
`CINDER_PROP(target_)` declares it like any field. `Reflect` is a leaf and cannot name `Scene`, so it
knows a reference only as `PropType::Ref` — any type derived from `Reflect::RefBase` — read and written
as an `int` through `readInt`/`writeInt`, plus `PropDef::accepts(const void*)`, which takes an `Object*`
and runs the field's `dynamic_cast` to `T`. `ref.get(scene)` looks the id up on every call and returns
null when it is 0, gone, marked destroyed or not a `T`, so a reference never dangles: deleting the
target, Stop and a hot reload all just change what the lookup finds. `Scene::forEachRef` visits an
object's reference props.

`props<T>()` builds the list once into a function-local static and a `PropChain` recursion emits the
base class's props first, so `Object::enabled` is always the first prop of every class —
which the delta serializer's field order depends on. `PropBuilder` is templated on the **concrete**
type, so inherited props carry no base-subobject offset assumption.

Four behaviours are easy to lose, and each has a test pinning it:

- **Vector writes mutate in place.** `Transform::position()` hands out a live reference; assigning a
  fresh vector would orphan every held one.
- **Clamping is write-only**, so a save never clamps live values.
- **Enums serialize as their lowercase name**, and an unknown constant is silently rejected — no
  write, and critically no notify. `CINDER_ENUM_NAMES` declares the table; comparison is ASCII-only, so
  locale independence is true by construction.
- **`PropSink::propChanged`** fires after every successful write; `Transform` overrides it to
  `dirty()`, and the renderers override it to drop a cached texture or mesh handle.

`ClassRegistry` is an instance owned by `Engine`, threaded to `Scene` -> `SceneCodec` / `SceneApi`. It
keeps insertion-ordered iteration (a vector plus two indices) and caches a class-default instance per
type for delta encoding. Re-binding a name keeps its slot — so iteration order is stable across a
reboot — and drops its cached default, so a default instance never outlives the library whose code
it runs.

`SceneCodec::VERSION` and `OLDEST` are both 5 — see *The scene file*. `Tests/scene_files_test` loads
and re-saves every `.scene` and every project's `Config/Game.ini` under `Samples/` and
`Tests/selftest/`, and requires the bytes to match, so shipped scenes and settings stay canonical as
the formats move.

## Projects and packaging

A project is a folder in Unreal's shape:

```
MyGame/
  MyGame.cinder      the project descriptor, Unreal's .uproject — marks the folder as a project
  Config/Game.ini    settings
  Content/           scenes, textures, meshes — Scenes/ and Textures/ by convention, not by rule
  Source/            the game's C++, built into one library
  Saved/             per-user and generated; gitignored, never packaged
```

`ProjectConfig::root` takes the folder or its `.cinder` file. `ProjectConfig::load` requires exactly
one `*.cinder` in the folder, parses it into `config.descriptor`, then walks `Config/Game.ini`
through `IniLoad`; a project with no `Game.ini` runs on defaults. `ProjectConfig::walk` serves both
directions, like `SceneCodec::walk`, and `IniSave` writes only values that differ from the defaults and
drops a section left empty — so a sample's `Game.ini` is usually just its title.

**The `.cinder` file is the project descriptor**, Unreal's `.uproject` with camelCase keys: JSON that
says what the project is and what it is made of, while `Config/` says how it is set up.

```json
{
	"fileVersion": 1,
	"engineAssociation": "0.1.0",
	"category": "Games",
	"description": "A short description for the project picker.",
	"modules": [
		{
			"name": "Gameplay",
			"type": "Runtime",
			"loadingPhase": "Default"
		}
	],
	"plugins": [
		{
			"name": "Physics",
			"enabled": true,
			"targetAllowList": [
				"Editor"
			]
		}
	],
	"targetPlatforms": [
		"macOS",
		"Linux"
	],
	"postBuildSteps": {
		"macOS": [
			"codesign --force -s - $(StageDir)/player"
		]
	}
}
```

`Core/ProjectDescriptor` carries every `.uproject` field that means something outside Epic's own
tooling — `EpicSampleNameHash` and `IsEnterpriseProject` are left out — and not all of them are read
yet:

- **`fileVersion`** must be `ProjectDescriptor::FILE_VERSION`, 1.
- **`engineAssociation`** is the cinder version the project was made with. One that differs from
  `ProjectDescriptor::engineVersion()` — `cbt`'s `ENGINE_VERSION`, baked in as `CINDER_VERSION` —
  logs a warning. Empty means the project lives in the engine's tree, which is Unreal's convention
  and what the samples do.
- **`targetPlatforms`** (`macOS`, `Linux`, `Windows`; empty means all) and **`preBuildSteps`** /
  **`postBuildSteps`**, a platform name to command lines, are read by the packager, below.
- **`category`** and **`description`** are for the project picker.
- **`modules`**, **`plugins`**, **`additionalRootDirectories`**, **`additionalPluginDirectories`** and
  **`disableEnginePluginsByDefault`** are parsed, type-checked and saved, and nothing reads them: they
  wait for a plugin system and for `require`.

`ProjectDescriptor::save` writes Unreal's layout — tabs, the first four keys always, everything else
only when it is set — and `Tests/scene_files_test` holds the shipped descriptors to it. A wrong type,
or a module or plugin without a `name`, is an error that names the key, and `ProjectConfig::load`
prefixes the file name. `Serial/Json` is the format: `parseJson` builds a `PropValue` — objects are
`PropRec`, arrays `PropSeq`, and `null` is rejected — and reports errors as `line:column`, and
`JsonWriter` streams, so the descriptor rather than a sorted map decides the key order.

`Serial/IniLoad` and `Serial/IniSave` are the `Archive` for INI: a record is a `[Section]`, a field is
`key=value` with camelCase keys like props, a value runs raw to the end of its line, and numbers go
through `from_chars`/`to_chars`. INI has no arrays, attribute bags or nested sections, and asking for
one throws `std::logic_error`.

`Platform/Assets` is the only thing that turns a name into a path, and it has four roots:

- **`enginePath`** — engine data, such as `Content/Fonts/`. `CINDER_ENGINE` if set; else `Engine/`
  next to the executable, if it exists, which is the packaged layout; else the repo root, baked in by
  `cbt` as `CINDER_ENGINE_DEFAULT`.
- **`shaderPath`** — the compiled shaders, which are built into `Binaries/` rather than beside their
  HLSL. `CINDER_SHADERS` if set; else `Engine/Shaders/` next to the executable, if it exists, which is
  the packaged layout; else `Binaries/<Platform>/<Config>/Shaders/`, baked in as
  `CINDER_SHADERS_DEFAULT`; else `engineRoot() / "Shaders"`. The packaged layout deliberately beats
  the baked build directory, so a packaged game run on the dev machine never reads the build tree.
- **`contentPath`** — `Content/`: the scene to open, `SpriteComponent.texture`, `StaticMeshComponent.texture` and
  `Game::loadTexture`.
- **`sourcePath`** — `Source/`: the game's C++, which `cbt game` builds.

The loader picks the root, so a path never names it — a scene says `texture "Textures/box.png"`. Both roots sit under the project root, which is made absolute, so a project
folder can live anywhere and runs from any working directory; the player falls back to `Project/`
next to itself. A path that is absolute or climbs out of its root with `..` throws: a texture logs
and draws white, and `Game::loadTexture` throws. It would work in the editor and break
once packaged. A path that exists but differs in case from the disk logs `[assets] ... differs in
case`: macOS is case-insensitive by default, and the same project would not find the file on a
case-sensitive system.

**Packaging is C++.** `Dev/Staging` does it, and two things call it: the `packager`
executable, and the editor's **Platforms** menu, which runs that executable as a child process.
`stage(request)`:

1. loads the project through `ProjectConfig` — the descriptor's `targetPlatforms`, `preBuildSteps` and
   `postBuildSteps` — and refuses a platform the project does not target;
2. runs `preBuildSteps` for the platform, each through `sh -c` in the project folder with
   `$(ProjectDir)`, `$(EngineDir)` and `$(StageDir)` expanded; a step that fails stops the package;
3. builds the project's C++ with `cbt game --config <Config> --library
   Saved/Binaries/Package-<Config>/<name>.dylib`;
4. wipes the output and copies `player`, keeping its permissions, into a folder shaped like the repo:
   `Engine/Shaders/` with the compiled shaders the targets need — macOS `.spv` and `.metallib`, iOS
   `.metallib`, Linux and Android `.spv`, Windows `.dxil`, all three when no platform is named, a
   missing one a warning; `Engine/Content/Fonts/`; and `Project/`, holding the `.cinder` file,
   `Config/`, `Content/` and the library in `Binaries/<Platform>/` — **never `Source/` or `Saved/`**:
   a package ships compiled code only;
5. for **Shipping**, copies the unstripped `player` and library to `Saved/Symbols/<Platform>/` and
   `strip`s the shipped ones — Unreal's split, where the symbols stay with the developer;
   **Development** ships them as built;
6. runs `postBuildSteps`.

`packager <project> [--platform macOS] [--config development|shipping] [--out dir]` defaults to the
host, Development and `<project>/Saved/Builds/<Platform>/<name>/`. When it finds `cbt` it first runs
`cbt build player --config <Config>`, so a Shipping package ships a Shipping `player` and that
configuration's shaders; without `cbt` it falls back to the `player` and shaders next to itself.

**The editor's Platforms menu** has a Development/Shipping check and one entry per platform in
`Dev/Staging` — macOS, Linux, Windows, iOS and Android. `Dev/Packager` runs `packager` through
`Dev/Process`, in a process group of its own, and reads its output on a worker thread; the lines are
queued and logged as `[package]` from `Toolbar::update`, on the main thread, so the console sink is
never called off it. Only the host's own desktop platform is enabled; the others are listed, disabled,
with a tooltip saying why, and a project whose `targetPlatforms` leaves a platform out disables it too.
Adding a platform means making `platformSupported` true for it and teaching `stage` its layout.
Packaging is an Edit-mode action and packages what is on disk, so a dirty scene first gets the Save /
Don't Save / Cancel dialog, worded for packaging. `Packager::cancel` sends `SIGTERM` to the whole group
— the packager, its compilers and its build steps — and the destructor cancels rather than waits;
closing the editor mid-package asks to Stop and Close or Keep Packaging.

## Conventions

- **Zero comments.** No comments or doc blocks anywhere, by choice. Match it; explain in chat or in
  these docs.
- Members carry a trailing underscore. `CINDER_PROP(position_)` strips it, so the wire key stays
  `position`. This is also why `CameraComponent`'s clip planes are `near_`/`far_` — `near` and `far` are
  macros in `windef.h`.
- `Glfw` is refcounted `acquire`/`release`. `Window` acquires in its constructor and releases in its
  destructor, and `main` holds an outer acquire across the whole run.
- `Input` is edge-triggered: `keyPressed`/`keyReleased` are true for exactly one fixed update,
  cleared by `input.consume()` at the end of `Engine::update` and on every `GameLoop::idle` frame.
- **`std::to_chars` everywhere in `Serial`** — never `printf` or `ostream`, which follow the locale.
  A test pins the output under a Turkish locale, and `IniLoad` and `parseJson` parse with
  `std::from_chars` for the same reason.
- Sizes and view dimensions come in two flavours and they are not interchangeable: the swapchain and
  render target are in **framebuffer pixels**, while `ViewCamera` and `screenToWorld` are in
  **window points**. On a Retina display these differ by 2x. GLFW reports cursor positions in points,
  which is why the cameras use them. In the editor the view is the Scene panel rather than the
  window, and `setViewportSize` takes points and converts to pixels itself.
