---
type: module
system: "[[Editor]]"
source: "Source/dev/Manipulator.hpp, Source/dev/GizmoGeometry.hpp"
---

Move, Rotate, Scale on a spatial selection, Edit mode only. Tools on **1/2/3**, World/Local on **X** (not W/E/R — [[Decision - Fly keys always on]]).

- `gizmoFor` places the gizmo at the node's world position, `GIZMO_POINTS` long on screen. None behind the camera or under a non-invertible parent.
- `shapes` is **the one handle geometry**: drawn by `drawManipulator`, hit by `hitHandle`. Axes within ~15° of the view ray and edge-on planes are dropped (hides Z in 2D). Ties: centre, planes, axes.
- `Manipulation` holds the node **id** and the press-time transform and recomputes from that each frame, so snapping is exact and cancel writes the start values.

**Move:** arrows, plane squares, a camera-facing centre square. Travel is world-space, taken into the parent's space as a direction (a `Folder` parent is identity).
**Rotate:** rings are the **Euler axes** (Blender's Gimbal) — `Transform::local()` is T·Ry·Rx·Rz·S, so each ring changes exactly one component and nothing is decomposed. Rings aren't perpendicular once pitched. Only a ring's front half is grabbable unless it mostly faces the camera. A mirrored parent negates the angle.
**Scale:** always local (world-axis scale on a rotated node needs shear).

**Ctrl/⌘** snaps the change: `MOVE_SNAP`, `ROTATE_SNAP_DEGREES`, `SCALE_SNAP`. **Esc** cancels. Writes go through `PropDef` and `History::touch`, so a drag is one undo step ("Move Box"). While the mouse hasn't moved, a drag writes the start values exactly — arm64 fuses multiply-adds, and an ulp would record a step.
