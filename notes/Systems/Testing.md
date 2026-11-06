---
type: system
status: live
layer: "tests"
source: "tests/"
started: 2026-09-09
last-update: 2026-09-26
---

## About

**What it is:** doctest unit tests (all headless), architecture checks, a C++ selftest project, a hot-reload test that compiles real game C++, and a graphical smoke test.

```sh
ctest --test-dir build --output-on-failure
```

## Code
[[ctest suite]], [[Selftest]], [[graphics_smoke]], [[UI harness]]

## Notes
- Verify in a scratch tree, not `build/` — see [[Verification recipes]].
- No CI ([[Decision - No CI for now]]).
