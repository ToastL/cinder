local spawner = script.parent

stepped:connect(function()
    if not engine.mouseDown("left") then return end

    local minSize = spawner:getAttribute("minSize")
    local maxSize = spawner:getAttribute("maxSize")
    local mx, my = engine.mousePosition()
    local wx, wy = engine.screenToWorld(mx, my)
    local size = minSize + math.random() * (maxSize - minSize)

    local box = scene:create("Sprite")
    box.name = "Box"
    box.position = vec3(wx, wy, 0)
    box.size = vec2(size, size)
    box.color = rgba(math.random(), math.random(), math.random(), 0.85)

    box:setAttribute("speed", 20 + math.random() * 80)
    box:add("Script").file = spawner:getAttribute("boxScript")

    task.delay(spawner:getAttribute("lifetime"), function()
        if box:valid() then box:destroy() end
    end)
end)
