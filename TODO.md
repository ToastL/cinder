# TODO

Roadmap toward a Unity/Unreal-shaped engine: an editor where you select a node, edit its fields,
hit Play, hit Stop, and land back where you started.

**Where we are:** a Vulkan renderer driving an ordered pass list into an offscreen target,
composited to the swapchain by a fullscreen triangle. Sprite batch with atlas support, mesh pipeline
with one directional light, per-pass cameras, edge-triggered input, and Lua scripts in Roblox's shape —
code that runs top to bottom, attributes on nodes, and per-file hot reload that stops what the old
code started. A Roblox-style tree of nodes
with a prop system that feeds the serializer and the script bindings from one declaration. A
bidirectional `Archive` with text and INI backends, and games that are project folders in Unreal's
shape — a `.cinder` file, `Config/`, `Content/` and `Source/` — rather than scripts in the engine. Dear ImGui is up in a separate `engine_dev` target, with a
dockspace holding the Scene viewport and a console with a live Lua REPL, and a play toolbar in the
main menu bar — `player` ships without a byte of it. The editor opens a project in Edit mode, seen
through a free-flying editor camera with a frustum drawn for every scene camera; Play serializes the
scene into a fresh Lua state and Stop restores it. An Explorer shows the whole node tree, scripts
included, and Properties edits the selected node's fields and attributes with no per-type editor
code. A click in the Scene view selects, every edit is undoable, and an unsaved scene is marked and
guarded on close. 116 headless test cases and a 60-check Lua selftest.

Foundations, the scene model, the scripting ergonomics, serialization and the Edit/Play split are
done. The editor shell is underway — ImGui, the console, the toolbar, the Scene viewport, the
editor camera, the Explorer, Properties, picking and undo have landed; transform gizmos are next.

---

## Editor shell

- [x] Add Dear ImGui with its Vulkan backend. `gfx/vk` was kept free of engine concepts precisely so
      the backend can build on it without dragging in passes or assets. Lives in `src/dev`, behind
      the abstract `gfx::Overlay`, drawn inside the present pass. `editor` is the dev build;
      `player` links `engine` and contains no ImGui symbols at all.
- [ ] Docking layout: viewport, explorer, properties, console, asset browser. `dev/Dockspace` is up
      with Scene, Explorer, Properties and Console; the asset browser docks into it when it lands.
- [x] **Viewport** — `dev/Viewport`, the "Scene" window. The target is sized to the panel through
      `Renderer::setViewportSize` and registered with ImGui through `Overlay::addTexture`, and
      `engine.mousePosition()` is relative to the panel's top-left.
- [x] **Explorer** — `dev/Explorer`, the whole node tree with scripts as children: a filter, "+" to
      insert any registered class, right-click to insert, duplicate or delete, and drag to reparent.
      `dev/Selection` holds a node id, so the selection survives Play and Stop.
- [ ] Explorer: multi-select, inline rename, copy and paste
- [x] **Properties** — `dev/Properties` shows one node: its transform when it is spatial, its
      class's props with one widget per `PropType` (enum combos, colour pickers), and its attributes
      with add and remove. An edit during Play fires the game's changed signals.
- [ ] Properties: an asset-reference widget for texture and mesh names, and a node-reference widget
- [ ] Gizmos — translate/rotate/scale handles, snapping. `dev/Gizmos` already draws a frustum for
      every perspective `Camera` in Edit mode, on the Scene window's draw list.
- [x] Mouse picking — `dev/Picking`, a CPU raycast against each `MeshPart`'s cube, each `Sprite`'s
      rectangle and each perspective `Camera`; the Explorer reveals the pick and `dev/Gizmos` outlines it
- [ ] Picking against real mesh bounds — every `MeshPart` draws a cube today, so the cube is exact;
      imported meshes will need their own bounds or triangles
