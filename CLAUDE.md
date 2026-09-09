# CLAUDE.md

Guidance for Claude Code (claude.ai/code) when working in this repository.

## What this is

A from-scratch C++20 game engine: Vulkan renderer, GLFW windowing, Lua 5.4 scripting. The long-term
target is a Unity/Unreal-shaped editor workflow — select an actor, edit its fields, hit Play, hit
Stop, land back where you started. `TODO.md` is the authoritative roadmap; read it before proposing
architectural work, since it records what is deliberately deferred and what is out of scope.

## Commands

```bash
cmake -S . -B build -G Ninja && cmake --build build
```

```bash
ctest --test-dir build --output-on-failure
```

```bash
./build/game
```

`game` takes `--assets <dir>`, `--script <path>`, `--frames <n>` and `--capture <png>`. The last two
make it scriptable: `--frames 90 --capture out.png` runs headless-ish and writes a screenshot.

The first configure fetches every dependency and needs network — glfw, glm, lua, VMA, stb, volk,
Vulkan-Headers and doctest, all pinned in `cmake/Dependencies.cmake`. Nothing needs installing.
`glslangValidator` is the one exception: it is a *build tool*, found with `find_program`, and it
compiles `assets/shaders/*.{vert,frag}` to `.spv`. Editing a shader needs a rebuild, not just a
restart. `brew install glslang` if it is missing.

`editor` builds and prints a placeholder. It exists so the editor can own its own loop later.

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
DYLD_LIBRARY_PATH=/opt/homebrew/lib ./build/game
```

**Do this whenever touching the renderer.** Without it you are running unvalidated — it has already
caught a missing `TRANSFER_SRC_BIT` that no test would have.

## Layout

```
cinder/
  CMakeLists.txt
  cmake/          dependency, Lua and shader-compilation modules
  src/
    reflect/ lua/ platform/          leaves
    scene/ serial/ components/       the world model
    gfx/ gfx/vk/ gfx/asset/ gfx/pass/  the renderer
    script/ core/                    the Lua host and the engine
    game/ editor/                    the two executables
  tests/
```

`src/` is the only include root, so every include carries its layer: `#include "scene/Actor.hpp"`.
Headers sit next to their sources. `assets/` holds the shaders, the Lua prelude and the demo
scripts.

## Layering

The layer graph is **acyclic and strictly layered**. Keep it that way — this is what lets the editor
depend on `scene` + `reflect` without dragging in Vulkan or Lua.

```
reflect, lua, platform     -> leaves (no engine includes)
scene                      -> reflect
serial                     -> scene, reflect
components                 -> scene, reflect
gfx/vk                     -> platform
gfx/asset                  -> gfx/vk, scene
gfx/pass                   -> gfx/asset, gfx/vk, scene, lua
gfx                        -> gfx/pass, gfx/asset, gfx/vk, scene, platform, lua
script                     -> scene, reflect, gfx, platform, lua
core                       -> all of the above
```

`gfx` is split four ways and the seam that matters is `gfx/vk`: it knows Vulkan and knows nothing
about this engine, so an ImGui Vulkan backend can build on it without dragging in passes or assets.
`gfx/asset` is what you draw with (`Assets`, `Texture`, `Mesh`), `gfx/pass` is how you draw it
(`DrawPass` and its implementations, their pipelines and cameras), and `gfx` itself is only the
orchestrator — `Renderer`, `RenderTarget`, `CompositePipeline`, `RendererDrawList`. Nothing in a
subdirectory includes its parent.

Four placements are load-bearing and were each chosen to kill a cycle: `Glfw`/`Window`/`Input` live
in `platform`, not next to `Engine`; `DrawList` lives in `scene`, so the scene graph never includes
`gfx`; `LuaApi` lives in `lua`, so a pass can bind its own functions without including the script
host; and `PropBag`/`PropValue` live in `scene`, not `serial`, so `Behaviour` can expose its
Lua-side fields without `script` gaining an edge to the serializer.

`LuaHost` takes `(path, Scene&, Input&, Renderer&, quit)` — never `Engine&` — for the same reason.

## Frame flow

`main` -> `Glfw::acquire()` -> `Engine(config)` -> `GameLoop::tick(engine)` per iteration:

1. `Glfw::pollEvents()`, then `engine.beginFrame()` -> `script->poll()` (Lua file-watch hot reload)
2. minimized -> `Glfw::waitEvents()`, reset the clock, skip the frame
3. `GameLoop::advance()` accumulates real time, clamped at `MAX_FRAME_TIME = 0.25s`
4. `engine.update(fixedDt)` N times — fixed timestep; `script->update`, `scene.update`, then
   `input.consume()`
5. `engine.render(alpha())` once — `renderer.beginFrame()`, `script->render(alpha)`,
   `scene.render(alpha, draws)`, `renderer.drawFrame()`

`Engine` is a library that gets ticked, not something that runs itself. **Do not move the loop into
`Engine`.** `GameLoop::advance()` and `alpha()` are public precisely so the tests can drive them
without a window; keep new loop logic in that shape.

`ScriptHost::update` calls the Lua global `__step(dt)` with the fixed `dt`, and `render` calls
`__render(alpha)`. Both live in `assets/scripts/lib/task.lua`, which steps the coroutine scheduler
and fires the `stepped` and `rendered` signals.

