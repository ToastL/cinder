---
type: decision
status: settled
date: 2026-09-23
---

**Decision:** the graphics backend is picked by the `CINDER_BACKEND` cache variable (`vk` or `mtl`), not at runtime. No vtables: `gfx/rhi` declares the types, exactly one of `gfx/vk` or `gfx/mtl` defines them.

**Why:** the same reasoning as [[Decision - Dev tools as a link target]] — a binary carries only what it uses, and `Engine` keeps holding `rhi::Ctx` by value.

**How to apply:** nothing outside a backend directory names a Vulkan or Metal type. See [[Backend seam]].
