# TODO

Roadmap toward a Unity/Unreal-shaped engine: an editor where you select an actor, edit its fields,
hit Play, hit Stop, and land back where you started.

**Where we are:** a Vulkan renderer driving an ordered pass list into an offscreen target,
composited to the swapchain by a fullscreen triangle. Sprite batch with atlas support, mesh pipeline
with one directional light, per-pass cameras, edge-triggered input, Lua scripting with file-watch hot
reload. An actor/component scene with a prop system that feeds the serializer and the script
bindings from one declaration. A bidirectional `Archive` with a text backend. ~7,300 lines, 65
headless test cases, and a 44-check Lua selftest.

Foundations, the scene model, the scripting ergonomics and serialization are done. The editor is
next.

---

## Editor shell

- [ ] Add Dear ImGui with its Vulkan backend. `gfx/vk` was kept free of engine concepts precisely so
      the backend can build on it without dragging in passes or assets.
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
- [ ] Console panel — route the `[engine]`/`[lua]`/`[gfx]` prints into it
- [ ] Editor layout + window state persisted between runs

**Done when:** adding `CINDER_PROP(bounciness_)` to any component makes it appear in the inspector, save
to disk, and become undoable — with zero editor code.

## Play mode

- [ ] Play / Pause / Step / Stop
- [ ] Serialize scene on Play, restore on Stop
- [ ] Input routing — the game gets input only when the viewport is focused
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

- [ ] Build target: package the runtime + cooked assets with no editor
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
- [ ] **Lua hot reload discards all state** — a reload rebuilds the whole `lua_State`. Behaviours are
      named files with a per-path prototype cache, so `poll()` could reload one changed file and
      re-instantiate its behaviours instead of tearing down the world. Needs the watch to cover
      `assets/scripts/behaviours/`, not just the entry script.
- [ ] **`poll()` stats the entry script every frame** — move to a watch service, or throttle.
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
