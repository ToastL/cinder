local dropper = script.parent
local spread = dropper:getAttribute("spread")

while true do
    task.wait(dropper:getAttribute("interval"))

    local ball = scene:create("Body")
    ball.name = "Dropped"
    ball.position = dropper.worldPosition
        + vec3((math.random() - 0.5) * spread, 0, (math.random() - 0.5) * spread)
    ball.scale = vec3(0.8, 0.8, 0.8)
    ball.linearDamping = 0.2

    ball:add("MeshPart").mesh = "sphere"

    local collider = ball:add("Collider")
    collider.shape = "sphere"
    collider.restitution = 0.5

    task.delay(dropper:getAttribute("lifetime"), function()
        if ball:valid() then ball:destroy() end
    end)
end