- [x] Editor camera, independent of the game camera — `dev/EditorCamera`, seeded from the scene's
      camera and handed to `Renderer::overrideCamera3d` in Edit mode. Right-drag to look and fly
      with WASD/QE, middle-drag to pan, scroll to dolly. Orthographic scenes still look through the
      scene's 2D camera; a 2D pan/zoom for them is not done.
- [x] **Undo/redo** — `dev/History` snapshots the scene text through `SceneCodec` after each finished
      edit, so every edit is covered with no per-edit code; ⌘Z / ⌘⇧Z and toolbar buttons
- [ ] Undo: a snapshot costs a full scene save per edit — fine at sample scale; move to diffed snapshots
      or a command stack if scenes grow large
- [x] Console panel — every print routes through `platform/Log`, and `dev/Console` installs the
      sink. REPL line evaluates against the live `lua_State`, with history on up/down.
- [x] Toolbar — Play / Pause / Step / Stop and Save, on ⌘P / ⌘⇧P / ⌘⌥P / ⌘S, in the main menu bar
- [ ] Console: click a `file:line` to open it in `$EDITOR`; filter by level; search
- [ ] Editor layout + window state persisted between runs — `io.IniFilename` is currently `nullptr`;
      the file belongs in the project's `Saved/`
- [ ] Project picker and new-project template — the editor takes the project folder or its
      `.cinder` file on the command line; the template writes `Content/Scenes/` and `Content/Textures/`,
      and the picker lists each descriptor's `category` and `description`
- [ ] Project Settings panel — `ProjectConfig::walk` already writes `Config/Game.ini` through `IniSave`
- [ ] Scene switching — open, new and save-as. The editor edits the one scene named by
      `startScene` in `Config/Game.ini` or `--scene`.

**Done when:** adding `CINDER_PROP(bounciness_)` to any node class makes it appear in Properties, save
to disk, and become undoable — with zero editor code.

**Script editing is external, by decision.** Scripts are plain files on disk, so the editor's job is
the round trip — watch, reload per file, preserve state, report errors with a real `file:line` — not
a text buffer that competes with VS Code and loses. An embedded editor for quick tweaks is optional
sugar, never the primary surface.

## Play mode

- [x] Play / Pause / Step / Stop — `dev/PlaySession`, driven by `dev/Toolbar`
- [x] Serialize scene on Play, restore on Stop — both are `Engine::loadScene` on one snapshot, so
      every session also starts from a fresh `lua_State`
- [x] Editor keeps rendering while paused — `GameLoop::idle`
- [x] Game code never runs in Edit mode — there is no entry script, and scripts only run from
      `Scene::update`, which Edit mode never calls
- [x] Input routing — `dev/Viewport` gives the game the keyboard while the Scene window is focused
      and the mouse while its image is hovered; a locked cursor keeps both. Play focuses the Scene.
- [x] Dirty tracking — the toolbar shows `main.scene*`, and closing the window with unsaved changes
      asks Save / Don't Save / Cancel
- [ ] Keep a change made during Play — Unity's "copy component values" escape hatch

## Serialization — what is left

- [ ] Node references — a prop type holding a node id, set by dragging a node from the Explorer into
      Properties. `reflect` is a leaf and cannot include `scene`, so the prop stores the id and
      `scene` resolves it.
- [ ] Prefabs — a saved subtree that can be instanced, with per-instance overrides. `Scene::clone`
      already copies a subtree generically.
- [ ] Tags — one script driving every node carrying a tag, like Roblox's `CollectionService`

## Asset pipeline

- [ ] GUIDs — a `.meta` sidecar per asset, stable across renames and moves. Scenes currently store
      the path relative to `Content/` or `Source/`, which a rename breaks.
- [ ] Asset database: GUID -> path -> loaded handle. `DrawList::textureHandle` / `meshHandle` are
      already the lookup seam; they just need GUID lookup in front.
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
      scripts; debug draw for colliders
- [ ] **Audio** — clip loading, one-shot + looping sources, 3D positional, buses/volume
- [ ] **Animation** — skeletal (skinning shader, joint palette), sprite sheet animation, and a
      clip/state machine
