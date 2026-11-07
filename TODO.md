# TODO

Roadmap toward a Unity/Unreal-shaped engine: an editor where you select an actor, edit its fields,
hit Play, hit Stop, and land back where you started.

**Where we are:** a renderer with Vulkan and Metal backends driving an ordered pass list into an offscreen target,
composited to the swapchain by a fullscreen triangle. Sprite batch with atlas support, mesh pipeline
with one directional light, one camera that every pass draws through, edge-triggered input, and game
code in C++: each project's `Source/` defines actor and component classes, built by the editor into a
library it hot-reloads between frames. Unreal's object model — actors owning components, with
transforms in a scene-component hierarchy —
with a prop system that feeds the serializer and the editor from one declaration. A
bidirectional `Archive` with text and INI backends, and games that are project folders in Unreal's
shape — a `.cinder` file, `Config/`, `Content/` and `Source/` — rather than code in the engine. The
editor runs on a UI framework of our own, in a separate `Dev` module: docked tabs holding the
Outliner, the Scene view, Details and a console, over a toolbar with the File,
Edit, Window, Play and Platforms menus and a Build button — `player` ships without a byte of it. The editor opens a project in Edit mode, seen
through a free-flying editor camera — 2D scenes included — with a gizmo drawn for every scene camera; Play serializes the
scene and rebuilds every actor from it, and Stop restores it. An Outliner shows the actor tree, and Details edits the selected actor's fields, components and attributes with no per-type editor
code. A click in the Scene view selects, gizmos move, rotate and scale the selection, every edit is
undoable, and an unsaved scene is marked and guarded on close. A physics engine of our own has
started: bodies and colliders as components, spheres and boxes, friction, and a sequential-impulse solver
with warm starting that stacks crates and never runs in Edit mode; boxes, spheres and capsules meet
through a bounding volume tree, settled islands fall asleep, and game code pushes bodies around, casts rays
and hears about contacts, with a `planar` body making a 2D scene physical. 385 headless test cases, a
hot-reload test that compiles real C++, and a C++ selftest project.

Foundations, the scene model, game code in C++, serialization and the Edit/Play split are
done. The editor shell is underway — the UI framework, docking, the console, the toolbar, the Scene
view, the editor camera, the Outliner, Details, picking, undo and transform gizmos have landed.

---

## Editor shell

- [x] The editor's UI is our own — see *UI framework*. `editor` is the dev build on the `Dev` module;
      `player` never links it, and `cbt test`'s `shipping` check runs `nm` over it.
- [x] Docking layout: the Scene view, Outliner, Details and Console as tabs in splits, with
      drag-to-redock and a Window menu; the asset browser docks into it when it lands.
- [x] **Scene view** — `Dev/Panels/SceneView`. The target is sized to the panel through
      `Renderer::setViewportSize` and drawn as an opaque image element, and `engine.mousePosition()`
      is relative to the panel's top-left.
- [x] **Outliner** — `Dev/Panels/Outliner`, a virtualized tree over actor ids: a filter, "+" to spawn any registered actor class, right-click to add, duplicate or
      delete, and drag to attach. `Dev/Selection` holds an actor or component id, so the selection
      survives Play and Stop.
- [ ] Outliner: multi-select, inline rename, copy and paste
- [ ] Outliner: folders as an editor label on actors, the way Unreal's are — the old `Folder` node went
      with the node tree
- [x] **Details** — `Dev/Panels/Details` shows one actor: its props, its components with Add Component
      and remove, and the selected component's transform and props with a widget per `PropType` and
      `PropHint` (spin boxes, vector boxes, enum combos, colour pickers), and its attributes with add
      and remove. An edit during Play fires the game's changed signals.
- [x] Details: an actor-reference widget — a combo of the objects a `Ref` accepts, a Pick eyedropper
      that takes the next Scene view or Outliner click, and Go to select the target
- [ ] Details: a filter in the reference menu, and an id or path beside duplicate names
- [ ] Details: an asset-reference widget for texture and mesh names
- [ ] Details: rename, reorder and reattach components by dragging, and set the root
- [x] **Gizmos** — `Dev/Manipulator` moves along world or local axes and planes, rotates on gimbal
      rings that each turn one Euler angle, and scales along local axes or uniformly; ⌘ snaps and
      Esc cancels. 1/2/3 pick the tool and X toggles World/Local. `Dev/GizmoLines` builds the strokes
      from the same shapes the hit test uses, and every scene `CameraComponent` as a frustum.
