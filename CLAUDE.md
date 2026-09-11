# CLAUDE.md

Guidance for Claude Code (claude.ai/code) when working in this repository.

## What this is

A from-scratch C++20 game engine: Vulkan renderer, GLFW windowing, Lua 5.4 scripting. The long-term
target is a Unity/Unreal-shaped editor workflow — select an actor, edit its fields, hit Play, hit
Stop, land back where you started. `TODO.md` is the authoritative roadmap; read it before proposing
architectural work, since it records what is deliberately deferred and what is out of scope.

The repo holds the engine only. A game is a **project folder** — a `project.lua`, its scenes and its
scripts — that the editor opens and the player plays. `samples/` holds two example projects; nothing
in them is compiled.

## Commands

```bash
cmake -S . -B build -G Ninja && cmake --build build
```

A plain build produces `engine`, `engine_dev`, `editor` and `tests` — **no game**. The runtime that
plays a project, `player`, is `EXCLUDE_FROM_ALL` and only built on demand.

```bash
ctest --test-dir build --output-on-failure
```

`ctest` builds `player` itself through a fixture, because `shipping_binary_is_clean` inspects it.

```bash
./build/editor samples/sandbox2d
```

`editor` is the **dev build** — the same engine plus the ImGui overlay: a dockspace holding the Scene
viewport and the console, with the play toolbar in the main menu bar. It opens a project in **Edit
mode**: the scene is loaded and drawn, and no game code runs.
Play/Pause/Step/Stop are on the toolbar and on ⌘P, ⌘⇧P and ⌘⌥P; ⌘S saves the scene. It takes
`--scene <path>` (project-relative), `--play` (start in Play), `--frames <n>` and `--capture <png>`.
The last two make it scriptable: `--frames 90 --capture out.png` runs headless-ish and writes a
screenshot. See *The dev overlay* below for why this is a second executable rather than a flag.

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

Note that `--capture` reads back the **scene render target**, not the swapchain, so an overlay would
never appear in a capture anyway. That is deliberate: it is the game's picture, not the editor's. In
the editor that target is the size of the Scene panel, not the window.

The first configure fetches every dependency and needs network — glfw, glm, lua, VMA, stb, volk,
Vulkan-Headers, Dear ImGui and doctest, all pinned in `cmake/Dependencies.cmake`. Nothing needs
installing.
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
  engine/         engine data: shaders/ (GLSL and the compiled .spv) and lua/ (the prelude)
  samples/        sandbox2d/ and sandbox3d/ — example projects
  src/
    reflect/ lua/ platform/          leaves
    scene/ serial/ components/       the world model
    gfx/ gfx/vk/ gfx/asset/ gfx/pass/  the renderer
    script/ core/                    the Lua host and the engine
    dev/                             ImGui, panels, dockspace, play session; NOT part of `engine`
    player/ editor/                  the two executables
  tests/
    selftest/                        a project whose scene runs the Lua smoke test
```

`src/` is the only include root, so every include carries its layer: `#include "scene/Actor.hpp"`.
Headers sit next to their sources.

## Layering

The layer graph is **acyclic and strictly layered**. Keep it that way — this is what lets the editor
depend on `scene` + `reflect` without dragging in Vulkan or Lua.

```
reflect, platform          -> leaves (no engine includes)
lua                        -> platform
scene                      -> reflect
serial                     -> scene, reflect, platform
components                 -> scene, reflect
gfx/vk                     -> platform
gfx/asset                  -> gfx/vk, scene
gfx/pass                   -> gfx/asset, gfx/vk, scene, lua, platform
gfx                        -> gfx/pass, gfx/asset, gfx/vk, scene, platform, lua
script                     -> scene, reflect, gfx, platform, lua
core                       -> all of the above
dev                        -> core and all of the above (a separate target, see below)
```

Everything down to `core` is the `engine` library. **`dev` is not** — it is its own target,
`engine_dev`, and it holds everything editor-only: ImGui, the console, the Scene viewport, the
dockspace, the play session and the toolbar. It is the only place ImGui may be mentioned. `player` links `engine`; `editor` links
`engine_dev`. If a `#include <imgui.h>` ever appears outside `src/dev/`, the split is broken.

