---
type: decision
status: settled
date: 2026-09-21
---

**Decision:** the project browser (first screen of `editor` with no argument) is deferred, and will be built a different way than Claude's plan.

**Why:** Claude proposed recent projects in a per-user settings folder, Empty 2D/3D templates, `nativefiledialog-extended` and a window that closes and reopens on the chosen project. The user: "i want to do the project browser later, a different way".

**How to apply:** don't propose it as the next step. When it comes up, ask how they want it done before planning. The controls it needs (TextField, ScrollBox, Image) exist in `src/ui/`.
