---
type: concept
system: "[[Physics]]"
source: "Source/physics/World.cpp"
---

A body slower than `SLEEP_LINEAR` and `SLEEP_ANGULAR` for `SLEEP_TIME` is ready to sleep — but sleeps only with its **island**. Contacts between dynamic bodies are unioned; an island sleeps when every member is ready. Otherwise one crate in a stack would freeze while the rest settled.

A sleeping body is solved as static and never integrated, so a settled stack is bit-for-bit still.

**Wakes on:** its node moved, velocity written, an impulse or force applied, contact with anything moving, or a contact disappearing (the floor deleted under it).
