local camera = script.parent
local pitch = camera.rotation.x
local yaw = camera.rotation.y

engine.log("click to capture the mouse, wasd + space/ctrl to fly, esc to release")

stepped:connect(function(dt)
    if engine.mousePressed("left") and not engine.cursorLocked() then
        engine.setCursorLocked(true)
    end
    if engine.keyPressed("escape") then
        if engine.cursorLocked() then
            engine.setCursorLocked(false)
        else
            engine.quit()
        end
    end

    if engine.cursorLocked() then
        local sensitivity = camera:getAttribute("sensitivity")
        local dx, dy = engine.mouseDelta()
        yaw = yaw - dx * sensitivity
        pitch = math.max(-1.55, math.min(1.55, pitch - dy * sensitivity))
        camera.rotation = vec3(pitch, yaw, 0)
    end

    local move = vec3(0, 0, 0)
    if engine.keyDown("w") then move = move + camera.forward end
    if engine.keyDown("s") then move = move - camera.forward end
    if engine.keyDown("d") then move = move + camera.right end
    if engine.keyDown("a") then move = move - camera.right end
    if engine.keyDown("space") then move.y = move.y + 1 end
    if engine.keyDown("lctrl") then move.y = move.y - 1 end

    local speed = camera:getAttribute(engine.keyDown("lshift") and "sprint" or "speed")
    camera:translate(move * (speed * dt))
end)
