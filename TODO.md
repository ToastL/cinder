# TODO

Roadmap toward a Unity/Unreal-shaped engine: an editor where you select an actor, edit its fields,
hit Play, hit Stop, and land back where you started.

**Where we are:** a Vulkan renderer driving an ordered pass list into an offscreen target,
composited to the swapchain by a fullscreen triangle. Sprite batch with atlas support, mesh pipeline
with one directional light, per-pass cameras, edge-triggered input, Lua scripting with file-watch hot
reload — per-file for behaviours, carrying instance fields across the swap. An actor/component scene
with a prop system that feeds the serializer and the script bindings from one declaration. A
bidirectional `Archive` with a text backend. Dear ImGui is up in a separate `engine_dev` target,
with a console panel and a live Lua REPL — `game` ships without a byte of it. ~8,000 lines, 73
headless test cases, and a 44-check Lua selftest.

Foundations, the scene model, the scripting ergonomics and serialization are done. The editor shell
is underway — ImGui and the console have landed; the viewport, hierarchy and inspector are next.

---

## Editor shell

- [x] Add Dear ImGui with its Vulkan backend. `gfx/vk` was kept free of engine concepts precisely so
      the backend can build on it without dragging in passes or assets. Lives in `src/dev`, behind
      the abstract `gfx::Overlay`, drawn inside the present pass after the composite. `editor` is
      the dev build; `game` links `engine` and contains no ImGui symbols at all.
- [ ] Docking layout: viewport, hierarchy, inspector, console, asset browser
- [ ] **Viewport** — the offscreen target as an ImGui image. `Renderer::viewport()` already returns
      the descriptor set; it needs resizing to the panel, not the window.
- [ ] **Hierarchy** — tree of actors, drag to reparent, multi-select
- [ ] **Inspector** — iterate `props<T>()`, one widget per field type (`float`, `int`, `bool`,
      `std::string`, `glm::vec3`, color, asset reference, enum). `PropDef` already carries `label`,
      `min`, `max` and `step`; only `step` and `label` are currently unread.
- [ ] Gizmos — translate/rotate/scale handles, snapping
- [ ] Mouse picking — click the viewport to select (id buffer or CPU raycast)
- [ ] Editor camera, independent of the game camera
- [ ] **Undo/redo** — command stack recording `(component, field, old, new)`
- [x] Console panel — every print routes through `platform/Log`, and `dev/Console` installs the
      sink. REPL line evaluates against the live `lua_State`, with history on up/down.
- [ ] Console: click a `file:line` to open it in `$EDITOR`; filter by level; search
- [ ] Editor layout + window state persisted between runs — `io.IniFilename` is currently `nullptr`

**Done when:** adding `CINDER_PROP(bounciness_)` to any component makes it appear in the inspector, save
to disk, and become undoable — with zero editor code.

**Script editing is external, by decision.** Scripts are plain files on disk, so the editor's job is
the round trip — watch, reload per file, preserve state, report errors with a real `file:line` — not
a text buffer that competes with VS Code and loses. An embedded editor for quick tweaks is optional
sugar, never the primary surface.

## Play mode

- [ ] Play / Pause / Step / Stop
- [ ] Serialize scene on Play, restore on Stop
- [ ] Input routing — the game gets input only when the viewport is focused. Half of this exists:
      `Engine::beginFrame` already feeds ImGui's `WantCapture*` into `Input::setSuppressed`. What is
      missing is the viewport panel itself to focus.
- [ ] Editor keeps rendering while paused
- [ ] Warn on unsaved changes when entering play mode

## Serialization — what is left

- [ ] Cross-actor references. Needs an actor-reference prop type, which nothing declares yet, and a
      layering decision: `reflect` is a leaf and cannot include `scene`, so `Actor` would need a
      marker interface in `reflect`.
- [ ] Prefabs — a saved subtree that can be instanced, with per-instance overrides

## Asset pipeline

- [ ] GUIDs — a `.meta` sidecar per asset, stable across renames and moves
- [ ] Asset database: GUID -> path -> loaded handle. `Assets`' int handles are already the right
      shape; they just need GUID lookup in front.
- [ ] Import pipeline: source file -> cooked asset, cached, re-run on mtime change
- [ ] Reference counting + **unload** — `Assets` never frees a texture until shutdown, and
      `MAX_TEXTURES = 256` is a hard cap with a fixed descriptor pool
- [ ] Hot reload for textures and meshes (Lua already reloads)
- [ ] Shader hot reload. Shaders are compiled to SPIR-V at build time, so editing one needs a
      rebuild. A runtime GLSL path behind a debug flag would restore the edit-and-restart loop.
- [ ] Mesh import
- [ ] Asset browser panel with thumbnails

