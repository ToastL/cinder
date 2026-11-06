---
type: concept
system: "[[Scene]]"
source: "Source/scene/Scene.hpp, Source/scene/Scene.cpp"
---

How the tree announces changes and how nodes schedule work. Everything here belongs to the `Scene`, never to a game library — which is what makes [[Hot reload]] safe.

**Signals** — `AttributeChanged`, `ChildAdded`, `ChildRemoved`, `Touched`, `TouchEnded`, `Destroying`. `SignalArgs` carries `other` (the child or the touching node) and `name` (the attribute).
- `Scene::emit(source, signal, args)` calls the source's own hook (`onAttributeChanged`, `onChildAdded`, …; `Destroying` has no hook), then every handler registered with `connect(source, signal, owner, handler)`.
- Only **started**, not-destroyed nodes react — the source for its hook, the owner for a connection. So nothing reacts in Edit mode, where `Scene::update` never runs.
- `connect` returns a handle for `disconnect`. A connection is keyed by both node ids and dies with either.
- Handlers may connect, disconnect or destroy while being called: `emit` snapshots the handles first and re-finds each one.
- Who emits: `Node::setAttribute` (only on a real change, `2 == 2.0`; `loadAttributes` never notifies); `ChildAdded`/`ChildRemoved` on the parent when a node is inserted, reparented or removed; `Destroying` on teardown (not from `Scene::clear`); and [[Physics]] contacts (the `Body`, or the `Collider` when there is none).

**Timers** — `after(owner, seconds, fn)` once, `every(owner, seconds, fn)` repeating; both return a handle for `cancel`.
- Run on scene time (`Scene::time()`) at the top of `Scene::update`, after pending starts and before the tree updates — see [[Node lifecycle]].
- A disabled owner's timers still fire. A destroyed owner's are dropped.
- A late `every` fires once per update and reschedules to `max(due + interval, now)` — it never bursts to catch up. A negative interval counts as 0.

**Lifetime** — destroying a node drops every connection it is source or owner of, and every timer it owns. `Scene::clear` (Play, Stop, undo, hot reload) drops them all, so a reloaded scene starts with none.
