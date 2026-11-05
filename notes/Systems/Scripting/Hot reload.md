---
type: concept
system: "[[Scripting]]"
source: "src/script/LuaHost.cpp"
---

Only scripts hot-reload, and editing one reloads only that file.

- `LuaHost::reloadScript` calls `__scriptForget(path)` then `__scriptCheck(path)` to compile once. A compile error logs once and changes nothing — scripts on the old code keep running.
- Otherwise every started, enabled `Script` on that file runs again: its old owner stops ([[Task ownership]]) and the file runs from the top.
- Roblox's trade: locals start over; [[Attributes]] survive.
- `__scriptRead` is the single funnel for reading script files, so it records what to watch. Nothing walks a directory. `poll()` stats each read script every frame (in `Engine::beginFrame`).
- In Edit mode nothing has been read, so nothing is watched; Play reads every script from disk anyway.
- A script that failed to compile at start is still watched, so fixing the file starts it.
