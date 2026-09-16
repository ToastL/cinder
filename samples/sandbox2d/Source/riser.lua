local box = script.parent

stepped:connect(function(dt)
    box:translate(vec3(0, box:getAttribute("speed") * dt, 0))
end)
