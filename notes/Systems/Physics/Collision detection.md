---
type: concept
system: "[[Physics]]"
source: "src/physics/Collide.cpp, src/physics/Broadphase.cpp"
---

- **Broadphase:** a bounding volume tree rebuilt every step by splitting the widest axis at the median. A pair is tested once, when at least one side is dynamic.
- **Sphere–sphere, sphere–box:** closed form.
- **Capsule:** a segment with a radius. Against a sphere or capsule, closest point on the segment; against a box, closest points found by walking segment and box in turn, with the segment clipped to a face it lies flat on — so a capsule on the ground rests on two points.
- **Box–box:** SAT over 15 axes. A winning face becomes the reference face and the other box's incident face is clipped against its sides (up to four points); a winning edge axis gives one point.
- **Every point carries a feature id**, so the next step recognizes it for warm starting.

**Load-bearing tolerances**, each found watching a stack fall over: `CLIP_SLACK` and `TOUCH_SLACK` keep corners that sit exactly on a plane (float noise would drop one and tip the box); `FACE_BIAS` makes the second box's face win only by a margin, because on an exact tie the choice flapped and threw the warm start away.

Raycasts don't use the tree: they walk every collider, because the tree belongs to the last step.