- [ ] Gizmos: world- and local-space rotation rings and a view-facing ring — rotation is gimbal-only
- [ ] Gizmos: snapping to absolute grid positions, and snap steps set in the editor
- [ ] Gizmos during Play — like picking, they only exist in Edit mode
- [ ] Gizmos for a multi-selection, about the pivot or the selection's centre — waits for Outliner
      multi-select
- [x] Mouse picking — `Dev/Picking`, a CPU raycast against each `StaticMeshComponent`'s cube, each
      `SpriteComponent`'s quad and each `CameraComponent`; the hit's actor is selected, the Outliner
      reveals it and `Dev/GizmoLines` outlines it
- [ ] Picking against real mesh bounds — `MeshPass` now has a `sphere` primitive that `Picking` still
      treats as a cube, and imported meshes will need their own bounds or triangles
- [x] Editor camera, independent of the game camera — `Dev/EditorCamera`, seeded from the scene's
      camera and handed to `Renderer::overrideCamera` in Edit mode, which every pass draws through, so
      a 2D scene is flown like a 3D one. Right-drag to look and fly with WASD/QE, middle-drag to pan,
      scroll to dolly.
- [ ] Scene view 2D mode — Unity's toggle to an orthographic editor camera looking down -Z, panned and
      zoomed rather than flown
- [x] **Undo/redo** — `Dev/History` snapshots the scene text through `SceneCodec` after each finished
      edit, so every edit is covered with no per-edit code; ⌘Z / ⌘⇧Z and toolbar buttons
- [ ] Undo: a snapshot costs a full scene save per edit — fine at sample scale; move to diffed snapshots
      or a command stack if scenes grow large
- [x] Console panel — every print routes through `Platform/Log`, and `Dev/Panels/Console` installs the
      sink. It is a log view.
- [ ] Console commands registered by game code — Unreal's `exec` functions and console variables
- [x] Toolbar — Play / Pause / Step / Stop and Save, on ⌘P / ⌘⇧P / ⌘⌥P / ⌘S, in the main menu bar
- [ ] Console: click a `file:line` to open it in `$EDITOR`; filter by level; search
- [ ] Editor layout + window state persisted between runs — `TabManager` holds the layout in memory
      only; the file belongs in the project's `Saved/`
- [ ] Project picker and new-project template — the editor takes the project folder or its
      `.cinder` file on the command line; the template writes `Content/Scenes/` and `Content/Textures/`,
      and the picker lists each descriptor's `category` and `description`. The *Project browser*
      under *UI framework*.
- [ ] Project Settings panel — `ProjectConfig::walk` already writes `Config/Game.ini` through `IniSave`
- [ ] Scene switching — open, new and save-as. The editor edits the one scene named by
      `startScene` in `Config/Game.ini` or `--scene`.

**Done when:** adding `CINDER_PROP(bounciness_)` to any actor or component class makes it appear in Details, save
to disk, and become undoable — with zero editor code.

**Code editing is external, by decision.** Game code is C++ on disk, edited in VS Code, Xcode or
anything with clangd — the editor writes `compile_commands.json` for it. The editor's job is the round
trip: build, hot-reload, preserve the scene, and report compiler errors with a real `file:line` — not
a text buffer that competes with a real IDE and loses.

## Game code

- [x] **C++ game code** (2026-11-06) — game code is actor and component classes in the project's
      `Source/`, registered with `CINDER_REGISTER`, so props save, show in Details and undo with no
      extra code
- [x] **Actor/Component object model** (2026-11-07) — Unreal's shape replaced the Roblox/Godot node
      tree: `Scene/Actor`, `Component` and `SceneComponent`, default components made in a constructor
      and written against the class's own default, one id space, scene file version 5, and the
      Explorer and Properties became the Outliner and Details
- [x] Unknown classes load as `ActorPlaceholder` and `ComponentPlaceholder` and save back byte for
      byte, so a failed build never loses an actor or a component
- [x] Actor and component event hooks, `Scene::connect` and `Scene::after`/`every` — owned by the scene, so a library
      can always be unloaded
