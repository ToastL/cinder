---
type: system
status: live
layer: "tests"
source: "tests/"
started: 2026-09-09
last-update: 2026-09-26
---

## About

**What it is:** doctest unit tests (all headless), architecture checks, a Lua selftest project, and a graphical smoke test. 222 headless test cases and a 65-check selftest per `TODO.md` (`CLAUDE.md` says 60 checks (?)).

```sh
ctest --test-dir build --output-on-failure
```

## Code
[[ctest suite]], [[Lua selftest]], [[graphics_smoke]], [[UI harness]]

## Notes
- Verify in a scratch tree, not `build/` — see [[Verification recipes]].
- No CI ([[Decision - No CI for now]]).
