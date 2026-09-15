local cursor = script.parent

stepped:connect(function()
    local mx, my = engine.mousePosition()
    local wx, wy = engine.screenToWorld(mx, my)
    cursor.position = vec3(wx, wy, 0)
end)
