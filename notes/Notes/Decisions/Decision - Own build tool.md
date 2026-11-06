---
type: decision
status: settled
date: 2026-11-06
---

**Decision:** cinder gets its own build tool that builds the engine and game code, replacing CMake — UnrealBuildTool's model. The user chose "option 2" over keeping CMake plus `cinder-toolchain.json`.

**Choices (via questions):** written in C++, bootstrapped by a one-command script; module rules are code like Unreal's `Build.cs` (C++ rule files beside each module; a project's rules compiled into a library the tool loads); side by side with CMake, then delete it.

**Where:** in this repo, like UBT inside the engine; it encodes how the engine is built, so it is versioned with it.

**How to apply:** plan first. Keep CMake working until the tool reaches parity, then delete it. Generated Xcode/VS projects only call the tool. See [[Build and shaders]], [[Game code]].

## Layout (2026-11-06)
The user noticed the mixed casing — "there is the whole word "Program" or "ThirdParty" and then there is the non-capitalized "src" and "engine"" — and approved the proposal with "sure go ahead": the repo root takes a project's shape. `src/` → `Source/` (still the include root), `ThirdParty/` and `Programs/` move inside it, `engine/shaders/` → `Shaders/`, `engine/fonts/` → `Content/Fonts/`, `samples/` → `Samples/`, `tests/` → `Tests/`. Container folders are capitalized; module folders stay lowercase to match namespaces and include paths. A package mirrors it: `player`, `Engine/Shaders/`, `Engine/Content/Fonts/`, `Project/`.

## Done (2026-11-06)
All 7 phases, verified on macOS: `cbt test` 6/6; `graphics_smoke` and the selftest ALL PASS; the gallery capture is byte-identical to CMake's; a Shipping package runs from another folder; Xcode builds through the generated project. Linux untested. See [[cbt]].