- [ ] **UI system** — the current "HUD" is `SpriteBatch` with an ortho camera. Needs anchoring,
      layout, text rendering (MSDF or stb_truetype), input hit-testing.
- [ ] **Particles** — emitters as nodes, GPU-driven if it matters
- [ ] Scene queries — raycast, overlap, spatial partition (BVH or grid)
- [ ] Timers, event bus

## Ship a game

- [x] Build target — `player` is `EXCLUDE_FROM_ALL`, and `package_game` stages it with the engine
      data and one project into `build/dist/<project>/`, runnable from any working directory
- [ ] "Build Game" in the editor — run `package_game` for the open project
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
- [ ] **Scripts cannot share code.** There is no `ModuleScript`/`require`: `package.path` does not
      include the project, so a helper has to be copied or hung off `_G` by another script. `Source/`
      is the natural root — `Source/?.lua`, with required files watched like scripts.
- [ ] **Half the project descriptor is read by nothing.** `modules`, `plugins`,
      `additionalRootDirectories`, `additionalPluginDirectories` and `disableEnginePluginsByDefault`
      are parsed, checked and saved; they wait for a plugin system and `require`.
- [ ] **The prelude is not watched** — editing `types.lua`, `scene.lua` or `task.lua` needs a restart.
- [ ] **`poll()` stats every script it has read, every frame** — move to a watch service, or throttle.
- [ ] **An attribute cannot name a node** — it holds numbers, strings, bools and small vectors. A node
      reference needs *Node references*.
- [ ] **A disabled parent does not stop its child scripts.** `enabled` skips a subtree's update and
      render, but a `Script` stops only when its own `enabled` is written.
- [ ] **Runtime meshes cannot be named in a scene** — `MeshRenderer.mesh` names a primitive, and
      `engine.newCube` hands back an anonymous handle. Mesh import is where named meshes come from.
- [ ] Fixed caps with no growth path: `MAX_QUADS = 10000`, `MAX_DRAWS = 4096`, `MAX_TEXTURES = 256`
- [ ] **`Renderer::capture` stalls the device** and reads the target back synchronously. It is a
      debug tool; do not call it per frame. `RenderTarget` carries `TRANSFER_SRC_BIT` only for it.
- [ ] **Resizing the Scene panel stalls the device.** `Renderer::setViewportSize` calls
      `vkDeviceWaitIdle` and rebuilds both targets on every size change, so dragging a dock splitter
      stalls once per frame. Rebuilding each target when its own fence comes round would avoid it,
      but the ImGui registration already recorded for that frame would have to survive the rebuild.
- [ ] **ImGui blends the Scene image by its alpha.** The mesh pipeline writes `albedo.a` unblended,
      so a translucent mesh lets the panel background through in the editor, while the player's
      opaque swapchain ignores it. Sprites are fine — their alpha factors keep the target at 1.
- [ ] **The layer graph is convention, not enforced.** See the Layering section of `CLAUDE.md`.
      `#include` cycles are invisible in a way package cycles are not, so this is worth re-checking
      by eye when adding a subdirectory.
- [ ] **The points-vs-pixels split is implicit.** Cameras, `resize()` and `screenToWorld` are in
      window points; the swapchain and render target are in framebuffer pixels. 2x apart on Retina.
      `cursor.lua` and `spawner.lua` in `samples/sandbox2d` depend on the current behaviour.
- [ ] **The packaged player finds itself through `argv[0]`**, so launching it through a `PATH`
      lookup rather than by path misses `engine/` and `project/` next to it
- [ ] No CI

## Explicitly out of scope

Naming these keeps them from creeping in:

- Feature parity with Roblox/Unity/Unreal. Roblox's Explorer, Properties and Play loop over a tree of
  nodes (Godot's structure too) is the shape being copied — the *workflow*, not the feature list.
- Console platforms
- Networking / multiplayer
- Visual scripting
- A second scripting language beyond Lua
- Ray tracing