**`platform` is the one leaf everything may reach for**, because `platform/Log.hpp` lives there and
every layer logs, and `platform/Assets.hpp` is the only thing that turns a name into a path. That is
the only reason `serial`, `lua` and `gfx` have an edge to it, and neither header includes anything
from the engine, so the edges cost nothing and create no cycle.

`gfx` is split four ways and the seam that matters is `gfx/vk`: it knows Vulkan and knows nothing
about this engine, which is what let `dev/ImGuiLayer` build the ImGui Vulkan backend on it without
dragging in passes or assets. `gfx/asset` is what you draw with (`Assets`, `Texture`, `Mesh`),
`gfx/pass` is how you draw it (`DrawPass` and its implementations, their pipelines and cameras), and
`gfx` itself is only the orchestrator — `Renderer`, `RenderTarget`, `CompositePipeline`,
`RendererDrawList`. Nothing in a subdirectory includes its parent.

Five placements are load-bearing and were each chosen to kill a cycle: `Glfw`/`Window`/`Input` live
in `platform`, not next to `Engine`; `DrawList` lives in `scene`, so the scene graph never includes
`gfx`; `LuaApi` lives in `lua`, so a pass can bind its own functions without including the script
host; `PropBag`/`PropValue` live in `scene`, not `serial`, so `Behaviour` can expose its Lua-side
fields without `script` gaining an edge to the serializer; and `Overlay` is an abstract interface in
`gfx` with its only implementation in `dev`, so the renderer can host an ImGui layer it cannot name.

`LuaHost` takes `(Scene&, Input&, Renderer&, quit)` — never `Engine&` — for the same reason.

## Frame flow

`main` -> `Glfw::acquire()` -> `Engine(config)` -> `engine.openScene(path)` -> a loop.

`Engine`'s constructor boots Lua but **loads nothing and runs nothing**; the host decides what
happens next. The player calls `GameLoop::tick` every iteration. The editor calls
`PlaySession::tick`, which calls one of three `GameLoop` entry points:

- **`tick`** — the simulation:
  1. `Glfw::pollEvents()`, then `engine.beginFrame()` -> `script->poll()` (behaviour hot reload)
  2. minimized -> `Glfw::waitEvents()`, reset the clock, skip the frame
  3. `GameLoop::advance()` accumulates real time, clamped at `MAX_FRAME_TIME = 0.25s`
  4. `engine.update(fixedDt)` N times — fixed timestep; `script->update`, `scene.update`, then
     `input.consume()`
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

## Edit mode and Play mode

A game is a `.scene` file whose actors carry components — the Lua ones are `Behaviour`s. There is no
entry script and no top-level game code.

Nothing runs a behaviour until something calls `Scene::update`: `start` fires from
`startPending()` at the top of the first update, and every `onUpdate` follows it. `Scene::render`
does **not** wait for `start` — it draws every enabled component — so a scene that is never updated
is still fully drawn. That split *is* Edit mode: built-in renderers and cameras draw, behaviours
never instantiate.

`dev/PlaySession` owns the editor's state, `Edit | Playing | Paused`:

- **Play** saves the scene to text with `SceneCodec::save` and hands it to `Engine::loadScene`, which
  is `scene.clear()`, then `LuaHost::boot()` — a fresh `lua_State` — then `SceneCodec::load`. Play
  therefore takes exactly the path the player takes from disk. Every session starts with fresh
  prototypes and no leftover coroutines, and a serialization bug shows up on Play rather than on
  Stop.
- **Stop** hands the same text to `loadScene` and releases a locked cursor. Play and Stop are one
  operation; only whether the loop simulates afterwards differs.
- **Pause** switches to `GameLoop::idle`; **Step** runs one `GameLoop::step`.

The order inside `loadScene` is load-bearing: every `Behaviour` holds the `lua_State*` it was created
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
The target uses `swapchain.format()` (sRGB) so the encode/decode round trip is identity; a UNORM
target would visibly brighten everything.

