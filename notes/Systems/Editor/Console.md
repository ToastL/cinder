---
type: module
system: "[[Editor]]"
source: "src/dev/panels/Console.hpp"
---

Installs the `LogSink` ([[Logging]]) in its constructor and clears it in its destructor. Lines queued by the sink join the panel in `update`, so nothing edits the widget tree during paint.

The input line runs `LuaHost::eval` against the live `lua_State`: it tries `return <text>` first, then the raw text, so `1 + 1` prints `2` and multi-statement chunks run. Results and errors go back through `logInfo`/`logError` with `[console]` as the chunk name.

In Edit mode the state holds only the prelude, so the console can build a scene (`scene:create`, `node:setAttribute`, `node:add("Script")`) without starting any of it, and Save writes the result. Typing doesn't drive the game: the console has focus, not the Scene view.
