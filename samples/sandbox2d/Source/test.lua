local time = 0
local frames = 0

stepped:connect(function(dt)
    time = time + dt

    if time > 1 then
        engine.log("FPS: " .. frames)
        time = 0
        frames = 0
    end
end)

rendered:connect(function()
    frames = frames + 1
end)