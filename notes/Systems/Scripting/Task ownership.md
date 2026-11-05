---
type: concept
system: "[[Scripting]]"
source: "engine/lua/task.lua"
---

`task.lua` keeps a module-local `current` owner. A thread records the owner current when it was spawned, a connection when it connects, and resuming or firing puts that owner back — so everything a script starts, and everything *that* starts, belongs to the script.

- `__taskStop(owner)` disconnects the owner's connections and closes its suspended threads.
- A thread running when its owner stops is only marked; the scheduler drops it at its next yield.
- `__taskStop` never removes from the thread list, so it's safe from inside a resume (a script disabling itself).
- The console and the prelude run with no owner; what they start lives until Stop discards the state.
