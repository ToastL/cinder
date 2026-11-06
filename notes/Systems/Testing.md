---
type: system
status: live
layer: "tests"
source: "Tests/"
started: 2026-09-09
last-update: 2026-11-06
---

## About

**What it is:** doctest unit tests (all headless), architecture checks, a C++ selftest project, a hot-reload test that compiles real game C++, and a graphical smoke test.

```sh
./Binaries/macOS/cbt test
```

## Code
[[cbt test]], [[Selftest]], [[graphics_smoke]], [[UI harness]]

## Notes
- Verify with `--output <scratch>`, not the user's `Binaries/` — see [[Verification recipes]].
- 2026-11-06: ctest replaced by [[cbt test]].
- No CI ([[Decision - No CI for now]]).
