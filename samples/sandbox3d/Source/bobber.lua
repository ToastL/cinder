local box = script.parent

stepped:connect(function()
    local h = 1 + math.sin(engine.time() * box:getAttribute("rate") + box:getAttribute("phase"))
    local p = box.position
    box.position = vec3(p.x, h * 0.5, p.z)
    box.scale = vec3(1, h, 1)
end)
