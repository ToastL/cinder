---
type: concept
system: "[[Scene]]"
source: "src/scene/Scene.hpp, src/script/SceneObservers.cpp"
---

How the tree announces `attributeChanged`, `childAdded`, `childRemoved` and `destroying` without naming Lua. An interface in `scene`; its only implementation is in `script/SceneObservers`, installed by `LuaHost::boot`.

The observer calls the prelude's `__attributeChanged`, `__childAdded`, `__childRemoved`, `__destroying`. That's how an edit in Properties during Play reaches the game's signals. `__destroying` also drops the node's cached signals; `LuaHost::close` removes the observer before the state closes.

`physics::ContactObserver` follows the same pattern for contacts — see [[Physics Lua API]].
