---
type: system
status: live
layer: "serial"
source: "Source/serial/"
started: 2026-09-09
last-update: 2026-09-21
---

## About

**What it is:** a bidirectional `Archive` with text (scene), INI (config) and JSON (descriptor) backends. One `walk` function per format serves load and save.

**Headers:** `Archive`, `IniLoad`, `IniSave`, `Json`, `NumberText`, `SceneCodec`, `TextLoad`, `TextSave`.

**Connections:** [[Scene file format]] for scenes and for Play/Stop snapshots ([[PlaySession]]) and [[Undo history]]; [[Game.ini]] and [[Project descriptor]] for projects.

**Tests:** `serial_*_test`, `scene_files_test` (every shipped scene, `Game.ini` and descriptor must re-save byte-identically).

## Code
[[Scene file format]], [[Archive]], [[Locale-independent numbers]]

## Tasks
- [ ] Scene text numeric parsing still follows the process locale (`TextLoad` uses `std::stod`/`stoll`)
- [ ] GUIDs / `.meta` sidecars — scenes store paths today

## Notes
