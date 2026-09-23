# CLAUDE.md

Guidance for Claude Code (claude.ai/code) when working in this repository.

## What this is

A from-scratch C++20 game engine: Vulkan renderer, GLFW windowing, Lua 5.4 scripting. The long-term
target is a Unity/Unreal-shaped editor workflow — select a node, edit its fields, hit Play, hit
Stop, land back where you started. `TODO.md` is the authoritative roadmap; read it before proposing
architectural work, since it records what is deliberately deferred and what is out of scope.

The repo holds the engine only. A game is a **project folder** — a `.cinder` file, `Config/`,
`Content/` and `Source/` — that the editor opens and the player plays; see *Projects and packaging*.
`samples/` holds three example projects; nothing in them is compiled.

## Commands

```bash
cmake -S . -B build -G Ninja && cmake --build build
```

A plain build produces `engine`, `engine_dev`, `editor`, `ui_gallery` and `tests` — **no game**. The
runtime that plays a project, `player`, is `EXCLUDE_FROM_ALL` and only built on demand.

```bash
DYLD_LIBRARY_PATH=/opt/homebrew/lib ./build/ui_gallery --frames 10 --capture-window gallery.png
```

`ui_gallery` is the test bench for the new UI, linked against `engine` alone: every widget, live, on
one side of a splitter, and everything the UI renderer can draw on the other. `--capture-window` writes
the swapchain image, UI included, which `--capture` never does; `--input-script <file>` replays input
instead of the OS's (see *The UI framework*); `--lowdpi` turns off framebuffer scaling so text can be
judged at 1x on a Retina display, and `--text-gamma` tunes glyph coverage.

```bash
ctest --test-dir build --output-on-failure
```

`ctest` builds `player` itself through a fixture, because `shipping_binary_is_clean` inspects it.
It also runs `architecture_layers`, which checks first-party includes against the layer graph, and
`architecture_checker`, which exercises allowed and forbidden include fixtures.

The optional graphical integration check builds only when requested and needs a working window and
Vulkan device:

```bash
cmake --build build --target graphics_smoke
DYLD_LIBRARY_PATH=/opt/homebrew/lib ./build/graphics_smoke
```

It runs the editor's own UI and drives it with injected input: it clicks a node in the Explorer, drags
a move gizmo in the Scene view, undoes and redoes with ⌘Z, saves a scene, plays and stops with ⌘P,
cycles Play/Pause/Step/Resume/Stop through fresh Lua states, re-docks the console onto the Scene
stack, resizes the window and scene targets, and captures both the window and the composited view. Its output stays in `build/graphics-smoke/`;
it never saves over a sample project. On other platforms omit the Apple-specific library path.

```bash
DYLD_LIBRARY_PATH=/opt/homebrew/lib ./build/editor samples/sandbox2d
```

`editor` is the **dev build** — the same engine plus the editor UI, which is our own (see *The UI
framework*): one window of docked tabs, the Explorer on the left, the Scene view in the middle with
the console under it and Properties on the right, over a toolbar carrying the File, Edit, Window and
Play menus. Drag a tab to another stack's middle or edge to re-dock it, close it, and reopen it from
the Window menu. It opens a project in **Edit mode**: the scene is loaded and drawn, and no game code
runs. Play/Pause/Step/Stop are on the toolbar and on ⌘P, ⌘⇧P and ⌘⌥P; ⌘S saves the scene, and ⌘Z and
⌘⇧Z undo and redo edits. In the Scene view 1, 2 and 3 pick the move, rotate and scale gizmos. It takes
the project folder or its `.cinder` file, then `--scene <path>` (relative to `Content/`), `--play`
(start in Play), `--frames <n>`, `--capture <png>`, `--capture-window <png>` and `--input-script
<file>`; a script can also `close` the window to exercise the unsaved-changes prompt. `--frames 90
--capture out.png` runs headless-ish and writes a screenshot. See *The dev tools* below for why this
is a second executable rather than a flag.

```bash
cmake --build build --target player && ./build/player samples/sandbox3d
```

`player` is the generic runtime: it opens a project and plays it at once. It takes the same
`--scene`, `--frames` and `--capture`; with no project argument it looks for `project/` next to its
own executable, which is the packaged layout.

```bash
cmake --build build --target package_game
```

Stages a runnable game in `build/dist/<project>/` — see *Projects and packaging*.

