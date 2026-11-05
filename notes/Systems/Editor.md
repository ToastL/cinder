---
type: system
status: in-progress
layer: "dev"
source: "src/dev/, src/editor/"
started: 2026-09-11
last-update: 2026-09-26
---

## About

**What it is:** the dev build. `editor` links `engine_dev` ([[Decision - Dev tools as a link target]]): one window of docked tabs — Explorer left, Scene view middle with the console under it, Properties right — over a toolbar with File, Edit, Window, Play and Platforms menus. Opens a project in **Edit mode**: the scene is drawn, no game code runs.

**Run:**
```sh
DYLD_LIBRARY_PATH=/opt/homebrew/lib ./build/editor samples/sandbox2d
```
Flags: `--scene <path>` (relative to `Content/`), `--play`, `--frames <n>`, `--capture <png>` (scene target only), `--capture-window <png>` (UI included), `--input-script <file>`.

**Shortcuts:** ⌘P Play/Stop, ⌘⇧P Pause, ⌘⌥P Step, ⌘S save, ⌘Z / ⌘⇧Z undo/redo. In the Scene view: 1/2/3 move/rotate/scale, X World/Local, WASD/QE fly ([[Decision - Fly keys always on]]).

**The seam:** `Renderer::setUiPaint`. `editor/main.cpp`'s hook drains `Input`'s queue into `Application::processEvents`, runs each panel's `update`, paints, then `SceneView::afterPaint` (camera override, input suppression). `player` never installs one.

## Code
**State:** [[PlaySession]], [[Selection]], [[Undo history]]
**Panels (`dev/panels/`):** [[Scene view]], [[Explorer]], [[Properties panel]], [[Console]], [[Toolbar]]
**Scene view tools:** [[Editor camera]], [[Picking]], [[Transform gizmos]]
**Packaging:** [[Editor packager]]

## Tasks
- [ ] Explorer multi-select, inline rename, copy/paste
- [ ] Asset- and node-reference widgets in Properties
- [ ] Gizmos: world/local rotate rings, grid snap, during Play, multi-selection
- [ ] Picking against real mesh bounds (spheres pick as cubes)
- [ ] Scene view 2D mode
- [ ] Undo with diffed snapshots
- [ ] Console: `file:line` links, level filter, search
- [ ] Layout persisted to `Saved/`; Project Settings panel; scene open/new/save-as
- [ ] Keep a change made during Play

## Notes
- macOS is getting a SwiftUI editor; this one stays the Linux editor and the automated test surface. See [[macOS editor]].
