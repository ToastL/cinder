---
type: format
system: "[[Projects and packaging]]"
source: "src/core/ProjectConfig.cpp"
---

`Config/Game.ini`, read through `IniLoad` without touching Lua — opening a project runs nothing.

Every key, as an example (a real file holds only non-defaults — `samples/sandbox2d`'s is just `title=Sandbox 2D`):

```ini
[Game]
title=Sandbox 2D
startScene=Scenes/main.scene
fixedHz=60
[Window]
width=1280
height=720
[Physics]
gravity=0 -9.81 0
```

`startScene` is what both executables open; `--scene` overrides. `IniSave` writes only non-defaults and drops empty sections. A project with no `Game.ini` runs on defaults. See [[Archive]].