**The target follows the window unless the host embeds it.** `setViewportSize(w, h)`, in window
points, detaches it: both targets are rebuilt at `w × h` times the framebuffer scale, the passes are
resized to `w × h`, and the present pass stops drawing the composite triangle — the overlay shows
the target instead. While embedded, `createTargets` registers each target's image view with the
overlay through `Overlay::addTexture`, and `Renderer::viewport()` returns the current frame's
registration. The target's own descriptor set cannot stand in for it: ImGui's Vulkan backend binds
user textures as `SAMPLED_IMAGE` sets with its own sampler, and validation rejects a
combined-image-sampler set there. `destroyTargets` removes the registrations, and every rebuild sits
behind a `vkDeviceWaitIdle`, so no set is freed while a frame still reads it. A size change rebuilds
on the spot, so the set the panel draws with in that same frame is already the new one. `player`
never calls it and composites exactly as before.

The scene pass carries a second subpass dependency (`0 -> EXTERNAL`, color-write -> fragment-read)
that orders the composite's sample after the scene's writes. Any new pass that reads a previous
pass's output needs the same treatment.

`DrawPass` (`beginFrame` / `record` / `registerApi` / `resize`) is the extension point. It is named
`DrawPass`, not `RenderPass`, so that `renderPass` unambiguously means a `VkRenderPass`. A new pass
means: implement the interface, add it to `passes_` in the `Renderer` constructor in draw order, and
let it register its own Lua functions. Passes own their cameras (`MeshPass` -> `PerspectiveCamera`,
`SpritePass` -> `OrthographicCamera`).

Sprite space is **Y-down** — the ortho camera deliberately has no Y flip, so `y = 0` is the top of
the screen. The perspective camera does flip, in the projection matrix rather than with a negative
viewport height.

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

**Components never store those handles**, because a handle means nothing on the next run.
`SpriteRenderer::texture` and `MeshRenderer::texture` / `mesh` are *names* — a project-relative path,
or a primitive such as `"cube"` — resolved through `DrawList::textureHandle` / `meshHandle` on first
draw and cached until `propChanged` says the name changed. A missing file logs once and draws white;
an unknown mesh logs once and draws a cube. The clear colour and the 2D virtual size are `Camera`
props, pushed to the renderer every frame through `DrawList::background` and `camera2d`, so a scene
file carries them.

`Renderer::capture(path)` reads the target back to a PNG. It stalls the device — a debug tool, not a
per-frame feature.

## The dev overlay

**The dev tools are a separate link target, not a runtime flag.** `player` links `engine` and
`editor` links `engine_dev`, so ImGui is physically absent from the shipping binary — `nm
build/player | grep -i imgui` returns nothing. There is no `--dev`: to get the tools, run `editor`.

The seam is [`gfx/Overlay.hpp`](src/gfx/Overlay.hpp) — a pure interface (`beginFrame`, `record`,
`discardFrame`, `setMinImageCount`, `addTexture`, `removeTexture`) plus an `OverlayFactory` typedef.
`gfx` knows only that.
`dev/ImGuiLayer` is the only implementation; it owns the ImGui context and both backends, and draws
**inside the present pass**, where the player draws the composite triangle. It samples the scene
render target and never writes it.

`Renderer`'s constructor takes an `OverlayFactory`. `player` passes nothing and the pointer stays
null; `editor` passes `cinder::dev::overlayFactory()`. Panels are a second, separate hook —
`setOverlayDraw(std::function<void()>)`, called from `beginFrame()` between `ImGui::NewFrame` and the
`ImGui::Render` that happens during command recording. `editor/main.cpp` sets it to draw
`dev/Toolbar`, `dev/Dockspace`, `dev/Viewport` and `dev/Console`, in that order — the dockspace has
to be submitted before the windows it hosts. The two hooks together are what keep `gfx` free of both
ImGui and `script`.

What remains in the shipping binary is a null `unique_ptr`, an empty `std::function`, a zero
viewport size, and three branches per frame. That is the whole cost of the seam.

Three things about the ImGui frame lifecycle are load-bearing:

- `NewFrame` and `Render` must pair exactly once per frame. `drawFrame()` can bail out early on
  `VK_ERROR_OUT_OF_DATE_KHR` without recording, so that path calls `discardFrame()` — otherwise the
  next `NewFrame` asserts.
