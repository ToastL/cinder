local actor = script.actor

stepped:connect(function()
    local h = 1 + math.sin(engine.time() * actor:getAttribute("rate") + actor:getAttribute("phase"))
    local p = actor.position
    actor.position = vec3(p.x, h * 0.5, p.z)
    actor.scale = vec3(1, h, 1)
end)