- [x] `Core/GameModule` loads, BuildId-checks, registers and unloads a library; `player` loads one
      before opening its scene
- [x] Game code builds through `cbt game`: parallel, incremental through depfiles and command
      signatures, `compile_commands.json`; Build and ⌘B in the editor, the build before the scene opens
- [x] Hot reload between frames in Edit mode, through a scene round trip; a failed build or load keeps
      the old library
- [ ] Hot reload during Play — today it waits for Stop
- [ ] Rebuild automatically when a file in `Source/` is saved, as Unity does
- [ ] Coroutines for waits — `co_await Game::seconds(0.5)` in place of `Scene::after` chains
- [x] A library built against an older engine is rebuilt: the engine's BuildId is part of every game
      compile command, and `GameModule` refuses a library carrying another one
- [ ] Compiler errors in the console as clickable `file:line`
- [ ] A precompiled header for `Game/Game.hpp`, which dominates compile time
- [ ] iOS forbids loading code that is not signed into the app: link the game statically there

## Build tool

`cbt`, in `Source/Programs/CinderBuildTool/`, builds the engine, the tools and game code — Unreal's
UnrealBuildTool model.

- [x] Rules as code: a `.Build.cpp` per module and a `.Target.cpp` per target, compiled into `cbt` by
      `Setup.sh`; `cbt` rebuilds itself when they change
- [x] Vendored third-party code in `Source/ThirdParty/`, built from its own rules
- [x] Parallel, incremental actions: depfiles, mtimes and command signatures; shaders; Debug,
      Development and Shipping; `mtl` and `vk`
- [x] The layer graph derived from the rules — `cbt check`
- [x] `cbt test` — layers, fixtures, shaders, the shipping check, unit tests and hot reload
- [x] `cbt game` — project C++ with optional project rules; the BuildId keeps it in step with the engine
- [x] `cbt generate xcode` and `cbt generate compile-commands`; Xcode only calls `cbt`
- [x] The repo root in a project's shape: `Source/`, `Content/`, `Shaders/`, `Samples/`, `Tests/`
- [ ] Remember `--backend` and `--shader-formats` between runs (`Intermediate/BuildSettings.ini`); an
      Xcode build always uses the backend the project was generated with
- [ ] Linux: the rules carry the X11 and `-rdynamic` cases, but nothing has been built there
- [ ] Running from Xcode is untested beyond `xcodebuild`: a scheme's Run is fixed to the Debug binary
- [ ] Project-level `cbt generate xcode --project <dir>` for a game's own Xcode project

## UI framework

**The editor's UI is our own, by decision, and it is shaped like Unreal's without borrowing its
names** — not Qt, not RmlUi, and not an immediate-mode debug UI. Unreal builds its editor on Slate and
its game UI on UMG, which wraps Slate; this engine does the same, so one framework is both the game UI
system and the editor's toolkit. Qt would be a second UI system, an installed dependency with LGPL
terms, and it and GLFW both want to own the macOS app; RmlUi would still leave every editor widget,
docking and multiple windows to build.

The shape is Slate's: a retained tree of widgets with plain names — `Button`, `Label`, `TextBox`,
`HorizontalBox`, `Splitter`, `Canvas` — built with `UI::make<T>()`, slots and chained arguments; layout
in two passes, desired size bottom-up and arrangement top-down; paint into draw elements that a
GPU-free batcher turns into vertices; input routed along the widget path with a `Reply`, focus and
mouse capture; a `Theme` of named styles; docking; menus and shortcuts from a `CommandList`. `UI` never
includes `Scene`, `Reflect` or `Gfx` — the reflection-driven Details panel lives in `Dev`.

- [x] **Text** — `Text/` (-> `Platform`) on FreeType and HarfBuzz: load a font, shape and measure a
      string, rasterize glyphs into CPU atlas pages, map carets to clusters. No GPU, so layout measures
      text without an edge to `Gfx`, and all of it tests headlessly. Roboto and Roboto Mono ship in
      `Content/Fonts/`.
