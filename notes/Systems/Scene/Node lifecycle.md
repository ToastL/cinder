---
type: concept
system: "[[Scene]]"
source: "src/scene/Scene.cpp"
---

- Every inserted node queues `onStart`, run at the top of the next `Scene::update` — disabled or not. A node inserted during a start starts in the same pass.
- A disabled node skips itself **and its subtree** in `update` and `render`. It doesn't stop scripts under it: a `Script` stops only when its own `enabled` is written.
- `destroy` is deferred to the end of `update`. `destroyNow` tears down at once — for the editor, which never calls `update` in Edit mode. Never call it inside an update walk.
- `Scene::clear` doesn't notify the observer, so loading isn't a stream of destroy events.
- `Scene::render` doesn't wait for `start`: a scene never updated is still fully drawn. That split *is* Edit mode — see [[PlaySession]].
