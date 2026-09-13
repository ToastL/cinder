local actor = script.actor

stepped:connect(function(dt)
    actor:translate(vec3(0, actor:getAttribute("speed") * dt, 0))
end)