- [x] **UI renderer** — `Gfx/UiRenderer` records in the present pass after the composite, not as a
      `DrawPass`: every `DrawPass` records in the scene pass, which has depth. Rounded rectangles and
      borders as an SDF, anti-aliased lines and convex fills for gizmos, images including the scene
      target drawn opaque, and glyphs from R8 atlas pages rasterized at the framebuffer scale.
      `ui_gallery` shows all of it and `--capture-window` captures it.
- [x] **Widget core** — `UI/Core` (widgets, geometry, attributes, replies, draw elements, hit
      testing, styles), `UI/Framework` (the `Application` that routes events, commands and shortcuts)
      and the first widgets: boxes, border, overlay, canvas, scaler, label, image, button, check box,
      scroll box, splitter, text field and box, viewport. `Input` records an ordered event queue with
      characters, repeats and modifiers, and `Platform/InputScript` replays one for tests and captures.
- [x] **Vertical slice** — the toolbar, the Scene view and the console on the new UI
      in a fixed layout: picking, gizmo drags, the editor camera, undo, Play/Stop, the console
      and the close prompt all run on it, and `--input-script` drives every one of them.
- [x] **Value editors and menus** — popups and tooltips in the `Application`; `SpinBox` (drag, click
      to type an expression, bounds only when declared, never rounded to its display),
      `VectorInputBox`, `ComboBox`, `ExpandableArea`, `ColorBlock` and `ColorPicker`; `Menu`,
      `MenuBuilder`, `MenuAnchor`, `MenuBar` and context menus, with commands, checks and submenus.
      The editor's toolbar has File, Edit, Window, Play and Platforms menus.
- [x] **Lists and trees** — `TreeView` (a `ListView` is one with no children) builds only the rows in
      view, keyed by item id, with selection by mouse and arrow keys, expansion from the arrow or a
      double click, `reveal` through collapsed ancestors, context menus, and drag and drop carrying a
      `DragDropOperation` with a decorator. Tab walks the fields in paint order and stays inside a popup.
- [ ] **Font fallback** — shaping falls back per glyph to a symbol font, so shortcuts can read ⌘⇧P
      instead of `Shift+Cmd+P`. Roboto has no ⌘, and a label that asks for one draws a missing glyph.
- [x] **Docking** — `UI/Docking`: a `TabManager` holds the layout as a tree of splits and stacks,
      spawns a tab's panel once and keeps it across re-docks, and rebuilds the widgets on every
      change, carrying the splitter sizes over. A tab drags to another stack's middle or against an
      edge, which splits it; closing the last tab of a stack collapses it; the Window menu reopens a
      closed tab where it last lived. The editor runs on it.
- [ ] **Floating panels** — panels in their own OS windows, after parity: `RHI::Ctx` has to stop owning
      the one GLFW surface — one device, a swapchain per window.
- [x] **Port the panels** — `Dev/Panels/` holds `Toolbar`, `SceneView`, `Outliner`, `Details`,
      `Console` and the docked `Layout`, and `editor` is now the new UI. `PlaySession`, `History`,
      `Selection`, `Picking`, `EditorCamera`, `Manipulator`, `GizmoGeometry` and `GizmoLines`
      carried over unchanged, and
      `graphics_smoke` drives the new editor with injected input.
- [x] **Retire the old UI** — the previous immediate-mode panels, their second executable, `Probe`,
      `Gfx::Overlay` with its `OverlayTexture` and the third-party dependency behind them are all
      gone; `Renderer`'s only UI seam is `setUiPaint`.
- [ ] **Game UI** — reflected widget classes saved as `.widget` assets under `Content/`, created from
      game code and added to the viewport, never actors in the `Scene`: Unreal's UMG, in its own layer over
      `UI`, `Reflect` and `Serial`, and its own plan.
- [ ] **Project browser** — the first screen of `editor` with no argument: recent projects and
      templates, as Unreal's Project Browser and Godot's Project Manager do. Built on the new UI once
      it lands, and designed separately.

## Play mode

- [x] Play / Pause / Step / Stop — `Dev/PlaySession`, driven by `Dev/Panels/Toolbar`
- [x] Serialize scene on Play, restore on Stop — both are `Engine::loadScene` on one snapshot, so
      every actor is rebuilt from text and every connection and timer starts empty
- [x] Editor keeps rendering while paused — `GameLoop::idle`
- [x] Game code never runs in Edit mode — there is no entry point, and hooks, connections and timers
      reach only objects `Scene::update` has started, which Edit mode never calls. `onRender` still draws.
