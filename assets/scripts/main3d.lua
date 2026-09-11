engine.log("click to capture the mouse, wasd + space/ctrl to fly, esc to release")

local camera = scene:spawn("Camera")
camera.position = vec3(0, 3, 10)
local lens = camera:add("Camera")
lens.fov = 70
lens.clearColor = rgba(0.05, 0.06, 0.09, 1)
camera:behaviour("assets/scripts/behaviours/flycam.lua")

for x = -4, 4 do
    for z = -4, 4 do
        local box = scene:spawn("Box")
        box.position = vec3(x * 1.5, 0, z * 1.5)
        box:add("MeshRenderer")
        box:add("Spin").speed = vec3(0, 0.5, 0)
        box:behaviour("assets/scripts/behaviours/bobber.lua", { phase = x + z })
    end
end
