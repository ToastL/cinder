---
type: module
system: "[[Platform]]"
source: "Source/platform/Log.hpp"
---

Everything prints through `logInfo` / `logError` — printf-style, `__attribute__((format))`-checked. Both always write to stdout/stderr, and also to a `LogSink` when one is installed.

- [[Console]] installs the sink in its constructor and clears it in its destructor; that's how `[build]`/`[module]`/`[vk]`/`[serial]` lines reach the panel.
- The sink gets the line **without** its trailing newline.
- `Log` stays in `engine`: the sink is the seam a shipping build will use for a crash log file.
