---
type: decision
status: settled
date: 2026-11-06
---

**Decision:** game code is C++.

**Choices (via questions):** game code is **node classes** (`CINDER_NODE`/`CINDER_PROP`, registered with `CINDER_REGISTER`); a **hot-reloaded library** per project; the **editor drives clang** (UnrealBuildTool-style, no per-project CMake); packaging without CMake through a separate `packager` binary, with Development/Shipping configurations.

**How to apply:** see [[Game code]].
