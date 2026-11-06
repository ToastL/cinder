---
type: meta
last-update: 2026-11-05
---

Code conventions that every change follows. The repo's `CLAUDE.md` *Conventions* section is authoritative.

- **Zero comments.** No comments or doc blocks anywhere, by choice. Explanations go in chat, `CLAUDE.md` or this vault.
- **Trailing underscore on members.** `CINDER_PROP(position_)` strips it, so the wire key is `position`. `Camera`'s clip planes are `near_`/`far_` because `near`/`far` are `windef.h` macros.
- **`src/` is the only include root**, so every include names its layer: `#include "scene/Node.hpp"`. Headers sit next to sources. See [[Layer graph]].
- **Locale-independent numbers.** `std::to_chars`/`from_chars` in `serial`, never `printf` or streams. See [[Locale-independent numbers]].
- **Points vs pixels.** Swapchain and render targets are framebuffer pixels; `ViewCamera`, `screenToWorld` and GLFW cursor positions are window points. 2x apart on Retina.
- **Vulkan code:** every call through `vk(result, "vkFn")`, zero-initialized structs with every count set by hand, VMA for memory, move-only owners with destructors and no `destroy()`. Declaration order is the destruction contract. See [[Vulkan backend]].
- **`Glfw` is refcounted** `acquire`/`release`; `Engine` declares `input_` before `renderer_`.
- **Edge-triggered input** clears in `input.consume()` per fixed step and on every idle frame. See [[Input]].
- **`player/main.cpp` and `editor/main.cpp` duplicate arg parsing on purpose.** Don't factor it into `core`.
- **Commits** are Conventional-Commit style (`feat(ui): ...`, `refactor(gfx): ...`), with a body explaining what was verified.
