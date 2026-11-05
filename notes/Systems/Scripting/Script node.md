---
type: class
system: "[[Scripting]]"
source: "src/script/Script.hpp"
---

A `Script` names a file in `Source/` (`file` prop) and runs it when the node starts.

- `Script::BOOTSTRAP` caches each file's source by path and compiles per instance with `load(source, "@" .. file, "t", env)`, where `env = setmetatable({ script = __node(id) }, { __index = _G })`. Globals a script assigns stay in its own env; `_G.x` shares.
- `__scriptStart(file, node, previous)` compiles first, so a start that fails to compile leaves `previous` running; then it stops `previous` and spawns the chunk as a thread of a fresh owner ([[Task ownership]]).
- `Script` is a `PropSink`: writing `enabled` false stops it, true runs the file again from the top. `Scene::startPending` calls `onStart` on disabled nodes too, so `Script::onStart` checks `isEnabled` itself.
- A script attaches another with `box:add("Script").file = "riser.lua"`.