- [x] Input routing — `Dev/Panels/SceneView` gives the game the keyboard while the Scene view is focused
      and the mouse while its image is hovered; a locked cursor keeps both. Play focuses the Scene.
- [x] Dirty tracking — the toolbar shows `main.scene*`, and closing the window with unsaved changes
      asks Save / Don't Save / Cancel
- [ ] Keep a change made during Play — Unity's "copy component values" escape hatch

## Serialization — what is left

- [x] Actor references — `Scene::Ref<T>`, a prop holding an actor or component id that `get` resolves
      on every call; saved only while it resolves, fixed up after a load, remapped by `clone`
- [ ] Attributes that name an actor — an attribute holds numbers, strings, bools and small vectors
- [ ] Prefabs — a saved actor that can be instanced, with per-instance overrides: Unreal's Blueprint
      class without the graph. `Scene::clone` already copies an actor, its components and its attached
      actors generically, and the archetype delta is the override model.
- [ ] Tags — one class driving every actor carrying a tag, like Unreal's actor tags
- [ ] Attach to a named component or socket of the parent actor — attachment is always to the parent's
      root today

## Asset pipeline

- [ ] GUIDs — a `.meta` sidecar per asset, stable across renames and moves. Scenes currently store
      the path relative to `Content/` or `Source/`, which a rename breaks.
- [ ] Asset database: GUID -> path -> loaded handle. `DrawList::textureHandle` / `meshHandle` are
      already the lookup seam; they just need GUID lookup in front.
- [ ] Import pipeline: source file -> cooked asset, cached, re-run on mtime change
- [ ] Reference counting + **unload** — `Assets` never frees a texture until shutdown, and
      `MAX_TEXTURES = 256` is a hard cap with a fixed descriptor pool
- [ ] Hot reload for textures and meshes (game code already reloads)
- [ ] Shader hot reload. Shaders are compiled at build time, so editing one needs a rebuild. A
      runtime HLSL path behind a debug flag would restore the edit-and-restart loop — and it would
      have to compile to whichever format the live backend loads, not just SPIR-V.
- [ ] Mesh import
- [ ] Asset browser panel with thumbnails

## Gameplay systems

- [x] **Physics, phase 1** — our own engine in `Physics/`, not Jolt or Box2D: bodies and colliders —
      `RigidBodyComponent` and `ColliderComponent` since the actor model — sphere and box shapes, compound mass and inertia about the centre of mass, a brute-force
      broadphase, exact sphere–sphere and sphere–box contacts, and sequential impulses with Baumgarte
      correction and restitution. `World::step` walks the scene, so Play, Stop, disable and reparent
      need no bookkeeping, and Edit mode never simulates. `Dev/GizmoLines` draws collider wireframes from
      the same geometry the solver collides. `Samples/physics` is the sample.
- [x] **Physics, phase 2** — box–box contacts through a separating axis test with face clipping and an
      edge-edge fallback, Coulomb friction on two tangents, impulses carried across steps by feature id,
      and split-impulse position correction. A stack of 10 crates stands for 600 steps, a crate holds on
      a slope it cannot slide down, and a sliding sphere turns into a rolling one.
- [x] **Physics, phase 3** — `applyImpulse` / `applyForce` / `applyTorque` as accumulators on
      the body, `Game::raycast` returning a `RayHit`, `Touched` and `TouchEnded` emitted on both actors
      and both colliders
      through `Scene::emit`, a `planar` lock that
      makes a scene 2D, and `[Physics] gravity` in `Game.ini`. `Samples/physics` gained a 2D scene.
- [ ] Physics: sensors that detect without responding, and collision layers to filter pairs
- [ ] Physics: `unitsPerMeter` in `Game.ini`, so a project working in pixels can scale the solver's
      slop and bounce threshold with it
- [x] **Physics, phase 4** — capsules (segment shapes that rest on two points when they lie down), a
      bounding volume tree rebuilt each step in place of the O(n²) pair loop, and sleeping islands that
      hold a settled stack bit-for-bit still until something moves, touches or deletes what holds it up.
      `MeshPass` gained a `capsule` primitive to draw them with.
