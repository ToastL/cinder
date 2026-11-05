---
type: concept
system: "[[Renderer]]"
source: "src/gfx/pass/ViewCamera.hpp"
---

**One camera; 2D and 3D differ only in its projection** — Unity's model. The scene's `Camera` writes the renderer's `ViewCamera` every frame through `DrawList::camera` as a `scene::View`.

- **Perspective:** fov at the view's aspect.
- **Orthographic:** covers `virtualSize / zoom` world units (or the view's size in points when `virtualSize` is zero), centred, stretched. Clips at `near`/`far` too, so a camera at `z = 0` can't see a sprite at `z = 0` — `samples/sandbox2d` puts its camera at `z = 10`.
- The world is **Y-up**; both projections flip Y in the matrix, not via a negative viewport.
- Several cameras: the last one rendered wins.
- `overrideCamera(view)` sets a second camera that the frame draws through ([[Editor camera]]); `releaseCamera` drops it. The scene keeps writing the first underneath, so release is exact.
- `engine.screenToWorld` / `screenSize` read the scene's camera, never the override. `screenToWorld` returns where the ray through a point meets `z = 0`.
- `corners()` gives the clip volume's corners, used for camera gizmos.