- `Input` installs its GLFW callbacks in its constructor, and `Engine` declares `input_` **before**
  `renderer_`. ImGui's GLFW backend therefore installs second and chains to `Input`'s callbacks.
  Swapping that declaration order silently breaks engine input.
- volk is handled by `IMGUI_IMPL_VULKAN_USE_VOLK`, set on the `imgui` target. The backend then uses
  volk's loaded pointers directly and no `ImGui_ImplVulkan_LoadFunctions` shim is needed, because
  `VkCtx` has already called `volkLoadInstance` and `volkLoadDevice` by the time the layer is built.

ImGui creates its own descriptor pool via `DescriptorPoolSize`, for the same reason `RenderTarget`
does not route through `Assets`: that pool has no `FREE_DESCRIPTOR_SET` flag. ImGui's does, which is
what lets `removeTexture` free the Scene panel's sets on every resize.

`io.IniFilename` is `nullptr`, so no `imgui.ini` is written yet. Turning it on is the "editor layout
persisted between runs" item in `TODO.md`.

The toolbar is the main menu bar, not a window, so it takes no dock slot and the dockspace sits
below it. Its shortcuts use `ImGui::Shortcut` with `ImGuiInputFlags_RouteGlobal`, so they work while
the cursor is locked by the game. `ImGuiMod_Ctrl` is ⌘ on macOS.

### The Scene viewport

`dev/Viewport` is the "Scene" window. Each frame it measures its content region, hands the size to
`Renderer::setViewportSize` and the region's top-left to `Input::setViewportOrigin`, then draws
`Renderer::viewport()` over an `InvisibleButton` covering the region — the button is what stops a
click on the scene from dragging the window. The order is load-bearing: `setViewportSize` may
rebuild the targets, so `viewport()` is read after it.

`Input::mouseX`/`mouseY` subtract that origin, so `engine.mousePosition()` and `screenToWorld` work
in the panel's own points, top-left at zero, exactly as they do across the player's whole window.

The viewport also owns input routing, through `Input::setSuppressed`. The game gets the **keyboard
while the Scene window is focused** and the **mouse while the image is hovered**, or while a press that
started on it is held. A locked cursor gives the game both and sets `ImGuiConfigFlags_NoMouse`: GLFW
still reports a virtual cursor while disabled, and ImGui would otherwise click whatever panel it
wanders over. Entering `Playing` focuses the window, so Play and Resume hand the game the keyboard
without a click on the scene first. The console opens with `NoFocusOnAppearing`: every new window
takes focus on its first frame, and the console is submitted after the Scene, so without the flag
`editor --play` would start with the keyboard in the console. The flags are set while building
frame N's overlay and read by frame N+1's updates.

`dev/Dockspace` builds the default layout — Scene above, Console below — with the `DockBuilder`
API from `imgui_internal.h`, once, when the dockspace node does not exist yet. With no ini file,
that is every launch.

### Logging and the console

Everything prints through `platform/Log.hpp` — `logInfo` / `logError`, printf-style and
`__attribute__((format))`-checked. Both always write to stdout/stderr, and additionally to a
`LogSink` if one is installed. `dev/Console` installs that sink in its constructor and clears it in
its destructor, which is how `[lua]`/`[vk]`/`[serial]` output reaches the panel. The sink receives
the line **without its trailing newline**. `Log` itself stays in `engine`: the sink is the seam a
shipping build will use for a crash log file, so it is not a dev-only facility.

The console's input line runs `LuaHost::eval` against the live `lua_State`. It tries `return <text>`
first and falls back to the raw text, so `1 + 1` prints `2` and a multi-statement chunk still runs.
Results and errors both go back through `logInfo`/`logError`, so they land in the panel like anything
else, with `[console]` as the chunk name. In Edit mode that state holds the prelude and nothing else,
so the console can build a scene — `scene:spawn`, `actor:behaviour(path, data)` — without starting
any of it, and Save writes the result.

Typing in the console does not also drive the game, because the console has focus and the Scene
window does not — see *The Scene viewport*.

## Scripting

A project's `project.lua` defines a global `project` table (`title`, `width`, `height`, `scene`,
`fixed_hz`) parsed by `ProjectConfig::load` in a throwaway `lua_State`. `scene` is the scene both
executables open; `--scene` overrides it.

