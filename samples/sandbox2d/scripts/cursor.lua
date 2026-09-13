local actor = script.actor

stepped:connect(function()
    local mx, my = engine.mousePosition()
    local wx, wy = engine.screenToWorld(mx, my)
    actor.position = vec3(wx, wy, 0)
end)