## Rendering

`Renderer` owns two `VkRenderPass`es and 2 frames in flight. The **scene pass** (color + depth) draws
an ordered `vector<unique_ptr<DrawPass>>` — `MeshPass` first, then `SpritePass` — into a
`RenderTarget`, and ends in `SHADER_READ_ONLY_OPTIMAL`. The **present pass** draws one fullscreen
triangle (`CompositePipeline`) sampling that target into the swapchain framebuffer.

`RenderTarget` is a VMA color image + `DepthBuffer` + framebuffer, plus its own sampler, 1-set
descriptor pool and descriptor set — deliberately *not* routed through `Assets`, whose pool has no
`FREE_DESCRIPTOR_SET` flag and would leak a set per resize. There is one target **per frame in
flight**; a single one would be cleared by frame N+1 while frame N's composite still sampled it.
`Renderer::viewport()` returns the current frame's descriptor set — that is the editor viewport
handle. The target uses `swapchain.format()` (sRGB) so the encode/decode round trip is identity; a
UNORM target would visibly brighten everything.

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

`Renderer::capture(path)` reads the target back to a PNG. It stalls the device — a debug tool, not a
per-frame feature.

## Scripting

`assets/game.lua` defines a global `game` table (`title`, `width`, `height`, `script`, `fixed_hz`)
parsed by `GameConfig::load` in a throwaway `lua_State`. Switch demos by pointing `script` at another
file, or pass `--script`.

A game script is **plain top-level code** — it runs once when loaded, Roblox-style. There are no
`on_load`/`on_update`/`on_render` globals; per-frame work belongs in a behaviour's `update`, in a
`stepped:connect(fn)` handler, or in a coroutine. Lua errors are caught and printed, not propagated.

### The prelude

`LuaHost` loads three files from `assets/scripts/lib/` after `registerApi()` — order matters, since
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
- Actor ids and texture/mesh handles push as **integers**; prop values push as **floats**.
- Edge-triggered input clears in `consume()` **per fixed step**, not per frame.
- `screenToWorld` takes **window points**, matching `mousePosition`.

Writing an unknown actor property **errors**; unknown component props print `[lua] X has no prop Y`
from `SceneApi`, since that is C++-side.

Behaviours are named script files that `return` a table: function values are behaviour, everything
else is a serializable field with a default. `Behaviour::BOOTSTRAP` caches one prototype per path,
copies it per instance, applies the `data` overrides, wraps `start` in `task.spawn` so it can yield,
and sets `self.actor` to an actor proxy.

`assets/scripts/selftest.lua` is a 44-check smoke test for this whole layer — run it with
`--script assets/scripts/selftest.lua` and it prints `ALL PASS` and quits. It needs a window, so it
is not part of `ctest`.

**Where to add a Lua function:** `LuaHost::registerApi()` builds the global `engine` table and binds
the engine-wide calls (time, quit, log, input, textures, clear color), then hands the `LuaApi` to
`registerSceneApi` and to `renderer.registerApi()`, which forwards it to every pass. Bind a function
in the class that owns the state it touches — draw and camera calls belong in `SpritePass`/`MeshPass`,
not in `LuaHost`.

`lua_CFunction` cannot capture, so each binding carries its receiver as a light-userdata upvalue:
`api.bind("name", fn, &receiver)`, read back with `LuaApi::context<T>(state)`. Use
`LuaApi::optFloat`/`optInt` for optional numeric arguments.

Hot reload rebuilds the entire `lua_State` and re-runs the game script, so **all script state is lost
on reload**. `poll()` stats the entry script every frame and watches nothing else — a behaviour or
prelude file can be edited without triggering a reload.

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
  `dirty()`.

`Components` is an instance owned by `Engine`, threaded to `Scene` -> `SceneCodec` / `SceneApi`. It
keeps insertion-ordered iteration (a vector plus two indices) and caches a class-default instance per
type for delta encoding. Re-binding a name keeps its slot — so iteration order is stable across a
hot reload — and drops its cached default, so a `Behaviour` default cannot outlive the `lua_State` it
closed over.

## Conventions

- **Zero comments.** ~7,300 lines with no comments or doc blocks, by choice. Match it; explain in
  chat or in these docs.
- Members carry a trailing underscore. `CINDER_PROP(position_)` strips it, so the wire key stays
  `position`. This is also why `Camera`'s clip planes are `near_`/`far_` — `near` and `far` are
  macros in `windef.h`.
- `Glfw` is refcounted `acquire`/`release`. `Window` acquires in its constructor and releases in its
  destructor, and `main` holds an outer acquire across the whole run.
- `Input` is edge-triggered: `keyPressed`/`keyReleased` are true for exactly one fixed update,
  cleared by `input.consume()` at the end of `Engine::update`.
- **`std::to_chars` everywhere in `serial`** — never `printf` or `ostream`, which follow the locale.
  A test pins the output under a Turkish locale.
- Sizes and view dimensions come in two flavours and they are not interchangeable: the swapchain and
  render target are in **framebuffer pixels**, while cameras, `resize()` and `screenToWorld` are in
  **window points**. On a Retina display these differ by 2x. GLFW reports cursor positions in points,
  which is why the cameras use them.