- [ ] Physics: let a raycast use the broadphase tree — it walks every collider today, because the tree
      belongs to the last step and game code can cast at any time
- [ ] Physics, phase 5 — joints, CCD, collision layers, a character controller, convex hulls
- [ ] Render interpolation — `alpha` reaches `onRender` and nothing uses it, so at `fixedHz` 60 on a
      120 Hz display a body visibly steps. Keeping the previous transform and interpolating in render
      fixes game-driven motion too.
- [ ] **Audio** — clip loading, one-shot + looping sources, 3D positional, buses/volume
- [ ] **Animation** — skeletal (skinning shader, joint palette), sprite sheet animation, and a
      clip/state machine
- [ ] **UI system** — there is no screen-space layer: sprites live in the world, so a HUD has to be
      placed in front of the camera. It is *Game UI* under *UI framework*, on the same widget core
      the editor moves onto. World-space text may still want MSDF later.
- [ ] **Particles** — emitters as components, GPU-driven if it matters
- [ ] Scene queries — raycast, overlap, spatial partition (BVH or grid)
- [ ] Timers, event bus

## Ship a game

- [x] Packaging in C++ — `Dev/Staging` and the `packager` executable; the packager has `cbt` build
      `player` in the package's configuration and copies it
- [x] Development and Shipping packages — Shipping strips `player` and the game library and keeps their
      symbols in `Saved/Symbols/`
- [x] "Build Game" in the editor — the **Platforms** menu lists macOS, Linux, Windows, iOS and
      Android; the host desktop packages the open project into `Saved/Builds/<Platform>/`, the rest
      are listed as unavailable until their hosts exist
- [ ] Asset bundling into an archive, not loose files
- [ ] Settings/save-data location per OS
- [ ] Crash handler + log file

---

## Rendering (ongoing, not blocking)

- [ ] Multiple lights + a real material model (currently one hardcoded directional light in
      `mesh.hlsl`, and no material struct at all)
- [ ] Shadow maps
- [ ] Sort transparent draws back-to-front; sort opaque by pipeline/material
- [ ] Frustum culling
- [ ] Mipmaps — `vk/VkImages.cpp` creates every image with `mipLevels = 1` and samplers with
      `MIPMAP_MODE_NEAREST`; Metal textures are `mipmapped:NO`
- [ ] Anisotropic filtering — `anisotropyEnable = VK_FALSE` in each `TexturePool`'s sampler
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
      is exact, but the angles Details shows flip near ±90° pitch.
- [ ] **Everything is host-visible memory.** `GpuBuffer` always maps; `Mesh` writes vertices straight
      into a mapped buffer. Static geometry wants device-local + staging.
- [ ] **Sprite batching is run-based** — `SpriteBatch::flush` only merges *consecutive* quads sharing
      a texture, so interleaved textures degrade to one draw call per quad. Sort by texture, or go
      bindless.
- [ ] **No sorting anywhere**, in either pass. Submission order only — sprites test depth against
      meshes but never write it, so they layer among themselves by submission order, not by z.
- [ ] **`endSingleTime` does a full `vkQueueWaitIdle`** per texture upload.
- [ ] **Half the project descriptor is read by nothing.** `modules`, `plugins`,
      `additionalRootDirectories`, `additionalPluginDirectories` and `disableEnginePluginsByDefault`
      are parsed, checked and saved; they wait for a plugin system.
- [ ] **A reference inside a placeholder is raw text.** An unknown-class object keeps its fields
      verbatim, so a `Ref` stored in one is neither checked on save nor remapped by a load or a clone.
- [ ] **A disabled actor's timers still fire.** `enabled` skips an actor's tick and render, but
      `Scene::after`/`every` only check that the owner exists.
- [ ] **Runtime meshes cannot be named in a scene** — `StaticMeshComponent.mesh` names a primitive, and
      `engine.newCube` hands back an anonymous handle. Mesh import is where named meshes come from.
- [ ] Fixed caps with no growth path: `MAX_QUADS = 10000`, `MAX_DRAWS = 4096`, `MAX_TEXTURES = 256`
- [ ] **`Renderer::capture` stalls the device** and reads the target back synchronously. It is a
      debug tool; do not call it per frame. `RenderTarget` carries `TRANSFER_SRC_BIT` only for it.
