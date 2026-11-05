---
type: concept
system: "[[Platform]]"
source: "src/platform/Glfw.cpp"
---

Two failures, one cause: Homebrew's `/opt/homebrew/lib` isn't on the dyld default search path, and both volk and the Vulkan loader `dlopen` by leaf name.

- `volkInitialize()` fails because volk hardcodes `/usr/local/lib`. `Glfw::acquire` walks its own candidate list and falls back to `volkInitializeCustom`.
- The validation layer enumerates but `vkCreateInstance` returns `VK_ERROR_LAYER_NOT_PRESENT`, because the manifest's `library_path` is a bare dylib name. The context retries without the layer and prints a notice.

**Fix:** run with `DYLD_LIBRARY_PATH=/opt/homebrew/lib`. Do this whenever touching the renderer — validation once caught a missing `TRANSFER_SRC_BIT` no test would have. A Metal build needs none of this.