There is **no entry script and no top-level game code**. Game code lives in behaviours: per-frame
work belongs in a behaviour's `update`, in a `stepped:connect(fn)` handler, or in a coroutine started
from `start`. Lua errors are caught and printed, not propagated.

### The prelude

`LuaHost` loads three files from `engine/lua/` after `registerApi()` — order matters, since
`scene.lua` and `task.lua` both need the `engine` table to exist:

- **`types.lua`** — `vec2`/`vec3`/`vec4`/`rgba` built by one `vectype(keys)` factory, with
  `+ - * / unary-minus == tostring`, plus `:length()`, `:dot()`, `:normalized()`, `:unpack()` and
  `vec3:cross()`. `rgba` is a `vec4` whose `r/g/b/a` alias `x/y/z/w`. `vecSize(v)` returns the
  component count or nil, and is how the proxy layer tells a vector from a scalar.
- **`scene.lua`** — the `scene` global plus actor and component proxies.
- **`task.lua`** — `task.wait`/`spawn`/`delay`, the `signal()` constructor, and the `stepped` and
  `rendered` signals.

**The proxy layer is pure Lua over the `engine.*` bindings** — there is no C++-side proxy. It works
because `engine.getProp` returns 1-4 values by arity and `setProp` takes them as varargs, so
`compMt.__index` counts returns to build the right vector and `__newindex` calls `value:unpack()`.
Adding a prop therefore needs no scripting-layer change at all. Actor proxies are cached in a
weak-valued table so `scene:find(n) == actor` holds.

These contracts are load-bearing and must not drift:

- `getProp` returns **1-4 values by arity**, and **zero** values when the actor or prop is missing.
- `find` / `parent` return **`nil`**, never `0` or `-1`.
- Actor ids and the handles `loadTexture`/`newCube` return push as **integers**; numeric prop values
  push as **floats**, string and enum props as strings.
- Edge-triggered input clears in `consume()` **per fixed step**, not per frame.
- `screenToWorld` takes **window points**, matching `mousePosition`.

Writing an unknown actor property **errors**; unknown component props print `[lua] X has no prop Y`
from `SceneApi`, since that is C++-side.

Behaviours are named script files that `return` a table: function values are behaviour, everything
else is a serializable field with a default. `Behaviour::BOOTSTRAP` caches one prototype per path,
copies it per instance, applies the `data` overrides, wraps `start` in `task.spawn` so it can yield,
and sets `self.actor` to an actor proxy.

