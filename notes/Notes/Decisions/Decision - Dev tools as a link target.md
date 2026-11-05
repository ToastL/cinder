---
type: decision
status: settled
date: 2026-09-11
---

**Decision:** editor-only code lives in `src/dev/`, built as `engine_dev`, linked by `editor` and `tests` but never by `player`. There is no `--dev` flag.

**Why:** nothing editor-only reaches the shipping binary. `ctest` runs `nm` over `player` (`shipping_binary_is_clean`) to prove it.

**How to apply:** the only seam is `Renderer::setUiPaint`. Don't add editor code to `engine`. See [[Editor]].
