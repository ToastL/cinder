---
type: concept
system: "[[Scripting]]"
source: "src/lua/LuaCalls.hpp"
---

Lua is built as C, so `luaL_error` longjmps past C++ destructors.

- A binding finishes its C++ work before it raises: `setAttribute` validates in a helper returning a status, and errors only after the helper's `std::optional` is gone.
- No C++ exception may cross Lua: `setParent` checks for a cycle before `Node::setParent`; `loadTexture` catches a failed load and calls `lua_error` only after the `catch` ends.
- `lua/LuaCalls`: `StackRestore`, `pushFunction`, `protectedCall` for host-to-Lua calls. `protectedCall` leaves results on success, logs and pops on failure. `runChunk` keeps its older contract (error left on the stack). Use stack restoration only on protected paths.
