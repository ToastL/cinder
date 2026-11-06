---
type: class
system: "[[Scene]]"
source: "Source/scene/Node.hpp, Source/scene/Spatial.hpp, Source/scene/Scene.hpp"
---

- **`Node`** — id, name, parent, children, `enabled`, attributes, the lifecycle hooks `onStart`, `onUpdate`, `onRender`, `onDestroy`, and the event hooks `onAttributeChanged`, `onChildAdded`, `onChildRemoved`, `onTouched`, `onTouchEnded` ([[Signals and timers]]). `CINDER_PROPS(Node, void)` declares `enabled`.
- **`Spatial`** — a `Node` that owns a `Transform`; `Node::transform()` is null on others. A transform composes with its parent only when the parent is spatial, so a `Folder` breaks the chain.
- **`NodeTypes`** — the class registry: name, factory, prop list and a cached default instance per class (for delta encoding). An instance owned by `Engine`. Re-binding a name keeps its slot, so iteration order (and the insert menu) is stable across reboots, and drops the cached default.
- **`Scene`** — owns every node by id. `insert(node, id?)` (the codec's path), `create(className)`, `clone` (props, transform, attributes, fresh ids). A node without a name takes its class name.

`Transform::local()` is T·Ry·Rx·Rz·S. See [[Node lifecycle]], [[Node classes]].
