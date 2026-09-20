local spawner = script.parent

engine.log("click to drop a box")

stepped:connect(function()
    if not engine.mousePressed("left") then return end

    local mx, my = engine.mousePosition()
    local wx, wy = engine.screenToWorld(mx, my)

    local box = scene:create("Body")
    box.name = "Dropped"
    box.planar = true
    box.position = vec3(wx, wy, 0)

    local sprite = box:add("Sprite")
    sprite.size = vec2(1, 1)
    sprite.color = rgba(0.4 + math.random() * 0.6, 0.4 + math.random() * 0.6, 0.9, 1)

    box:add("Collider")

    task.delay(spawner:getAttribute("lifetime"), function()
        if box:valid() then box:destroy() end
    end)
end)
