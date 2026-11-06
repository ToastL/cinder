---
type: concept
system: "[[Projects and packaging]]"
source: "src/core/ProjectConfig.cpp"
---

```
MyGame/
  MyGame.cinder      project descriptor (Unreal's .uproject)
  Config/Game.ini    settings
  Content/           scenes, textures, meshes (Scenes/, Textures/ by convention)
  Source/            the game's C++
  Saved/             per-user and generated; gitignored, never packaged
```

`ProjectConfig::root` takes the folder or its `.cinder` file. `ProjectConfig::load` requires exactly one `*.cinder`, parses it ([[Project descriptor]]), then reads [[Game.ini]]. Both content roots sit under the project root, made absolute, so a project runs from any working directory. See [[Assets and paths]].

There's no entry point: `startScene` names the scene, and game code is node classes in `Source/` ([[Game code]]).
