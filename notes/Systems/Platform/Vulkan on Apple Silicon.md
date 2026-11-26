---
type: concept
system: "[[Platform]]"
source: "Source/platform/Glfw.cpp"
---

Two failures, one cause: Homebrew's `/opt/homebrew/lib` isn't on the dyld default search path, and the Vulkan loader `dlopen`s by leaf name.

- A leaf-name `dlopen` of `libvulkan` finds nothing. `Glfw::acquire` walks `LOADER_CANDIDATES` (leaf names, Homebrew, `/usr/local`, MoltenVK) and keeps the first `vkGetInstanceProcAddr`; `Gfx/VK/Loader` loads every function from it. volk was removed on 2026-11-26.
- The validation layer enumerates but `vkCreateInstance` returns `VK_ERROR_LAYER_NOT_PRESENT`, because the manifest's `library_path` is a bare dylib name. The context retries without the layer and prints a notice.

**Fix:** run with `DYLD_LIBRARY_PATH=/opt/homebrew/lib`. Do this whenever touching the renderer — validation once caught a missing `TRANSFER_SRC_BIT` no test would have. A Metal build needs none of this.
