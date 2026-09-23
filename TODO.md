# TODO

Roadmap toward a Unity/Unreal-shaped engine: an editor where you select a node, edit its fields,
hit Play, hit Stop, and land back where you started.

**Where we are:** a Vulkan renderer driving an ordered pass list into an offscreen target,
composited to the swapchain by a fullscreen triangle. Sprite batch with atlas support, mesh pipeline
with one directional light, one camera that every pass draws through, edge-triggered input, and Lua scripts in Roblox's shape —
code that runs top to bottom, attributes on nodes, and per-file hot reload that stops what the old
code started. A Roblox-style tree of nodes
with a prop system that feeds the serializer and the script bindings from one declaration. A
bidirectional `Archive` with text and INI backends, and games that are project folders in Unreal's
shape — a `.cinder` file, `Config/`, `Content/` and `Source/` — rather than scripts in the engine. Dear ImGui is up in a separate `engine_dev` target, with a
dockspace holding the Scene viewport and a console with a live Lua REPL, and a play toolbar in the
main menu bar — `player` ships without a byte of it. The editor opens a project in Edit mode, seen
through a free-flying editor camera — 2D scenes included — with a gizmo drawn for every scene camera; Play serializes the
scene into a fresh Lua state and Stop restores it. An Explorer shows the whole node tree, scripts
included, and Properties edits the selected node's fields and attributes with no per-type editor
code. A click in the Scene view selects, gizmos move, rotate and scale the selection, every edit is
undoable, and an unsaved scene is marked and guarded on close. A physics engine of our own has
started: bodies and colliders as nodes, spheres and boxes, friction, and a sequential-impulse solver
with warm starting that stacks crates and never runs in Edit mode; boxes, spheres and capsules meet
through a bounding volume tree, settled islands fall asleep, and scripts push bodies around, cast rays
and hear about contacts, with a `planar` body making a 2D scene physical. 222 headless test cases and
a 65-check Lua selftest.

Foundations, the scene model, the scripting ergonomics, serialization and the Edit/Play split are
done. The editor shell is underway — ImGui, the console, the toolbar, the Scene viewport, the
editor camera, the Explorer, Properties, picking, undo and transform gizmos have landed.

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
- [x] **Gizmos** — `dev/Manipulator` moves along world or local axes and planes, rotates on gimbal
      rings that each turn one Euler angle, and scales along local axes or uniformly; ⌘ snaps and
      Esc cancels. 1/2/3 pick the tool and X toggles World/Local. `dev/Gizmos` draws the handles from
      the same shapes the hit test uses, and every scene `Camera` as a frustum.
- [ ] Gizmos: world- and local-space rotation rings and a view-facing ring — rotation is gimbal-only
- [ ] Gizmos: snapping to absolute grid positions, and snap steps set in the editor
- [ ] Gizmos during Play — like picking, they only exist in Edit mode
- [ ] Gizmos for a multi-selection, about the pivot or the selection's centre — waits for Explorer
      multi-select
- [x] Mouse picking — `dev/Picking`, a CPU raycast against each `MeshPart`'s cube, each `Sprite`'s
      quad and each `Camera`; the Explorer reveals the pick and `dev/Gizmos` outlines it
- [ ] Picking against real mesh bounds — `MeshPass` now has a `sphere` primitive that `Picking` still
      treats as a cube, and imported meshes will need their own bounds or triangles
- [x] Editor camera, independent of the game camera — `dev/EditorCamera`, seeded from the scene's
      camera and handed to `Renderer::overrideCamera` in Edit mode, which every pass draws through, so
      a 2D scene is flown like a 3D one. Right-drag to look and fly with WASD/QE, middle-drag to pan,
      scroll to dolly.
- [ ] Scene view 2D mode — Unity's toggle to an orthographic editor camera looking down -Z, panned and
      zoomed rather than flown
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
      and the picker lists each descriptor's `category` and `description`. The *Project browser*
      under *UI framework*, not in ImGui.
- [ ] Project Settings panel — `ProjectConfig::walk` already writes `Config/Game.ini` through `IniSave`
- [ ] Scene switching — open, new and save-as. The editor edits the one scene named by
      `startScene` in `Config/Game.ini` or `--scene`.

**Done when:** adding `CINDER_PROP(bounciness_)` to any node class makes it appear in Properties, save
to disk, and become undoable — with zero editor code.

**Script editing is external, by decision.** Scripts are plain files on disk, so the editor's job is
the round trip — watch, reload per file, preserve state, report errors with a real `file:line` — not
a text buffer that competes with VS Code and loses. An embedded editor for quick tweaks is optional
sugar, never the primary surface.

## UI framework

**The editor's UI is our own, by decision, and it is shaped like Unreal's without borrowing its
names** — not Qt, not RmlUi, and ImGui only until the replacement reaches parity. Unreal builds its
editor on Slate and its game UI on UMG, which wraps Slate; this engine does the same, so one framework
is both the game UI system and the editor's toolkit. Qt would be a second UI system, an installed
dependency with LGPL terms, and it and GLFW both want to own the macOS app; RmlUi would still leave
every editor widget, docking and multiple windows to build. ImGui stays the working `editor` until the
last panel is ported.