Note that `--capture` reads back the **scene render target**, not the swapchain, so the editor's own
UI never appears in a capture; `--capture-window` is the one that shows it. That is deliberate: it is the game's picture, not the editor's. In
the editor that target is the size of the Scene panel, not the window, and in Edit mode its view is
the editor camera's — which starts as a copy of a perspective scene camera, so an unattended `--frames
--capture` still writes the game's frame. An orthographic scene starts from a perspective view framing
the same rectangle instead. See *The editor camera*.

The first configure fetches every dependency and needs network — glfw, glm, lua, VMA, stb, volk,
Vulkan-Headers, doctest, FreeType and HarfBuzz, all pinned in `cmake/Dependencies.cmake`.
Nothing needs installing: FreeType is built with zlib, bzip2, PNG, Brotli and HarfBuzz switched off, so
it never finds Homebrew's copies, and HarfBuzz is compiled from its single-file `src/harfbuzz.cc`
rather than through its community-maintained CMake build.
`glslangValidator` is the one exception: it is a *build tool*, found with `find_program`, and it
compiles `engine/shaders/*.{vert,frag}` to `.spv`. Editing a shader needs a rebuild, not just a
restart. `brew install glslang` if it is missing.

`player/main.cpp` and `editor/main.cpp` duplicate their arg parsing on purpose, and their loops have
diverged: the player only ever calls `GameLoop::tick`, while the editor calls `PlaySession::tick`,
which picks `tick`, `idle` or `step`. `GameLoop` is the shared part. Do not factor the duplication
back into `core`.

## Vulkan on Apple Silicon

Two separate failures share one root cause: **Homebrew's `/opt/homebrew/lib` is not on the dyld
default search path, and both volk and the Vulkan loader `dlopen` by leaf name.**

- `volkInitialize()` fails because volk hardcodes `/usr/local/lib`, the Intel prefix. `Glfw::acquire`
  walks its own candidate list and falls back to `volkInitializeCustom`.
- The validation layer *enumerates* but `vkCreateInstance` returns `VK_ERROR_LAYER_NOT_PRESENT`,
  because the layer manifest's `library_path` is a bare `libVkLayer_khronos_validation.dylib`.
  `VkCtx` retries without the layer and prints a notice.

To actually get validation, put the prefix on the dyld path:

```bash
DYLD_LIBRARY_PATH=/opt/homebrew/lib ./build/editor samples/sandbox2d
```

**Do this whenever touching the renderer.** Without it you are running unvalidated — it has already
caught a missing `TRANSFER_SRC_BIT` that no test would have.

## Layout

```
cinder/
  CMakeLists.txt
  cmake/          dependency, Lua, shader-compilation and packaging modules
  engine/         engine data: shaders/ (GLSL and the compiled .spv), lua/ (the prelude) and fonts/
                  (Roboto and Roboto Mono, OFL-1.1, licences beside them)
  samples/        sandbox2d/, sandbox3d/ and physics/ — example projects
  src/
    reflect/ lua/ platform/ text/    leaves
    scene/ serial/ components/       the world model
    physics/                         rigid bodies, collision and the solver
    ui/core/ ui/framework/ ui/widgets/ ui/docking/  the UI framework the editor is built on
    gfx/ gfx/vk/ gfx/asset/ gfx/pass/  the renderer
    script/ core/                    the Lua host and the engine
    dev/                             editor panels, gizmos, history, play session; NOT part of `engine`
    player/ editor/                  the two executables
  tests/
    selftest/                        a project whose scene runs the Lua smoke test
```

`src/` is the only include root, so every include carries its layer: `#include "scene/Node.hpp"`.
Headers sit next to their sources.

## Layering

The layer graph is **acyclic and strictly layered**. Keep it that way — this is what lets the editor
depend on `scene` + `reflect` without dragging in Vulkan or Lua.

```
reflect, platform          -> leaves (no engine includes)
lua                        -> platform
text                       -> platform
scene                      -> reflect
serial                     -> scene, reflect, platform
components                 -> scene, reflect
physics                    -> scene, reflect, lua
ui/core                    -> text, platform
ui/framework               -> ui/core, text, platform
ui/widgets                 -> ui/framework, ui/core, text, platform
ui/docking                 -> ui/widgets, ui/framework, ui/core, text, platform
gfx/vk                     -> platform
gfx/asset                  -> gfx/vk, scene
gfx/pass                   -> gfx/asset, gfx/vk, scene, lua, platform
gfx                        -> gfx/pass, gfx/asset, gfx/vk, ui/core, text, scene, platform, lua
script                     -> scene, reflect, gfx, physics, platform, lua
core                       -> all of the above
dev                        -> core and all of the above (a separate target, see below)
```

Everything down to `core` is the `engine` library. **`dev` is not** — it is its own target,
`engine_dev`, and it holds everything editor-only: the panels in `dev/panels/`, the Scene view with
its editor camera and gizmos, the selection, the undo history and the play session. `player` links
`engine`; `editor` links `engine_dev`.

**The editor's UI is the engine's own** — see *The UI framework* below. `text` and the four `ui`
layers are part of `engine`, because the game will use them too; `ui` never includes `scene`,
`reflect` or `gfx`. `ctest` runs `nm` over `player` to prove no `cinder::dev` symbol reached it.

**`platform` is the one leaf everything may reach for**, because `platform/Log.hpp` lives there and
every layer logs, and `platform/Assets.hpp` is the only thing that turns a name into a path. That is
the only reason `serial`, `lua` and `gfx` have an edge to it, and neither header includes anything
from the engine, so the edges cost nothing and create no cycle.

`gfx` is split four ways and the seam that matters is `gfx/vk`: it knows Vulkan and knows nothing
about this engine, so a new backend can be built on it without dragging in passes or assets. `gfx/asset` is what you draw with (`Assets`, `Texture`, `Mesh`),
`gfx/pass` is how you draw it (`DrawPass` and its implementations, their pipelines, and `ViewCamera`), and
`gfx` itself contains orchestration and frame resources: `Renderer`, `FrameTargets`, `RenderTarget`,
`Capture`, `UiRenderer`, `CompositePipeline` and `RendererDrawList`. Nothing in a subdirectory includes
its parent. `cmake/AssertLayers.cmake` enforces these include directions through CTest. `ui` is split
the same way, and the same rule holds: `ui/core` never includes `ui/widgets`.

Five placements are load-bearing and were each chosen to kill a cycle: `Glfw`/`Window`/`Input` live
in `platform`, not next to `Engine`; `DrawList` lives in `scene`, so the scene graph never includes
`gfx`; `LuaApi` lives in `lua`, so a pass can bind its own functions without including the script
host; `PropValue` lives in `scene`, not `serial`, so a node's attributes are scene data that the
serializer, the Lua bindings and Properties each read without an edge to one another;
and `SceneObserver` is an interface in `scene` with its only implementation in `script`, so the tree
can announce changes to Lua it cannot name.

`LuaHost` takes `(Scene&, Input&, Renderer&, physics::World&, quit)` — never `Engine&` — for the same reason.

`platform/Files` holds the shared binary-mode text I/O used by project loading, scene files, Lua
source loading and editor history. Callers still decide whether to create parent directories and
whether to verify completion after closing a write. `serial/NumberText` holds locale-independent
numeric formatting shared by the text and INI writers; their parsing and wire formats are unchanged.
`reflect::copyProps` copies through each property's existing writer, including clamping and notifications.

## Frame flow

`main` -> `Glfw::acquire()` -> `Engine(config)` -> `engine.openScene(path)` -> a loop.

`Engine`'s constructor boots Lua but **loads nothing and runs nothing**; the host decides what
happens next. The player calls `GameLoop::tick` every iteration. The editor calls
`PlaySession::tick`, which calls one of three `GameLoop` entry points:

- **`tick`** — the simulation:
  1. `Glfw::pollEvents()`, then `engine.beginFrame()` -> `script->poll()` (script hot reload)
  2. minimized -> `Glfw::waitEvents()`, reset the clock, skip the frame
  3. `GameLoop::advance()` accumulates real time, clamped at `MAX_FRAME_TIME = 0.25s`
  4. `engine.update(fixedDt)` N times — fixed timestep; `script->update`, `scene.update`,
     `physics.step`, then `input.consume()`
  5. `engine.render(alpha())` once — `renderer.beginFrame()`, `script->render(alpha)`,
     `scene.render(alpha, draws)`, `renderer.drawFrame()`
- **`idle`** — Edit mode and Paused: steps 1–2, then it resets the clock, calls `input.consume()` and
  renders. Consuming on idle frames is load-bearing: without it, the key that pressed Play, or an
  Esc from Edit mode, would fire as `keyPressed` on the first play step.
- **`step`** — exactly one fixed update, then render. The Step button.

`Engine` is a library that gets ticked, not something that runs itself. **Do not move the loop into
`Engine`.** `GameLoop::advance()` and `alpha()` are public precisely so the tests can drive them
without a window; keep new loop logic in that shape.

`ScriptHost::update` calls the Lua global `__step(dt)` with the fixed `dt`, and `render` calls
`__render(alpha)`. Both live in `engine/lua/task.lua`, which steps the coroutine scheduler and fires
the `stepped` and `rendered` signals.

## The scene: a tree of nodes

The world is **one tree of nodes**, the way Roblox and Godot build it. A node is exactly one class,
and things are made by nesting: a Box is a `MeshPart` with a couple of `Script`s inside it. There
are no components. Behaviour is a script, not a node class: a node class exists when the engine has
to know about it — something to draw, to simulate, to organize — and everything else is Lua.

- **`scene/Node`** holds an id, a name, a parent, children, `enabled`, attributes, and the lifecycle
  hooks `onStart`, `onUpdate`, `onRender` and `onDestroy`. `CINDER_PROPS(Node, void)` declares
  `enabled`, so it is the first prop of every class.
- **`scene/Spatial`** is a `Node` that owns a `Transform`; `Node::transform()` is null on every other
  node. A `Transform` composes with its parent only when the parent is spatial, so a `Folder` breaks
  the chain and its children sit in world space.
- **`scene/NodeTypes`** is the class registry — a name, a factory, the prop list and a default
  instance per class for delta encoding. Re-binding a name keeps its slot, so iteration order, and
  the Explorer's insert menu, is stable across a reboot.
- **`scene/Scene`** owns every node by id. `insert` takes a node and an optional id — the codec's
  path — `create` builds one by class name, and `clone` copies a subtree through the registry, props,
  transform and attributes, with fresh ids. A node inserted without a name takes its class name.

Lifecycle rules that are easy to lose:

- Every inserted node queues `onStart`, which runs at the top of the next `Scene::update`, disabled
  or not; a node inserted during a start is started in the same pass.
- A disabled node skips itself **and its subtree** in `update` and `render`. It does not stop the
  scripts under it: a `Script` stops only when its own `enabled` is written.
- `destroy` is deferred to the end of `update`. `destroyNow` tears a subtree down at once and exists
  for the editor, which never calls `update` in Edit mode; never call it from inside an update walk.
- `Scene::clear` does not notify the observer, so loading a scene is not a stream of destroy events.

`SceneObserver`, declared in `scene/Scene.hpp`, is how the tree announces `attributeChanged`,
`childAdded`, `childRemoved` and `destroying` without naming Lua. `script/SceneApi` implements it and
`LuaHost::boot` installs it. An attribute notifies only on a real change — `sameAttribute` counts `2`
and `2.0` as equal — and `loadAttributes` never notifies.

| Class | Base | Props | |
|---|---|---|---|
| `Group` | Spatial | — | moves its children |
| `Folder` | Node | — | organizes; breaks the transform chain |
| `MeshPart` | Spatial | `mesh`, `texture` | |
| `Sprite` | Spatial | `texture`, `size`, `color` | a quad in its local XY plane |
| `Camera` | Spatial | `projection`, `fov`, clip planes, `zoom`, `clearColor`, `virtualSize` | `projection` is all that makes a scene 2D |
| `Body` | Spatial | `motion`, `mass`, `gravityScale`, damping, `velocity`, `planar` | physics moves it; lives in `physics` |
| `Collider` | Spatial | `shape`, `size`, `friction`, `restitution` | a shape of its nearest `Body`; see *Physics* |
| `Script` | Node | `file` | lives in `script`; see *Scripting* |

A new class derives from `Node` or `Spatial`, declares its props with `CINDER_NODE`, and is
registered — engine classes in `components::registerBuiltins`, physics ones in
`physics::registerNodes`. The serializer, Lua and the editor
need no change.

### The scene file

`serial/SceneCodec` writes version 4:

```
version 4
nodes {
    MeshPart {
        id 2
        name "Box"
        position -6 0 -6
        attributes {
            phase -8
        }
        children {
            Script {
                id 3
                file "spinner.lua"
                attributes {
                    speed 0 0.5 0
                }
            }
        }
    }
}
```

An item's block name is its class. A node writes `id`, `name` (omitted when it is the class name),
its transform if it is spatial, its class props where they differ from the class default,
`attributes`, then `children`. An unknown class is logged and skipped along with its subtree, and a
value an attribute cannot hold is logged and dropped. `VERSION` and `OLDEST` are both 4: the
actor/component formats are not read, and the repo's scenes were converted once.

## Edit mode and Play mode

A game is a `.scene` file — a tree of nodes, some of them `Script`s. There is no entry script.

Nothing runs a script until something calls `Scene::update`: `onStart` fires from `startPending()`
at the top of the first update, and that is when a `Script` runs its file. `Scene::render` does
**not** wait for `start` — it draws every enabled node — so a scene that is never updated is still
fully drawn. That split *is* Edit mode: meshes, sprites and cameras draw, and scripts never run. The one thing the editor draws differently is the view: Edit mode looks
through its own camera and shows the scene's cameras as gizmos — see *The editor camera*.

`dev/PlaySession` owns the editor's state, `Edit | Playing | Paused`:

- **Play** saves the scene to text with `SceneCodec::save` and hands it to `Engine::loadScene`, which
  is `scene.clear()`, then `LuaHost::boot()` — a fresh `lua_State` — then `SceneCodec::load`. Play
  therefore takes exactly the path the player takes from disk. Every session starts with fresh
  script sources and no leftover coroutines, and a serialization bug shows up on Play rather than on
  Stop.
- **Stop** hands the same text to `loadScene` and releases a locked cursor. Play and Stop are one
  operation; only whether the loop simulates afterwards differs.
- **Pause** switches to `GameLoop::idle`; **Step** runs one `GameLoop::step`.

The order inside `loadScene` is load-bearing: every `Script` holds the `lua_State*` it was created
with, so the scene must be cleared before `boot()` closes that state.

Toolbar buttons and shortcuts only set a request, and `PlaySession::tick` applies it **between
frames**. The toolbar draws inside `Engine::render`, and `stop()` closes the `lua_State`, which must
never happen inside a Lua call. For the same reason, game code calling `engine.quit()` only sets a
flag. The player ends on it; `PlaySession` turns it into Stop, the way Unity ignores
`Application.Quit` in the editor.

Save is disabled during Play: the scene on screen is the running game, not the document.

## Rendering

`Renderer` owns two `VkRenderPass`es and 2 frames in flight. The **scene pass** (color + depth) draws
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

`DrawPass` (`beginFrame` / `record` / `registerApi`) is the extension point. It is named `DrawPass`,
not `RenderPass`, so that `renderPass` unambiguously means a `VkRenderPass`. A new pass means:
implement the interface, add it to `passes_` in the `Renderer` constructor in draw order, and let it
register its own Lua functions. A pass owns no camera: `record` is handed the frame's view-projection.

**There is one camera, and 2D and 3D differ only in its projection** — Unity's model. `Renderer` owns
a `gfx/pass/ViewCamera`, and the scene's `Camera` writes it every frame through `DrawList::camera` as a
`scene::View`: the world matrix, `orthographic`, fov, clip planes, zoom and virtual size. A perspective
view uses the fov at the view's aspect. An orthographic one covers `virtualSize / zoom` world units —
or the view's own size in points when `virtualSize` is zero — centred on the camera and stretched to
the view. Both clip at `near` and `far`, so an orthographic camera at `z = 0` cannot see a sprite at
`z = 0`; `samples/sandbox2d` puts its camera at `z = 10`. Every pass draws through the one matrix, so
meshes and sprites share a world, and a scene with several cameras draws through the last one rendered.

`Renderer::overrideCamera(view)` sets a second `ViewCamera` that the frame draws through while it is
set, and `releaseCamera` drops it. The scene keeps writing the first underneath, which is what makes a
release exact. `engine.screenToWorld` and `engine.screenSize` are bound by `Renderer` and read the
scene's camera, never the override; `screenToWorld` returns where the ray through a point meets the
`z = 0` plane, which under an orthographic camera looking down -Z is just the point under the cursor.

The world is **Y-up** through either projection: both flip Y in the projection matrix rather than with
a negative viewport height. A `Sprite` is a quad of `size` in its node's local XY plane, pushed through
its world matrix, so it moves, turns and scales in 3D like a mesh, with the texture's first row along
its +Y edge. The sprite pipeline tests depth without writing it (`GraphicsPipelineBuilder::depthRead`):
`MeshPass` has already written depth, so a mesh in front hides a sprite, while sprites still layer among
themselves in submission order. `engine.drawSprite`, `drawSpriteRegion` and `drawRect` place a
rectangle on `z = 0` with `x, y` at its bottom-left.

Conventions used throughout, follow them:

- every call goes through `vk(result, "vkFunctionName")` from `gfx/vk/VkUtil.hpp`
- structs are zero-initialized (`VkFooCreateInfo info{};`) and **every count is set by hand**
- buffers and images allocate through VMA (`ctx.allocator()`); `GpuBuffer` is always host-visible
  and persistently mapped
- every Vulkan-owning type is move-only with a destructor and no `destroy()` method. `VkCtx` is
  declared **first** in its owner and destroyed **last**; everything holding Vulkan handles holds a
  `VkCtx&`, never a copy and never an owning pointer. Declaration order *is* the destruction
  contract.

`Assets` hands out `int` texture handles (`DrawList::WHITE == 0`), caches by path, and never frees;
`MAX_TEXTURES = 256` is backed by a fixed-size descriptor pool.

**Nodes never store those handles**, because a handle means nothing on the next run.
`Sprite::texture` and `MeshPart::texture` / `mesh` are *names* — a path relative to `Content/`,
or a primitive such as `"cube"` — resolved through `DrawList::textureHandle` / `meshHandle` on first
draw and cached until `propChanged` says the name changed. `RendererDrawList` caches the handle by
name, so a thousand sprites on one texture resolve its path once. A missing file logs once and draws white;
an unknown mesh logs once and draws a cube. The clear colour and the 2D virtual size are `Camera`
props, pushed to the renderer every frame through `DrawList::background` and `camera`, so a scene
file carries them.

**The UI draws in the present pass**, after the composite, through
`gfx/UiRenderer` — never as a `DrawPass`, because every `DrawPass` records in the scene pass, which has
depth. `Renderer::setUiPaint` installs a hook that `beginFrame` calls with a fresh `ui::ElementList`,
in window points, every frame. Paint runs before the frame fence, so it fills CPU
arrays only: `UiRenderer::prepare`, recorded before the scene pass, batches the list with the GPU-free
`ui::batch`, fills that frame's growable host-visible buffers and uploads the dirty rectangles of the
list's glyph atlas into R8 pages — barriers from `SHADER_READ_ONLY` so the glyphs already there survive,
and dirty rectangles cleared at record time, because the `OUT_OF_DATE` path paints without recording.
One pipeline draws everything with a mode per vertex: solid, textured, glyph coverage with a gamma
term, a rounded-box SDF for fills and borders, and an opaque image for the scene target, so the Scene
view is never blended by its own alpha. Output is premultiplied; a non-sRGB swapchain gets a
manual encode, and a format change rebuilds the pipeline. Only elements that sample a texture break a
batch, so boxes, lines and text on one glyph page draw in one call. `pixelsPerPoint` is the swapchain
width over the window's width in points, measured whenever the swapchain is rebuilt and never
in between, so a live resize cannot rasterize glyphs at a stream of fractional sizes.

`Renderer::capture(path)` delegates target readback and PNG encoding to `gfx/Capture`.
`requestWindowCapture(path)` copies the swapchain image of the next frame instead, adding `TRANSFER_SRC`
to the swapchain the first time it is asked. Both stall the device — debug tools, not
per-frame features.

## Physics

**The physics engine is our own**, like the renderer and the coming UI framework — not Jolt, not Box2D.
It is `src/physics/`: a layer over `scene` and `reflect` with no Vulkan and no window in it, so all of
it runs headlessly in `ctest`. Its only other edge is to `lua`, because a pass binds its own script
functions, exactly as `gfx/pass` does.

`Engine` owns a `physics::World` and steps it in `Engine::update`, after `Scene::update` and before
`input.consume()`. Edit mode never calls `Engine::update`, so **physics never runs while you edit** —
the same split that keeps scripts from running, at no cost in code.

Two node classes, registered by `physics::registerNodes`:

- **`Body`** is what physics moves: `motion` (`static | kinematic | dynamic`), `mass`, `gravityScale`,
  `linearDamping`, `angularDamping`, `velocity`, `angularVelocity` and `planar`. **`planar` is all that
  makes physics 2D**: it locks z travel and x/y turning, so a 2D scene is the same engine with three
  degrees of freedom removed — the rule the renderer already follows, where `projection` is all that
  makes a scene 2D. Velocity is a prop like any
  other, so a scene file carries a starting velocity, Properties shows it live during Play, and
  `ball.velocity = vec3(0, 5, 0)` will work from Lua — with no code in the scripting layer or the
  editor.
- **`Collider`** is a shape: `shape` (`box | sphere | capsule`), `size`, `friction` and `restitution`. The
  material lives on the shape, not the body, because a static floor is a collider with no body and
  still has to be rough or bouncy; a pair combines as `sqrt(fA * fB)` and `max(eA, eB)`. Its own transform
  offsets it from the body, and `size` is the box the shape fits in — a sphere takes the largest of the
  three; a capsule stands along its own Y, as wide as the larger of x and z and as tall as y, so
  `1 2 1` is a capsule of length 2 and `1 1 1` collapses to a sphere. The default `1 1 1` box is
  exactly the unit cube a `MeshPart` draws.

**A collider belongs to its nearest `Body` ancestor**, which is Unreal's welding rule: every shape under
one body is one rigid compound. A collider with no `Body` above it is static, so a floor is a `MeshPart`
with a `Collider` inside it and nothing else — Unity's and Unreal's behaviour, where Godot would want
the body spelled out.

`World::step` **walks the scene** every step rather than keeping a registry, which is what makes
inserting, destroying, disabling and reparenting a body need no bookkeeping: a disabled subtree is
skipped exactly as `Scene::update` skips it, and `Scene::clear` on Stop empties the world by
construction. The walk is in tree order and nothing iterates an `unordered_map`, so a scene steps
identically every run; `tests/physics_test` pins that bit for bit.

`World` gathers `BodyState` values, owns scene traversal, integration, sleeping and contact events,
and delegates contact preparation, warm starting and impulse solving to `ContactSolver`. The solver
owns its contacts and carried impulses, operates on a span of the current step's body states, and
exposes read-only contacts for island sleeping. Begin it only after body gathering is complete;
never grow the state vector while the solver is using it. Constants, ordering and equations are
unchanged.

The step, in order:

1. **Place** — a body whose node matrix is not the one physics last wrote has been moved by something
   else — a script, Properties, or a parent moving — so it teleports there, keeping its velocity. A
   kinematic body always takes its pose from its node and derives its velocity from how far it moved.
2. **Geometry** — each collider's world shape, from its node matrix, through `geometryOf`. `dev/Gizmos`
   draws the wireframes from the same function, so what is drawn is what collides.
3. **Weigh** — a body's mass is split between its colliders by volume, giving a centre of mass and a
   full inertia tensor through the parallel axis theorem. **A body turns about its centre of mass**,
   not its origin, so a collider hung off to one side behaves like the weight it is.
4. **Integrate velocities** — gravity, then damping.
5. **Detect** — `physics::Broadphase` is a bounding volume tree, rebuilt from the frame's bounds every
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
   never integrated, so a settled stack is bit-for-bit still. It wakes when its node is moved, its
   velocity written, an impulse or force applied, a contact with anything that is moving, or a contact
   it had disappears — which is what catches the floor being deleted under it.
8. **Advance** — integrate the centre of mass and the orientation by velocity plus drift, then write
   the pose back
   as the node's local position and the Euler angles in the order `Transform::local()` composes them
   (Y, then X, then Z). The world keeps the quaternion, so a tumbling body simulates exactly through
   ±90° pitch even though the angles Properties shows flip there.

Three tolerances in `Collide.cpp` are load-bearing, and each was found watching a stack fall over.
Clipping keeps a corner within `CLIP_SLACK` of a side plane, and a contact within `TOUCH_SLACK` of the
reference face, because two boxes resting squarely put four corners exactly on those planes, where
float noise drops one and the lopsided support tips the box. `FACE_BIAS` makes the second box's face
beat the first's by a margin before it becomes the reference face, because on an exact tie the choice
flapped from step to step, changing every feature id and throwing the warm start away.

**Scripts reach physics through `engine` and through node signals.** `World::registerApi` delegates to
`physics/PhysicsApi`, which binds
`applyImpulse`, `applyForce`, `applyTorque`, `raycast`, `gravity` and `setGravity`; the prelude wraps
the last three so a raycast takes vectors and hands back `{ node, position, normal, distance }` or
`nil`, and puts `applyImpulse(impulse, point?)` on every node proxy. Impulses and forces are
**accumulators**, applied at the next step and cleared there, so the order a script calls them in never
matters. A node that is not a `Body` raises a Lua error rather than doing nothing quietly.

Contacts reach scripts the way scene changes do. `physics::ContactObserver` is an interface in
`physics`, `script/SceneObservers` implements it, and `LuaHost::boot` installs it — so the simulation
never calls Lua directly, just as `scene` never does. `World` tracks which collider pairs touch, compares that with the last
step's, and fires `touched` and `touchEnded` on both nodes, Roblox's names. The node handed over is the
`Body` when there is one and the `Collider` when there is not, so a floor with no body still answers.

`Config/Game.ini` carries `[Physics] gravity` as three numbers, read by `ProjectConfig` and pushed into
the world by `Engine`; a project that says nothing falls at 9.81 units a second squared.

A raycast does **not** use the tree: it walks the scene and tests every collider, because the tree
belongs to the last step and a script can cast a ray at any time, after anything has moved. One ray
costs the walk either way; many rays per frame are what would justify keeping the tree valid.

`samples/physics` is the sample: a floor, a ramp and a step as static colliders, balls, capsules, a
stack of crates, and a `Script` that drops more balls onto it. `--scene Scenes/planar.scene` opens the 2D half —
an orthographic camera over sprites with `planar` bodies, where a click drops another box. `MeshPass` carries three primitives — `cube`, `sphere` and `capsule`, the last two matching what a
sphere and a `1 2 1` capsule collider cover; `Picking` still treats every `MeshPart` as a cube.

What is missing is tracked in `TODO.md`: joints, CCD, sensors, collision layers and convex hulls. So is the one that is not physics' fault — nothing interpolates
transforms, so at `fixedHz` 60 on a 120 Hz display a falling body visibly steps.

## The UI framework

**The editor runs on a retained UI of our own, shaped like Unreal's Slate and named our own way.** Widgets are objects that persist between frames; each frame the `Application` asks the
root for its desired size bottom-up (`Widget::prepass`), arranges children top-down
(`arrangeChildren` hands out a `Geometry`), and paints into the renderer's `ui::ElementList`. The same
paint builds the `HitTester`, so input always routes against what was last drawn. Everything in
`text` and `ui` runs headlessly: `tests/ui_harness.hpp` drives a real `Application` with a
`HeadlessPlatform` clock and clipboard.

Widgets are built with `ui::make<T>()`, which returns the widget's `Args`; chained setters fill them,
`[child]` sets the content, `+ T::slot()` adds a slot, and the `Args` convert to `std::shared_ptr<T>`
— or to any base — by building the widget. `.assign(ptr)` keeps a handle, the way `SAssignNew` does:

```cpp
auto toolbar = ui::make<HorizontalBox>()
    + HorizontalBox::slot().autoWidth().padding(4)
    [
        ui::make<Button>().text("Play").onClicked([&] { session.requestPlay(); return Reply::handled(); })
    ]
    + HorizontalBox::slot().fill(1)
    [
        ui::make<Label>().assign(title_).text([&] { return history.title(); })
    ];
```

A widget declares `struct Args : ui::Args<Args, Widget>` with `UI_ATTR`, `UI_ARG`, `UI_EVENT`,
`UI_CONTENT` and `UI_SLOTS`, each taking an optional default, and implements `construct(const Args&)`.
`ui::Args` adds visibility, enabled, tooltip and cursor to every widget. An `Attribute<T>` holds a value
or a getter called when read, which is how a label follows `history.title()` without being told.
`auto x = ui::make<T>()...` is the `Args`, not the widget; name the type or call `.build()`. Slot types
live at namespace scope, because clang cannot use a nested class's member initializers inside the
class that encloses it.

**Input goes through `platform::Input`'s event queue, not its edge flags.** `setRecording(true)`
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

`platform/InputScript` replays a text file of timed input — `12 move 40 60`, `13 click left`,
`14 type Crate`, `15 tap enter`, `20 capture out.png`, `21 quit` — through `Input::inject` with the
OS's events ignored. It is how a change to a panel is verified without a human at the keyboard, and
`graphics_smoke` drives the whole editor the same way.

**The editor panels live in `dev/panels/`**, namespace `cinder::dev::panels`, over the units that
know nothing about any UI: `Toolbar` (play controls, save, undo and redo, the dirty marker, the
⌘ shortcuts as a `CommandList`, and the Save / Don't Save / Cancel `Dialog`), `SceneView` (a `Viewport`
whose client drives `EditorCamera`, `Picking`, `Manipulation` and `GizmoLines`), `Explorer`,
`Properties`, `Console` and the docked `Layout`. The host's
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
until the pointer leaves that widget. Widgets that open popups remember the `Application` they opened
on, so tests can ask `isOpen()` outside a frame. Anything that reads the hit grid during paint — a
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
rebuilding every node cannot invalidate a row: each frame it flattens the expanded items through
`treeItemsSource` and `onGetChildren`, realizes the slice in view — reusing the row a visible item
already has — and drops the rest, so a thousand items cost a dozen widgets. A `ListView` is a
`TreeView` with no children callback. Selection is the tree's own, or the panel's when
`isItemSelected` is bound, which is how the Explorer will follow `Selection` with nothing to keep in
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

**Docking is a layout tree, not widget surgery.** `ui/docking`'s `TabManager` holds a `LayoutNode`
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
`"Color.Primary"` — built by `ui::defaultTheme()` in the colours of Unreal's dark editor. Colours are
linear `ui::Color`s authored as sRGB hex; the swapchain encodes them.

## The dev tools

**The dev tools are a separate link target, not a runtime flag.** `player` links `engine` and
`editor` links `engine_dev`, so nothing editor-only is in the shipping binary. There is no `--dev`:
to get the tools, run `editor`. `tests` links `engine_dev` as well, so `History`, `Picking`,
`EditorCamera`, `Manipulator` and the panels themselves are tested headlessly.

The seam is `Renderer::setUiPaint(std::function<void(ui::ElementList&)>)`: `gfx` knows how to draw an
element list and nothing about panels, `Application` or `scene`. `editor/main.cpp` installs a hook
that drains `Input`'s event queue into `Application::processEvents`, updates each panel, paints the
widget tree into the list, and lets the Scene view apply its camera override and input suppression.
`player` never installs one, and what remains in the shipping binary is an empty `std::function`, a
zero viewport size, an empty camera override and a couple of branches per frame. That is the whole
cost of the seam.

`Input` installs its GLFW callbacks in its constructor, and `Engine` declares `input_` **before**
`renderer_`; keep that order, because the window has to exist and its callbacks have to be installed
before anything else listens.

The editor's layout is not persisted between runs yet: `TabManager` holds it in memory, and writing
it to the project's `Saved/` is the "editor layout persisted between runs" item in `TODO.md`.

### The Scene view

`dev/panels/SceneView` is the "Scene" tab: a `ui::Viewport` widget whose client it drives. Each frame
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

In Edit mode a **left click** on the image picks. `dev/Picking` is a CPU raycast that knows nothing
about the UI: the click unprojects through the editor camera and hits every enabled `MeshPart` as the
unit cube `Mesh::cube` is — which is every mesh there is — every enabled `Sprite` as the quad it draws,
in its own plane and axes, and every enabled `Camera` within 10 points of where it projects, taking the
nearest. A tie goes to the later node in render order, so of two sprites in one plane the one drawn on
top wins. A disabled node hides its subtree, as it does from `Scene::render`. A click is a press and
release that stays inside `Application::DRAG_THRESHOLD`; past it, a left drag still looks around. A hit
selects with `reveal`, so the Explorer expands the node's ancestors and scrolls to its row; a miss
clears the selection. A press on a gizmo handle comes first: it drags the handle, and its release never
picks — see *Transform gizmos*.

`dev/panels/Layout` builds the default dock layout — the Explorer left, the Scene view in the middle
with the console under it, Properties right — as a `TabManager` layout of splits and stacks, and hands
the toolbar the Window menu that reopens a closed tab.

### The editor camera

In Edit mode the Scene view looks through `dev/EditorCamera`, not through the scene's `Camera`.
`SceneView::afterPaint` hands its view to `Renderer::overrideCamera` on every Edit frame and calls
`releaseCamera` in every other state, so Play and Pause show the game's camera. The override covers the whole frame,
sprites included, so a 2D scene is flown around exactly like a 3D one — Unity's Scene view with 2D
mode off.

The editor camera is always perspective. It is seeded once, on the first Edit frame, from the last
enabled `Camera` in render order — the one whose `DrawList::camera` call wins — taking its pose, fov
and clip planes. From a perspective camera that is the game's frame exactly. From an orthographic one
it backs away along the camera's forward axis until its fov frames the orthographic rectangle at the
near plane, pushes its far plane back by the same distance, and flies at half that distance per
second, so a 640-unit view does not crawl at 5 units a second; anything beyond the near plane looks
slightly smaller than it will in Play. Stop does not reset it: like Unity's Scene view, it stays where
you left it. It has no roll and is not saved. `tests/editor_camera_test` pins both seeds.

Its controls are not read from `Input` — they are an interaction with a panel, and `Input` belongs to
the game — and they only act on the Scene image: the mouse comes from the widget's own events and the
keys from `Application::keyHeld`, which the UI fills from the event queue. **WASD** fly, **Q/E** go down
and up and **Shift** goes faster whenever the Scene view has focus, with no button held. Hold the
**right button** to look around, and turn the wheel while looking to change fly speed. A left drag that
does not start on a handle also looks around; drag the **middle button**, or left with **Alt**, to pan;
scroll with no button held to dolly. A press captures the mouse, so the drag keeps working past the
panel's edge, and focuses the Scene view, so fly keys never land in the console.

The Scene view draws every enabled `Camera` while editing. A perspective camera is a wireframe frustum:
the near and far rectangles, the four edges joining them, and dimmer lines from the camera to the near
corners. An orthographic camera is its near rectangle alone — every cross-section of its box is that
rectangle, so it is exactly what the camera sees. The corners come from `ViewCamera::corners()` at the
panel's size — the size Play renders at — and a test pins them to the clip volume of both
projections. `dev/GizmoLines` turns them into strokes in panel points, which the panel paints into the
element list as anti-aliased lines and convex fills. They are projected through the editor camera and clipped in clip space against the near plane and the four sides before the divide, so a
corner behind the editor camera cannot fold across the image. The sides are inset by `INSET`, so a camera the editor is looking straight through — as
it is right after seeding — draws nothing, rather than a frame along the border that float noise
leaves half-drawn. Being UI, the gizmos draw over geometry, cost the player nothing, and never appear
in a `--capture`.

`selectionStrokes` outlines the selection the same way, in orange: the twelve edges of a
`MeshPart`'s cube, a `Camera`'s gizmo, or a `Sprite`'s quad, all through the editor camera.

### Transform gizmos

In Edit mode a spatial selection gets a gizmo, drawn over it from `manipulatorStrokes`. The Scene
view's tool row picks the tool — **Move**, **Rotate** or **Scale**, or **1**, **2** and **3** while the
Scene view has focus — and, for Move, **World** or **Local** axes, toggled with **X**. The tools are not
Unity's W/E/R because WASD and Q/E fly the editor camera with no button held; the panel takes those
keys from its own `onKeyDown`.

`dev/GizmoGeometry` owns handle geometry and hit testing; `dev/Manipulator` owns drag state. Neither
knows about the UI:

- `gizmoFor` places the gizmo at the node's world position, `GIZMO_POINTS` long on screen at that depth,
  so it keeps its size at any distance. There is none behind the editor camera, or under a parent whose
  matrix cannot be inverted.
- `shapes` is **the one handle geometry** — world-space points per handle — that `drawManipulator`
  draws and `hitHandle` measures in screen points, so what is drawn is exactly what can be grabbed. An
  axis within about 15° of the view ray and a plane handle within about 15° of edge-on are left out,
  which is what hides the Z arrow in a 2D scene. Ties go to the earlier shape: the centre square, then
  the planes, then the axes.
- `Manipulation` is one drag. It holds the node's **id**, as `Selection` does, and the transform as it
  was at the press, and computes every frame from that start rather than from the last frame, so
  snapping is exact and cancelling is a write of the start values.

**Move** has an arrow per axis, a square per plane, and a camera-facing square in the centre that drags
in the plane facing the view. An axis drag takes the point on the axis nearest the mouse ray, and a plane
drag intersects the ray with the plane. The travel is a world-space vector, taken into the parent's
space through the inverse of the parent's world matrix — applied as a direction, so a zero travel adds
exactly zero. Under a `Folder` the parent matrix is the identity, because a `Folder` breaks the chain.

**Rotate's rings are the Euler axes, not world or local ones** — Blender's Gimbal orientation.
`Transform::local()` is T·Ry·Rx·Rz·S, so the Y ring turns about the parent's Y, the X ring about X after
yaw, and the Z ring about the node's own Z, and a ring changes exactly one component of `rotation`.
Nothing is decomposed back into angles, which is where a gizmo picks up flips near ±90° pitch and float
noise in the angles it did not touch; the price is that the rings are not perpendicular once a node is
pitched. A ring facing the camera follows the mouse round its plane, and one nearly edge-on turns with
the mouse along its screen tangent. Only a ring's front half can be grabbed, unless the ring mostly
faces the camera — perspective would otherwise put half of a face-on ring beside the centre of the view
"behind" the pivot. A mirrored parent negates the angle.

**Scale** is always local: a world-axis scale on a rotated node needs shear, which `Transform` cannot
hold. An axis handle multiplies its component by the mouse's travel along the axis over the gizmo's
length; the centre square multiplies all three by the drag right and up.

Holding **Ctrl — ⌘ on macOS** — snaps the change, not the value: `MOVE_SNAP` units along each dragged
axis, `ROTATE_SNAP_DEGREES`, and factors in steps of `SCALE_SNAP`. **Esc** cancels a drag.

The panel hit-tests on hover and on the press, through the editor camera as it was last drawn. A press
on a handle starts the drag, stops the left button looking around and keeps the release from picking,
until the capture is released. Every write goes through `PropDef`, as
Properties' writes do, and calls `History::touch`, so a drag is one undo step labelled like "Move Box",
and a cancelled drag saves to the same text and records nothing. While the mouse is still where it was
pressed, a drag writes the start values themselves: arm64 fuses multiply-adds, so `cross(v, v)` is not
exactly zero, and a recomputed angle would move a click by an ulp and record a step. Leaving Edit mode
ends a drag.

### The Explorer and Properties

`dev/Selection` holds the selected node's **id**, never a `Node*`. Play and Stop rebuild every node
through `loadScene`, and ids are what `SceneCodec` round-trips, so a selection survives both.
`resolve` returns null — and forgets the id — once the node is gone or marked destroyed, so a node
spawned during Play drops out of the selection on Stop.

`dev/panels/Explorer` is a `TreeView` over node **ids**, so it builds only the rows in view and
nothing it holds can dangle: every row reads its node back out of the scene each frame. A row is the
node's name followed, dimmed, by its class — or its file name, for a `Script` — and a node disabled
itself or through a parent is dimmed unless it is selected. Selection is bound straight to
`dev/Selection` through `isItemSelected`, so picking in the Scene view and clicking a row are the same
state with nothing to keep in sync. A click selects and a click on empty space clears. **"+"** lists
every registered class and inserts one under the selection, or at the root. Right-clicking a row
offers Insert, Duplicate (`Scene::clone`) and Delete; dragging a row onto another reparents it, cycles
refused, and dropping it below the rows makes it a root. Delete or Backspace removes the selection
while the tree has focus — a focused text field keeps the key for itself. A filter turns the tree into
a flat list of the nodes whose name or class matches, with no expanders.

Delete uses `destroyNow`, because Edit mode never runs the `Scene::update` that
flushes a deferred destroy. An inserted or duplicated node becomes the selection. During Play the same
edits act on the running game and are discarded by Stop.

`dev/panels/Properties` builds its rows from the prop list, with no per-node-class widget code: the
node's name, class and id, a Transform section for spatial nodes, the class's props and the
attributes, each section an `ExpandableArea` of name | value rows. The widget comes from the
`PropType` and the `PropHint`: a `SpinBox` for numbers, a `VectorInputBox` for vectors, a `CheckBox`,
a `TextBox`, a `ComboBox` filled from `PropDef::options()` for enums, and a `ColorBlock` that opens a
picker for `CINDER_PROP_COLOR`. Every write goes through `PropDef`, so clamping, in-place vector
writes and `propChanged` behave exactly as they do from Lua and from the serializer. `step` is the
drag speed; bounds are applied only when the prop declares them, and a spin box never rounds a value
to the three decimals it shows. `CINDER_PROP_ANGLE` — `Transform.rotation` — is dragged in degrees and
written back in radians, so Lua and scene files never see degrees. Text commits on Enter or focus
loss, not per keystroke, so a texture path does not try to load every prefix of itself.

The panel holds the selected node's **id** and nothing else, and rebuilds its rows only when the
selection or the attribute names change, so a drag is not fighting a fresh widget tree every frame.

The **Attributes** section lists each attribute with a remove button, and **Add Attribute…** opens a
popup for a name and a type. Every attribute number is edited as a float, integers included:
`TextLoad` reads `40` back as an integer and `40.5` as a float, so an integer drag could never move a
saved `40` to `40.5`. Writes go through `Node::setAttribute`, so an edit during Play fires the game's
changed signals.

Every edit either panel or a gizmo makes is undoable and marks the scene dirty — see *Undo, redo and unsaved
changes*.

### Undo, redo and unsaved changes

`dev/History` is the editor's undo stack, and it stores **whole scenes**, not commands: after every
finished edit it saves the scene to text with `SceneCodec::save` — the same text Play snapshots — and
undo loads the previous text back with `SceneCodec::load`. Nothing has to describe an edit to make it
undoable: a prop, an attribute, an insert, a delete, a reparent and a console line are all the same
kind of step, which is what makes a new `CINDER_PROP` undoable with zero editor code. It costs one
scene save per finished edit and one load per undo.

Panels and gizmos never push steps. They call `touch(label, selection)` when they write, and
`dev/panels/Toolbar` calls `settle(app.isInteracting())` in its `update`, which commits only once
nothing holds the mouse capture, no text is being edited and no drag-and-drop is in flight. A drag that
writes on forty frames is therefore one step, carrying the label and selection of its first frame, and
an edit that saves to the same text as before records nothing. `MAX_STEPS` is 100.

Undo calls the codec directly, not `Engine::loadScene`, so it does not reboot Lua and the console's
globals survive. That is only safe because history is enabled in Edit mode alone, where no script has
started: during Play, `touch`, `settle` and the buttons do nothing, and Stop discards Play's edits anyway.
Ids round-trip through the codec, so the selection stored with the step, and the Explorer's open rows,
which the tree keys by id, come back with it.

The document is the text of the last commit, and it is **dirty** while it differs from the text last
saved or opened — so undoing back to the saved state is clean again. Save commits a pending edit and
writes that text, and the toolbar shows the scene as `main.scene*` while it is dirty. Closing the window
does not end the loop directly: `editor/main.cpp` clears GLFW's close flag and calls
`Toolbar::requestClose`, which confirms at once when the scene is clean and otherwise opens a Save /
Don't Save / Cancel modal, and the loop runs until `closeConfirmed()`. ⌘Z and ⌘⇧Z route globally like
the other shortcuts, but an active text field claims ⌘Z for its own undo first.

### Logging and the console

Everything prints through `platform/Log.hpp` — `logInfo` / `logError`, printf-style and
`__attribute__((format))`-checked. Both always write to stdout/stderr, and additionally to a
`LogSink` if one is installed. `dev/panels/Console` installs that sink in its constructor and clears it in its destructor, which is how `[lua]`/`[vk]`/`[serial]` output reaches the panel. The sink receives
the line **without its trailing newline**. `Log` itself stays in `engine`: the sink is the seam a
shipping build will use for a crash log file, so it is not a dev-only facility.

The console's input line runs `LuaHost::eval` against the live `lua_State`. It tries `return <text>`
first and falls back to the raw text, so `1 + 1` prints `2` and a multi-statement chunk still runs.
Results and errors both go back through `logInfo`/`logError`, so they land in the panel like anything
else, with `[console]` as the chunk name. In Edit mode that state holds the prelude and nothing else,
so the console can build a scene — `scene:create`, `node:setAttribute`, `node:add("Script")` — without starting
any of it, and Save writes the result.

Typing in the console does not also drive the game, because the console has focus and the Scene view
does not — see *The Scene view*.

## Scripting

A project's settings are data, not code: `ProjectConfig::load` reads `Config/Game.ini` — `[Game]`
`title`, `startScene`, `fixedHz` and `[Window]` `width`, `height` — without touching Lua, so opening a
project runs nothing. `startScene` is the scene both executables open; `--scene` overrides it.

There is **no entry script**. Game code lives in `Script` nodes, in Roblox's shape: a `Script` names a
file in `Source/`, and that file runs top to bottom, once, when the node starts, with `script.parent` the
node it sits in. Per-frame work is a `stepped:connect(fn)` handler, and the top level may `task.wait`,
because it runs as a thread. Per-node settings are attributes, not script fields. Lua errors are
caught and printed, not propagated.

```lua
local box = script.parent

stepped:connect(function()
    local h = 1 + math.sin(engine.time() * box:getAttribute("rate") + box:getAttribute("phase"))
    box.scale = vec3(1, h, 1)
end)
```

### The prelude

`LuaHost` loads three files from `engine/lua/` after `registerApi()` — order matters, since
`scene.lua` and `task.lua` both need the `engine` table to exist:

- **`types.lua`** — `vec2`/`vec3`/`vec4`/`rgba` built by one `vectype(keys)` factory, with
  `+ - * / unary-minus == tostring`, plus `:length()`, `:dot()`, `:normalized()`, `:unpack()` and
  `vec3:cross()`. `rgba` is a `vec4` whose `r/g/b/a` alias `x/y/z/w`. `vecSize(v)` returns the
  component count or nil, and is how the proxy layer tells a vector from a scalar.
- **`scene.lua`** — the `scene` global, node proxies, the attribute API and the node signals.
- **`task.lua`** — `task.wait`/`spawn`/`delay`, the `signal()` constructor, the `stepped` and
  `rendered` signals, and the owners that let a stopped script take its threads and connections with
  it.

### Node proxies

**The proxy layer is pure Lua over the `engine.*` bindings**, which all take node ids. There is one
proxy type for every node, cached in a weak-valued table so `scene:find(n) == node` holds. Reading a
key looks, in order, at:

1. fixed getters — `name`, `className`, `parent`, `position`, `rotation`, `scale`, `worldPosition`,
   `forward`, `right`, `up`, and the signals `childAdded`, `childRemoved`, `destroying` and
   `attributeChanged`;
2. methods — `getChildren`, `findFirstChild`, `add(className)`, `clone`, `destroy`, `valid`,
   `translate` and the attribute methods;
3. the node's own props, through `engine.getProp`, which returns 1-4 values by arity — so a vector
   comes back as `vec2`/`vec3`/`vec4`, and adding a prop needs no scripting-layer change;
4. a child with that name.

A prop therefore wins over a child with the same name. Writing a key that is neither a setter nor a
prop **errors**, and so does writing a transform on a node that is not spatial, whose transform getters
return nil. `scene:create(className, parent)` and `node:add(className)` make nodes, and
`box:add("Script").file = "riser.lua"` is how a script attaches another.

These contracts are load-bearing and must not drift:

- `getProp` returns **1-4 values by arity**, and **zero** values when the node or prop is missing;
  `setProp` returns whether the prop exists.
- `find` / `parent` / `findFirstChild` return **`nil`**, never `0` or `-1`.
- Node ids and the handles `loadTexture`/`newCube` return push as **integers**; numeric prop values
  push as **floats**, string and enum props as strings.
- Edge-triggered input clears in `consume()` **per fixed step**, not per frame.
- `screenToWorld` takes **window points**, matching `mousePosition`, and returns the point on the
  **`z = 0` plane** under them, through the scene's camera.
- `getAttribute` returns **`nil`** for a missing attribute, and a 2–4-number attribute as a vector.

`script/LuaProps` converts between Lua values and `PropValue` — scalars, arrays of scalars,
string-keyed records, and vectors (detected by their metatable's `__vec`) as sequences of numbers.
Functions and anything else with a metatable are skipped.

`tests/selftest` is a project whose scene runs a 60-check smoke test for this whole layer from a
script's top level. `./build/editor tests/selftest --play --frames 120` or `./build/player
tests/selftest` prints `ALL PASS`. It needs a window, so it is not part of `ctest`.

**Where to add a Lua function:** `LuaHost::registerApi()` builds the global `engine` table and delegates
the engine-wide calls (time, quit, log, input, textures) to `script/RuntimeApi`, then hands the `LuaApi`
to `registerSceneApi`, `renderer.registerApi()` and `physics.registerApi()`. The renderer forwards it
to every pass. `RuntimeContext` is a member of each host; closures retain that host's context address
until its Lua state closes, with no process-global binding context. Bind a function
in the class that owns the state it touches — draw calls belong in `SpritePass`/`MeshPass` and camera
calls in `Renderer`, not in `LuaHost`.

`lua_CFunction` cannot capture, so each binding carries its receiver as a light-userdata upvalue:
`api.bind("name", fn, &receiver)`, read back with `LuaApi::context<T>(state)`. Use
`LuaApi::optFloat`/`optInt` for optional numeric arguments.

`lua/LuaCalls` supplies `StackRestore`, `pushFunction` and `protectedCall` for host-to-Lua calls.
`protectedCall` leaves successful results on the stack and logs and removes an error on failure;
`StackRestore` restores the caller's original top on scope exit. `runChunk` deliberately retains its
older contract: failures leave the error on the stack for the caller to report. Use stack restoration
only on protected host-call paths, never to rely on C++ destructors across a `lua_error` long jump.
The scene/contact observer adapters use these same helpers.

### Scripts

`Script::BOOTSTRAP` caches each file's source by path and compiles it per instance with
`load(source, "@" .. file, "t", env)`, where `env` is `setmetatable({ script = __node(id) }, { __index
= _G })`: `script` is the Script node's own proxy, so `script.parent` is its object and `script.file`
its path. A global a script assigns stays in its own environment, and `_G.x` is how two scripts share
one. `__scriptStart(file, node, previous)` compiles first, so a start that fails to compile leaves
`previous` running, and only then stops `previous` and spawns the chunk as a thread of a fresh owner.

`Script` is a `PropSink`: writing `enabled` false stops it, and writing it true runs the file again
from the top. `Scene::startPending` calls `onStart` on a disabled node too, so `Script::onStart` checks
`isEnabled` itself.

### Attributes

Per-node settings are **attributes**: a `PropRec` on `Node`, saved as an `attributes { ... }` block,
edited in Properties, and read by scripts. `scene/Attributes` decides what one may hold — a number,
string, bool, or a sequence of 2–4 numbers — and what a name may be: letters, digits and `_`, which the
text format needs anyway. The Lua API is Roblox's in camelCase: `node:getAttribute(name)`,
`setAttribute(name, value)` (nil removes), `getAttributes()`, `getAttributeChangedSignal(name)`, and
`node.attributeChanged`, which fires with the name. An invalid name or value raises a Lua error.
Because attributes are data on the node, nothing has to run for Properties to list them, and they
survive a hot reload.

`LuaHost::boot` installs the `SceneObserver` from `script/SceneObservers`, which calls `__attributeChanged`,
`__childAdded`, `__childRemoved` and `__destroying`; that is how an edit in Properties during Play
reaches the game's signals. `__destroying` also drops the node's cached signals, and `LuaHost::close`
removes the observer before the state closes.

Lua is built as C, so `luaL_error` longjmps past C++ destructors. A binding finishes its C++ work
before it raises: `setAttribute` validates in a helper that returns a status, and errors only after
that helper's `std::optional` is gone. No C++ exception may cross Lua either, which is why `setParent`
checks for a cycle before it calls `Node::setParent`, and why `loadTexture` catches a failed load and
calls `lua_error` only once the `catch` has ended.

### Ownership

`task.lua` keeps a module-local `current` owner. A thread records the owner that was current when it
was spawned, a connection records it when it connects, and resuming a thread or firing a handler puts
that owner back in `current` — so everything a script starts, and everything that starts, belongs to
the script. `__taskStop(owner)` disconnects the owner's connections and closes its suspended threads.
A thread that is running when its owner stops is only marked; the scheduler drops it when it next
yields. `__taskStop` never removes from the thread list, which is what makes it safe to call from
inside a resume — a script disabling itself, say. The console and the prelude run with no owner, and
what they start lives until Stop discards the state.

### Hot reload

Only scripts hot-reload, and editing one reloads only that file. `LuaHost::reloadScript` calls
`__scriptForget(path)` to drop the cached source, then `__scriptCheck(path)` to compile it once. A
file that fails to compile logs one error and changes nothing: every script running the old code
keeps running. Otherwise every `Script` on that file that is started and enabled runs again — its old
owner stops, taking the old code's threads and connections with it, and the file runs from the top.

That is Roblox's trade: what a script keeps in locals starts over, and what it keeps in attributes
survives, because attributes live on the node. The scene, the `lua_State` and every other script are
untouched.

`__scriptRead` is the single funnel every script file is read through, so it is also where `LuaHost`
records the path to watch. Nothing walks a directory; a script is watched because it was read.
`poll()` stats each read script every frame. In Edit mode nothing has been read, so nothing is
watched — and Play reads every script from disk anyway, so edits made while editing are picked up by
the next Play. A script that failed to compile when it first started is still watched, so fixing the
file starts it.

The prelude is not watched; editing `types.lua`, `scene.lua` or `task.lua` needs a restart.

## Reflection

There is no runtime reflection in C++, so props are declared with a macro that stringises the field
name — the identifier *is* the serialization and Lua key:

```cpp
CINDER_NODE(Camera, cinder::scene::Spatial) {
    CINDER_PROP(projection_);
    CINDER_PROP_S(fov_, 1.0f, 179.0f, 1.0f);
    CINDER_PROP_R(zoom_, 0.05f, 20.0f);
}
```

`CINDER_PROP_R` adds a clamp range, `CINDER_PROP_S` a range and a drag step, and `CINDER_PROP_COLOR`
marks a `vec3`/`vec4` as a colour through `PropHint`, and `CINDER_PROP_ANGLE` marks radians to show in
degrees. Only Properties reads the step and the hint; neither changes how a prop is written or saved, and
neither a colour nor an angle is clamped.

`props<T>()` builds the list once into a function-local static and a `PropChain` recursion emits the
base class's props first, so `Node::enabled` is always the first prop of every class —
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

`NodeTypes` is an instance owned by `Engine`, threaded to `Scene` -> `SceneCodec` / `SceneApi`. It
keeps insertion-ordered iteration (a vector plus two indices) and caches a class-default instance per
type for delta encoding. Re-binding a name keeps its slot — so iteration order is stable across a
reboot — and drops its cached default, so a `Script` default cannot outlive the `lua_State` it
closed over.

`SceneCodec::VERSION` and `OLDEST` are both 4 — see *The scene file*. `tests/scene_files_test` loads
and re-saves every `.scene` and every project's `Config/Game.ini` under `samples/` and
`tests/selftest/`, and requires the bytes to match, so shipped scenes and settings stay canonical as
the formats move.

## Projects and packaging

A project is a folder in Unreal's shape:

```
MyGame/
  MyGame.cinder      the project descriptor, Unreal's .uproject — marks the folder as a project
  Config/Game.ini    settings
  Content/           scenes, textures, meshes — Scenes/ and Textures/ by convention, not by rule
  Source/            scripts
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

`core/ProjectDescriptor` carries every `.uproject` field that means something outside Epic's own
tooling — `EpicSampleNameHash` and `IsEnterpriseProject` are left out — and not all of them are read
yet:

- **`fileVersion`** must be `ProjectDescriptor::FILE_VERSION`, 1.
- **`engineAssociation`** is the cinder version the project was made with. One that differs from
  `ProjectDescriptor::engineVersion()` — CMake's `PROJECT_VERSION`, baked in as `CINDER_VERSION` —
  logs a warning. Empty means the project lives in the engine's tree, which is Unreal's convention
  and what the samples do.
- **`targetPlatforms`** (`macOS`, `Linux`, `Windows`; empty means all) and **`preBuildSteps`** /
  **`postBuildSteps`**, a platform name to command lines, are read by `package_game`, below.
- **`category`** and **`description`** are for the project picker.
- **`modules`**, **`plugins`**, **`additionalRootDirectories`**, **`additionalPluginDirectories`** and
  **`disableEnginePluginsByDefault`** are parsed, type-checked and saved, and nothing reads them: they
  wait for a plugin system and for `require`.

`ProjectDescriptor::save` writes Unreal's layout — tabs, the first four keys always, everything else
only when it is set — and `tests/scene_files_test` holds the shipped descriptors to it. A wrong type,
or a module or plugin without a `name`, is an error that names the key, and `ProjectConfig::load`
prefixes the file name. `serial/Json` is the format: `parseJson` builds a `PropValue` — objects are
`PropRec`, arrays `PropSeq`, and `null` is rejected — and reports errors as `line:column`, and
`JsonWriter` streams, so the descriptor rather than a sorted map decides the key order.

`serial/IniLoad` and `serial/IniSave` are the `Archive` for INI: a record is a `[Section]`, a field is
`key=value` with camelCase keys like props, a value runs raw to the end of its line, and numbers go
through `from_chars`/`to_chars`. INI has no arrays, attribute bags or nested sections, and asking for
one throws `std::logic_error`.

`platform/Assets` is the only thing that turns a name into a path, and it has three roots:

- **`enginePath`** — shaders and the prelude. `CINDER_ENGINE` if set; else `engine/` next to the
  executable, if it exists, which is the packaged layout; else the source tree's `engine/`, baked in
  at configure time as `CINDER_ENGINE_DEFAULT`.
- **`contentPath`** — `Content/`: the scene to open, `Sprite.texture`, `MeshPart.texture` and
  `engine.loadTexture`.
- **`sourcePath`** — `Source/`: a `Script`'s `file`, through `__scriptRead`.

The loader picks the root, so a path never names it — a scene says `file "bobber.lua"` and `texture
"Textures/box.png"`. Both roots sit under the project root, which is made absolute, so a project
folder can live anywhere and runs from any working directory; the player falls back to `project/`
next to itself. A path that is absolute or climbs out of its root with `..` throws: a texture logs
and draws white, a script or `loadTexture` raises a Lua error. It would work in the editor and break
once packaged. A path that exists but differs in case from the disk logs `[assets] ... differs in
case`: macOS is case-insensitive by default, and the same project would not find the file on a
case-sensitive system.

`cmake --build build --target package_game` builds `player` and runs `cmake/PackageGame.cmake`, which
stages `build/dist/<name>/` as `player`, `engine/shaders/*.spv`, `engine/lua/*.lua`, `engine/fonts/` and `project/`
holding the `.cinder` file, `Config/`, `Content/` and `Source/` — never `Saved/` or anything else in
the folder. `<name>` is the `.cinder` file's stem. The cache variable `CINDER_PACKAGE_PROJECT` picks
the project, defaulting to `samples/sandbox2d`.

It reads the descriptor with CMake's own `string(JSON)`. Before staging it refuses a project whose
`targetPlatforms` does not list the host and runs the host's `preBuildSteps`; after staging it runs
`postBuildSteps`. Each step runs through `sh -c` — `cmd /c` on Windows — in the project folder, with
`$(ProjectDir)`, `$(EngineDir)` and `$(StageDir)` expanded, and a step that fails stops the package.

## Conventions

- **Zero comments.** No comments or doc blocks anywhere, by choice. Match it; explain in chat or in
  these docs.
- Members carry a trailing underscore. `CINDER_PROP(position_)` strips it, so the wire key stays
  `position`. This is also why `Camera`'s clip planes are `near_`/`far_` — `near` and `far` are
  macros in `windef.h`.
- `Glfw` is refcounted `acquire`/`release`. `Window` acquires in its constructor and releases in its
  destructor, and `main` holds an outer acquire across the whole run.
- `Input` is edge-triggered: `keyPressed`/`keyReleased` are true for exactly one fixed update,
  cleared by `input.consume()` at the end of `Engine::update` and on every `GameLoop::idle` frame.
- **`std::to_chars` everywhere in `serial`** — never `printf` or `ostream`, which follow the locale.
  A test pins the output under a Turkish locale, and `IniLoad` and `parseJson` parse with
  `std::from_chars` for the same reason.
- Sizes and view dimensions come in two flavours and they are not interchangeable: the swapchain and
  render target are in **framebuffer pixels**, while `ViewCamera` and `screenToWorld` are in
  **window points**. On a Retina display these differ by 2x. GLFW reports cursor positions in points,
  which is why the cameras use them. In the editor the view is the Scene panel rather than the
  window, and `setViewportSize` takes points and converts to pixels itself.