- [ ] **Resizing the Scene panel stalls the device.** `Renderer::setViewportSize` calls
      `vkDeviceWaitIdle` and rebuilds both targets on every size change, so dragging a dock splitter
      stalls once per frame. Rebuilding each target when its own fence comes round would avoid it,
      but the descriptor set already recorded for that frame would have to survive the rebuild.
- [x] **Enforce the layer graph.** `cbt test` derives it from the module rules and runs `layers`
      against first-party includes and `layer_fixtures` against allowed and forbidden include fixtures.
- [x] **Scene text numeric parsing is locale-independent.** `TextLoad` parses with `from_chars`
      through `Serial/NumberText`, rejects numeric prefixes, and logs a malformed value.
- [ ] **Swapchain format changes leave the scene and composite pipelines in place on Vulkan.** The
      renderer rebuilds its render passes and the UI pipeline when the format changes; the scene and
      composite pipelines still need the same treatment. Metal's format is fixed.
- [ ] **The points-vs-pixels split is implicit.** `ViewCamera` and `screenToWorld` are in
      window points; the swapchain and render target are in framebuffer pixels. 2x apart on Retina.
      `FollowMouse` and `Spawner` in `Samples/sandbox2d` depend on the current behaviour.
- [ ] **The packaged player finds itself through `argv[0]`**, so launching it through a `PATH`
      lookup rather than by path misses `Engine/` and `Project/` next to it
- [ ] No CI
- [ ] **The two presenters duplicate their bookkeeping** — the recreate callback, window-capture
      request, `pixelsPerPoint` and the shared fields of `Frame` are written twice; a neutral
      `RHI::PresenterState` would hold them once.
- [ ] **`UiRenderer` notices a new glyph atlas by its address**, so an atlas allocated where a freed
      one lived would skip the full re-upload.

## Backends and platforms

DXC is the single HLSL frontend and already emits SPIR-V, DXIL and MSL/`.metallib` — see CLAUDE.md's
*Commands* section. Only the Vulkan backend consumes its output; these are the phases that would
change that. Each leaves the engine running on Vulkan after every step.

- [x] **The backend seam.** *(done)* A neutral leaf `Source/Gfx/RHI/` (formats, `Commands`/`Uploads`
      handles, `TextureBinding`, a neutral pipeline builder) that `Gfx/VK` and `Gfx/MTL` each
      *define*, selected by `cbt`'s `--backend` — compile-time, no vtables, so `Engine` keeps holding
      its context by value. Nothing outside a backend directory names a Vulkan or Metal type.
      - [x] Delete `VkDescriptorSetLayout` from eight constructors — there is exactly one layout in
            the engine and `setLayoutCount` is hardcoded to 1, so it is a constant pretending to be
            a parameter. This removes more Vulkan from headers than the new types add.
      - [x] `VkRenderPass` becomes a two-valued `PassKind` enum; Metal has no render pass objects.
      - [x] Extract a per-backend `Presenter` (swapchain, `FrameSync`, acquire/submit/present,
            window capture). Riskiest step: the `OUT_OF_DATE` path paints without recording, and
            `pixelsPerPoint` is measured only on swapchain rebuild.
- [x] **`Source/Gfx/MTL`** — a native Metal backend on that seam. *(done; pixel-identical to Vulkan on the gallery and both sandboxes)* Objective-C++, so `OBJCXX` joins the
      project languages. `Platform::Window` needs no change (`GLFW_NO_API` already); only
      `Glfw::acquire`'s volk bootstrap gets guarded.
- [ ] **Android.** The shader work is nearly free (same SPIR-V; the GLES fallback would be
      SPIRV-Cross to GLSL ES 3.0, *not* FXC, which is a Windows-only D3D compiler). The platform
      work is the gap: GLFW has no Android backend, and `Platform/Assets`' path model breaks
      outright because APK assets are read through `AAssetManager` and have no path.

## Explicitly out of scope

Naming these keeps them from creeping in:

- Feature parity with Roblox/Unity/Unreal. Unreal's Outliner, Details and Play loop over actors and
  components is the shape being copied — the *workflow*, not the feature list.
- Console platforms
- Networking / multiplayer
- Visual scripting
- A scripting language beside C++
- Ray tracing