The shape is Slate's: a retained tree of widgets with plain names — `Button`, `Label`, `TextBox`,
`HorizontalBox`, `Splitter`, `Canvas` — built with `ui::make<T>()`, slots and chained arguments; layout
in two passes, desired size bottom-up and arrangement top-down; paint into draw elements that a
GPU-free batcher turns into vertices; input routed along the widget path with a `Reply`, focus and
mouse capture; a `Theme` of named styles; docking; menus and shortcuts from a `CommandList`. `ui` never
includes `scene`, `reflect` or `gfx` — the reflection-driven Properties panel lives in `dev`.

- [x] **Text** — `text/` (-> `platform`) on FreeType and HarfBuzz: load a font, shape and measure a
      string, rasterize glyphs into CPU atlas pages, map carets to clusters. No GPU, so layout measures
      text without an edge to `gfx`, and all of it tests headlessly. Roboto and Roboto Mono ship in
      `engine/fonts/`.
- [x] **UI renderer** — `gfx/UiRenderer` records in the present pass after the composite, not as a
      `DrawPass`: every `DrawPass` records in the scene pass, which has depth. Rounded rectangles and
      borders as an SDF, anti-aliased lines and convex fills for gizmos, images including the scene
      target drawn opaque, and glyphs from R8 atlas pages rasterized at the framebuffer scale.
      `ui_gallery` shows all of it and `--capture-window` captures it.
- [x] **Widget core** — `ui/core` (widgets, geometry, attributes, replies, draw elements, hit
      testing, styles), `ui/framework` (the `Application` that routes events, commands and shortcuts)
      and the first widgets: boxes, border, overlay, canvas, scaler, label, image, button, check box,
      scroll box, splitter, text field and box, viewport. `Input` records an ordered event queue with
      characters, repeats and modifiers, and `platform/InputScript` replays one for tests and captures.
- [x] **Vertical slice** — `editor_next` with the toolbar, the Scene view and the console on the new UI
      in a fixed layout: picking, gizmo drags, the editor camera, undo, Play/Stop, the console's Lua
      line and the close prompt all run on it, and `--input-script` drives every one of them.
- [x] **Value editors and menus** — popups and tooltips in the `Application`; `SpinBox` (drag, click
      to type an expression, bounds only when declared, never rounded to its display),
      `VectorInputBox`, `ComboBox`, `ExpandableArea`, `ColorBlock` and `ColorPicker`; `Menu`,
      `MenuBuilder`, `MenuAnchor`, `MenuBar` and context menus, with commands, checks and submenus.
      `editor_next`'s toolbar has File, Edit and Play menus.
- [x] **Lists and trees** — `TreeView` (a `ListView` is one with no children) builds only the rows in
      view, keyed by item id, with selection by mouse and arrow keys, expansion from the arrow or a
      double click, `reveal` through collapsed ancestors, context menus, and drag and drop carrying a
      `DragDropOperation` with a decorator. Tab walks the fields in paint order and stays inside a popup.
- [ ] **Font fallback** — shaping falls back per glyph to a symbol font, so shortcuts can read ⌘⇧P
      instead of `Shift+Cmd+P`. Roboto has no ⌘, and a label that asks for one draws a missing glyph.
- [x] **Docking** — `ui/docking`: a `TabManager` holds the layout as a tree of splits and stacks,
      spawns a tab's panel once and keeps it across re-docks, and rebuilds the widgets on every
      change, carrying the splitter sizes over. A tab drags to another stack's middle or against an
      edge, which splits it; closing the last tab of a stack collapses it; the Window menu reopens a
      closed tab where it last lived. `editor_next` runs on it.
- [ ] **Floating panels** — panels in their own OS windows, after parity: `VkCtx` has to stop owning
      the one GLFW surface — one device, a swapchain per window.
- [ ] **Port the panels** into `editor_next` beside `editor`, then flip the names and delete ImGui and
      `gfx::Overlay`. `PlaySession`, `History`, `Selection`, `Picking`, `EditorCamera`, `Manipulator`,
      `GizmoGeometry` and `GizmoLines` have no ImGui in them and carry over as they are; the new
      panels live in `dev/panels/` — `Toolbar`, `SceneView`, `Explorer`, `Properties`, `Console`.
      `cmake/AssertLayers.cmake` lists the files still allowed to mention ImGui, and the list only
      shrinks.
- [ ] **Game UI** — reflected widget classes saved as `.widget` assets under `Content/`, created from
      Lua and added to the viewport, never nodes in the `Scene`: Unreal's UMG, in its own layer over
      `ui`, `reflect` and `serial`, and its own plan once the editor is on the new UI.
- [ ] **Project browser** — the first screen of `editor` with no argument: recent projects and
      templates, as Unreal's Project Browser and Godot's Project Manager do. Built on the new UI once
      it lands, and designed separately.

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