## Gameplay systems

- [ ] **Physics** — rigid bodies, colliders, raycasts, triggers; collision callbacks routed to
      components; debug draw for colliders
- [ ] **Audio** — clip loading, one-shot + looping sources, 3D positional, buses/volume
- [ ] **Animation** — skeletal (skinning shader, joint palette), sprite sheet animation, and a
      clip/state machine
- [ ] **UI system** — the current "HUD" is `SpriteBatch` with an ortho camera. Needs anchoring,
      layout, text rendering (MSDF or stb_truetype), input hit-testing.
- [ ] **Particles** — emitters as components, GPU-driven if it matters
- [ ] Scene queries — raycast, overlap, spatial partition (BVH or grid)
- [ ] Timers, event bus

## Ship a game

- [ ] Build target: package the runtime + cooked assets with no editor. The code split is done —
      `game` links `engine`, `editor` links `engine_dev`. What is left is the packaging.
- [ ] Asset bundling into an archive, not loose files
- [ ] Settings/save-data location per OS
- [ ] Crash handler + log file

---

## Rendering (ongoing, not blocking)

- [ ] Multiple lights + a real material model (currently one hardcoded directional light in
      `mesh.frag`, and no material struct at all)
- [ ] Shadow maps
- [ ] Sort transparent draws back-to-front; sort opaque by pipeline/material
- [ ] Frustum culling
- [ ] Mipmaps — `Texture` hardcodes `mipLevels = 1` and `MIPMAP_MODE_NEAREST`
- [ ] Anisotropic filtering — `anisotropyEnable = VK_FALSE` in the shared sampler
- [ ] MSAA
- [ ] Post-processing chain (tonemap, bloom)
- [ ] Instanced rendering — `MeshPass` issues one `vkCmdDrawIndexed` per object
- [ ] Bindless textures or a descriptor array, to remove the per-texture bind
- [ ] Compute pass support

## Known issues & tech debt

- [ ] **Everything is host-visible memory.** `GpuBuffer` always maps; `Mesh` writes vertices straight
      into a mapped buffer. Static geometry wants device-local + staging.
- [ ] **Sprite batching is run-based** — `SpriteBatch::flush` only merges *consecutive* quads sharing
      a texture, so interleaved textures degrade to one draw call per quad. Sort by texture, or go
      bindless.
- [ ] **No sorting anywhere**, in either pass. Submission order only.
- [ ] **`endSingleTime` does a full `vkQueueWaitIdle`** per texture upload.
- [ ] **Editing the entry script still discards all state** — it rebuilds the whole `lua_State` and
      clears the scene, because a game script builds the world in top-level code. This stops being a
      problem once the scene loads from a `.scene` file and the entry script is only bootstrap.
- [ ] **A behaviour reload leaks the old instance's coroutines** — `task.spawn` tracks no owner, so a
      loop started by the pre-reload table keeps running against it. Needs threads tagged with the
      instance that spawned them, and dropped on reload and destroy.
- [ ] **The prelude is not watched** — editing `types.lua`, `scene.lua` or `task.lua` needs a restart.
- [ ] **`poll()` stats the entry script and every loaded behaviour every frame** — move to a watch
      service, or throttle.
- [ ] Fixed caps with no growth path: `MAX_QUADS = 10000`, `MAX_DRAWS = 4096`, `MAX_TEXTURES = 256`
- [ ] **`Renderer::capture` stalls the device** and reads the target back synchronously. It is a
      debug tool; do not call it per frame. `RenderTarget` carries `TRANSFER_SRC_BIT` only for it.
- [ ] **The layer graph is convention, not enforced.** See the Layering section of `CLAUDE.md`.
      `#include` cycles are invisible in a way package cycles are not, so this is worth re-checking
      by eye when adding a subdirectory.
- [ ] **The points-vs-pixels split is implicit.** Cameras, `resize()` and `screenToWorld` are in
      window points; the swapchain and render target are in framebuffer pixels. 2x apart on Retina.
      `cursor.lua` and `spawner.lua` depend on the current behaviour.
- [ ] **The asset root is baked in at configure time.** `CINDER_ASSETS_DEFAULT` records the source
      path; `--assets` and `CINDER_ASSETS` override it. A relocatable build wants the assets copied
      next to the binary instead.
- [ ] No CI

## Explicitly out of scope

Naming these keeps them from creeping in:

- Feature parity with Unity/Unreal. Unity's inspector-and-play-button loop and Unreal's
  actor/component model are the shape being copied — the *workflow*, not the feature list.
- Console platforms
- Networking / multiplayer
- Visual scripting
- A second scripting language beyond Lua
- Ray tracing
