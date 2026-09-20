local box = script.parent
local speed = script:getAttribute("speed") or vec3(0, 1, 0)

if box.rotation then
    stepped:connect(function(dt)
        box.rotation = box.rotation + speed * dt
    end)
end