- [x] **Physics, phase 1** — our own engine in `physics/`, not Jolt or Box2D: `Body` and `Collider` as
      nodes, sphere and box shapes, compound mass and inertia about the centre of mass, a brute-force
      broadphase, exact sphere–sphere and sphere–box contacts, and sequential impulses with Baumgarte
      correction and restitution. `World::step` walks the scene, so Play, Stop, disable and reparent
      need no bookkeeping, and Edit mode never simulates. `dev/Gizmos` draws collider wireframes from
      the same geometry the solver collides. `samples/physics` is the sample.
- [x] **Physics, phase 2** — box–box contacts through a separating axis test with face clipping and an
      edge-edge fallback, Coulomb friction on two tangents, impulses carried across steps by feature id,
      and split-impulse position correction. A stack of 10 crates stands for 600 steps, a crate holds on
      a slope it cannot slide down, and a sliding sphere turns into a rolling one.
- [x] **Physics, phase 3** — `applyImpulse` / `applyForce` / `applyTorque` as accumulators on the node
      proxy, `engine.raycast` returning `{ node, position, normal, distance }`, `touched` and
      `touchEnded` through a `ContactObserver` that `script/SceneObservers` implements, a `planar` lock that
      makes a scene 2D, and `[Physics] gravity` in `Game.ini`. `samples/physics` gained a 2D scene.
- [ ] Physics: sensors that detect without responding, and collision layers to filter pairs
- [ ] Physics: `unitsPerMeter` in `Game.ini`, so a project working in pixels can scale the solver's
      slop and bounce threshold with it
- [x] **Physics, phase 4** — capsules (segment shapes that rest on two points when they lie down), a
      bounding volume tree rebuilt each step in place of the O(n²) pair loop, and sleeping islands that
      hold a settled stack bit-for-bit still until something moves, touches or deletes what holds it up.
      `MeshPass` gained a `capsule` primitive to draw them with.
- [ ] Physics: let a raycast use the broadphase tree — it walks every collider today, because the tree
      belongs to the last step and a script can cast at any time
- [ ] Physics, phase 5 — joints, CCD, collision layers, a character controller, convex hulls
- [ ] Render interpolation — `alpha` reaches `onRender` and nothing uses it, so at `fixedHz` 60 on a
      120 Hz display a body visibly steps. Keeping the previous transform and interpolating in render
      fixes scripted motion too.
- [ ] **Audio** — clip loading, one-shot + looping sources, 3D positional, buses/volume
- [ ] **Animation** — skeletal (skinning shader, joint palette), sprite sheet animation, and a
      clip/state machine
- [ ] **UI system** — there is no screen-space layer: sprites live in the world, so a HUD has to be
      placed in front of the camera. It is *Game UI* under *UI framework*, on the same widget core
      the editor moves onto. World-space text may still want MSDF later.
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

- [ ] **The broadphase tree is rebuilt from scratch every step.** That is O(n log n) where a tree kept
      across steps would be O(moved bodies); fine until a level has thousands of colliders.
- [ ] **Two capsules meet at one point**, so a pair lying side by side can rock against each other.
      Face clipping like the box path would give two.
- [ ] **Contact features are recomputed every step.** Warm starting matches them by id, so a manifold
      that changes shape — a box tipping onto another face — drops its impulses for one step.
- [ ] **Fast bodies tunnel.** There is no CCD, so a small body moving more than its own size in one
      fixed step can pass through a thin collider.
- [ ] **A body's rotation is stored as Euler angles.** The world keeps a quaternion and the conversion
      is exact, but the angles Properties shows flip near ±90° pitch.
- [ ] **Everything is host-visible memory.** `GpuBuffer` always maps; `Mesh` writes vertices straight
      into a mapped buffer. Static geometry wants device-local + staging.
- [ ] **Sprite batching is run-based** — `SpriteBatch::flush` only merges *consecutive* quads sharing
      a texture, so interleaved textures degrade to one draw call per quad. Sort by texture, or go
      bindless.
- [ ] **No sorting anywhere**, in either pass. Submission order only — sprites test depth against
      meshes but never write it, so they layer among themselves by submission order, not by z.
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
- [x] **Enforce the layer graph.** CTest runs `architecture_layers` against first-party includes and
      `architecture_checker` against allowed/forbidden fixtures, including the editor-only ImGui rule.
- [ ] **Scene text numeric parsing still follows the process locale.** `TextLoad` uses `std::stod` /
      `std::stoll`, which also accept numeric prefixes. The refactor shares writer formatting only;
      changing this grammar and its error behavior belongs in a separate correctness change.
- [ ] **Swapchain format changes leave existing graphics pipelines in place.** The renderer rebuilds
      its render passes when the format changes, but the dependent scene, composite and overlay
      pipelines need a separate compatibility fix.
- [ ] **The points-vs-pixels split is implicit.** `ViewCamera` and `screenToWorld` are in
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
