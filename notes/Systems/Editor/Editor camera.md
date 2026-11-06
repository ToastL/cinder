---
type: class
system: "[[Editor]]"
source: "Source/dev/EditorCamera.hpp"
---

In Edit mode the Scene view looks through `dev/EditorCamera` via `Renderer::overrideCamera`; every other state calls `releaseCamera`, so Play and Pause show the game's camera. Covers sprites too, so a 2D scene is flown like a 3D one.

- Always perspective. **Seeded once**, on the first Edit frame, from the last enabled `Camera` in render order. From a perspective camera that's the game's frame exactly. From an orthographic one it backs away along forward until its fov frames the ortho rectangle at the near plane, pushes far back by the same distance, and flies at half that distance per second. `editor_camera_test` pins both seeds.
- Stop doesn't reset it. No roll, not saved.
- **Controls** come from the widget's events and `Application::keyHeld`, not `Input`: WASD fly, Q/E down/up, Shift faster — whenever the Scene view has focus, no button held. Right button looks (wheel changes speed); left drag off a handle also looks; middle or Alt+left pans; wheel dollies. A press captures the mouse and focuses the view.
- **Camera gizmos:** perspective = wireframe frustum; orthographic = its near rectangle. From `ViewCamera::corners()` at the panel's size, through `dev/GizmoLines`, clipped in clip space and inset by `INSET`, so a camera you're looking straight through draws nothing.
