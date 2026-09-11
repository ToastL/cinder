engine.log("wasd/arrows pan, scroll zooms, click spawns")

local camera = scene:spawn("Camera")
local lens = camera:add("Camera")
lens.projection = "orthographic"
lens.clearColor = rgba(0.05, 0.05, 0.08, 1)
lens.virtualSize = vec2(640, 360)
camera:behaviour("assets/scripts/behaviours/camera2d.lua")

local spawner = scene:spawn("Spawner")
spawner:behaviour("assets/scripts/behaviours/spawner.lua")

local cursor = scene:spawn("Cursor")
cursor:add("SpriteRenderer").size = vec2(6, 6)
cursor:behaviour("assets/scripts/behaviours/cursor.lua")
