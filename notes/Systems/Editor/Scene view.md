---
type: module
system: "[[Editor]]"
source: "Source/dev/panels/SceneView.hpp"
---

The "Scene" tab: a `ui::Viewport` whose client it drives (through a small forwarding client, to break the ownership cycle).

- Arrange hands its size to `Renderer::setViewportSize` and its top-left to `Input::setViewportOrigin`; paint draws `TextureRef::viewport()` (the frame's scene target, opaque) with gizmo strokes over it. Order is load-bearing: the size change may rebuild the targets, so the image is drawn after.
- `engine.mousePosition()` and `screenToWorld` work in the panel's points, top-left at zero.
- **Input routing** (`Input::setSuppressed`, in `afterPaint`): the game gets the keyboard while the Scene view has focus and the mouse while it's hovered or holds the capture. A locked cursor gives the game both and calls `Application::setMouseEnabled(false)`.
- Edit mode: left click picks ([[Picking]]), handles drag ([[Transform gizmos]]), the view is the [[Editor camera]]. Every enabled `Camera` is drawn as a gizmo; `selectionStrokes` outlines the selection in orange.