`Behaviour` is also a `PropBag`, so a scene saves its `data` overrides as a `data { ... }` block.
Before `start` the bag is the pending `data` table; after it, `__behaviourFields`. `script/LuaProps`
converts between Lua values and `PropValue` — scalars, arrays of scalars, string-keyed records, and
vectors (detected by their metatable's `__vec`) as sequences of numbers. Functions, actor proxies and
anything else with a metatable are skipped. On the way back in, `__behaviourNew` rebuilds a sequence
into a vector when the prototype's default for that key is one — the prototype is the schema.

`tests/selftest` is a project whose scene runs a 44-check smoke test for this whole layer from a
behaviour's `start`. `./build/editor tests/selftest --play --frames 120` or `./build/player
tests/selftest` prints `ALL PASS`. It needs a window, so it is not part of `ctest`.

**Where to add a Lua function:** `LuaHost::registerApi()` builds the global `engine` table and binds
the engine-wide calls (time, quit, log, input, textures), then hands the `LuaApi` to
`registerSceneApi` and to `renderer.registerApi()`, which forwards it to every pass. Bind a function
in the class that owns the state it touches — draw and camera calls belong in `SpritePass`/`MeshPass`,
not in `LuaHost`.

`lua_CFunction` cannot capture, so each binding carries its receiver as a light-userdata upvalue:
`api.bind("name", fn, &receiver)`, read back with `LuaApi::context<T>(state)`. Use
`LuaApi::optFloat`/`optInt` for optional numeric arguments.

### Hot reload

Only behaviours hot-reload. Editing a behaviour reloads only that file.
`__behaviourForget(path)` drops the cached prototype, then every live `Behaviour` whose `script()`
matches re-instantiates through `__behaviourNew`, carrying its current fields across as the `data`
overrides. `__behaviourFields` is what decides what "its current fields" means — everything that is
not a function and not `actor`, the same split that makes a behaviour table serializable. The scene,
the `lua_State`, and every other behaviour survive untouched.

`__behaviourRead` is the single funnel every behaviour file is read through, so it is also where
`LuaHost` records the path to watch. Nothing walks a directory; a behaviour is watched because it was
loaded. `poll()` stats each loaded behaviour every frame. In Edit mode nothing has been loaded, so
nothing is watched — and Play reloads every prototype from disk anyway, so edits made while editing
are picked up by the next Play.

Three things the reload deliberately does **not** do:

- It does not re-fire `start` or `destroy`. A reload is a code swap on a live object, not a lifecycle
  event, and re-running `start` would clobber the fields just carried over.
- It does not cancel coroutines the old instance spawned. `task.spawn` tracks no owner, so a loop
  started by the old table keeps running against the old table until Stop discards the state.
- A file that fails to load leaves the running instance alone — `Behaviour::reload` only swaps `ref_`
  once the new instance exists, so a syntax error mid-edit costs nothing.

The prelude is not watched; editing `types.lua`, `scene.lua` or `task.lua` needs a restart.

## Reflection

There is no runtime reflection in C++, so props are declared with a macro that stringises the field
name — the identifier *is* the serialization and Lua key:

```cpp
CINDER_COMPONENT(Camera, cinder::scene::Component) {
    CINDER_PROP(projection_);
    CINDER_PROP_S(fov_, 1.0f, 179.0f, 1.0f);
    CINDER_PROP_R(zoom_, 0.05f, 20.0f);
}
```

`props<T>()` builds the list once into a function-local static and a `PropChain` recursion emits the
base class's props first, so `Component::enabled` is always the first prop of every component —
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

`Components` is an instance owned by `Engine`, threaded to `Scene` -> `SceneCodec` / `SceneApi`. It
keeps insertion-ordered iteration (a vector plus two indices) and caches a class-default instance per
type for delta encoding. Re-binding a name keeps its slot — so iteration order is stable across a
reboot — and drops its cached default, so a `Behaviour` default cannot outlive the `lua_State` it
closed over.

`SceneCodec::VERSION` is 2 and `OLDEST` is 2: version 1 stored texture and mesh handles, which cannot
be migrated. `tests/scene_files_test` loads and re-saves every `.scene` under `samples/` and
`tests/selftest/` and requires the bytes to match, so shipped scenes stay canonical as the format
moves.

## Projects and packaging

`platform/Assets` has two roots, and nothing else turns a name into a path:

- **`enginePath`** — shaders and the prelude. `CINDER_ENGINE` if set; else `engine/` next to the
  executable, if it exists, which is the packaged layout; else the source tree's `engine/`, baked in
  at configure time as `CINDER_ENGINE_DEFAULT`.
- **`projectPath`** — `project.lua`, scenes, behaviours and textures. Set from the executable's first
  argument, made absolute; the player falls back to `project/` next to itself.

Every path inside a project — a scene's `script "scripts/riser.lua"`, a `SpriteRenderer.texture` — is
relative to the project root, so a project folder can live anywhere and runs from any working
directory.

`cmake --build build --target package_game` builds `player` and runs `cmake/PackageGame.cmake`, which
stages `build/dist/<project>/` as `player`, `engine/shaders/*.spv`, `engine/lua/*.lua` and
`project/`. The cache variable `CINDER_PACKAGE_PROJECT` picks the project, defaulting to
`samples/sandbox2d`.

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
  A test pins the output under a Turkish locale.
- Sizes and view dimensions come in two flavours and they are not interchangeable: the swapchain and
  render target are in **framebuffer pixels**, while cameras, `resize()` and `screenToWorld` are in
  **window points**. On a Retina display these differ by 2x. GLFW reports cursor positions in points,
  which is why the cameras use them. In the editor the view is the Scene panel rather than the
  window, and `setViewportSize` takes points and converts to pixels itself.
