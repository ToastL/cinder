local actorMt = {}
local compMt = {}
local cache = setmetatable({}, { __mode = "v" })
local isComponent = {}

for _, name in ipairs(engine.componentNames()) do isComponent[name] = true end

local ANY_ATTRIBUTE = "*"
local attributeSignals = {}

local function attributeValue(value)
    if type(value) ~= "table" then return value end
    if #value == 2 then return vec2(value[1], value[2]) end
    if #value == 3 then return vec3(value[1], value[2], value[3]) end
    return vec4(value[1], value[2], value[3], value[4])
end

local function attributeSignal(id, name)
    local byName = attributeSignals[id]
    if byName == nil then
        byName = {}
        attributeSignals[id] = byName
    end
    local changed = byName[name]
    if changed == nil then
        changed = signal()
        byName[name] = changed
    end
    return changed
end

function __attributeChanged(id, name)
    local byName = attributeSignals[id]
    if byName == nil then return end
    if byName[name] then byName[name]:fire() end
    if byName[ANY_ATTRIBUTE] then byName[ANY_ATTRIBUTE]:fire(name) end
end

local function component(id, name)
    return setmetatable({ id = id, component = name }, compMt)
end

function __actor(id)
    if id == nil then return nil end
    local a = cache[id]
    if a == nil then
        a = setmetatable({ id = id }, actorMt)
        cache[id] = a
    end
    return a
end

compMt.__index = function(self, key)
    local x, y, z, w = engine.getProp(self.id, self.component, key)
    if y == nil then return x end
    if z == nil then return vec2(x, y) end
    if w == nil then return vec3(x, y, z) end
    return vec4(x, y, z, w)
end

compMt.__newindex = function(self, key, value)
    local n = vecSize(value)
    if n == nil then
        engine.setProp(self.id, self.component, key, value)
    else
        engine.setProp(self.id, self.component, key, value:unpack())
    end
end

compMt.__tostring = function(self)
    return self.component .. "(" .. self.id .. ")"
end

local get = {
    name           = function(self) return engine.name(self.id) end,
    parent         = function(self) return __actor(engine.parent(self.id)) end,
    active         = function(self) return engine.active(self.id) end,
    position       = function(self) return vec3(engine.position(self.id)) end,
    rotation       = function(self) return vec3(engine.rotation(self.id)) end,
    scale          = function(self) return vec3(engine.scale(self.id)) end,
    worldPosition = function(self) return vec3(engine.worldPosition(self.id)) end,
    forward        = function(self) return vec3(engine.forward(self.id)) end,
    right          = function(self) return vec3(engine.right(self.id)) end,
    up             = function(self) return vec3(engine.up(self.id)) end,
    attributeChanged = function(self) return attributeSignal(self.id, ANY_ATTRIBUTE) end,
}

local set = {
    name     = function(self, v) engine.setName(self.id, v) end,
    parent   = function(self, v) engine.setParent(self.id, v and v.id or nil) end,
    active   = function(self, v) engine.setActive(self.id, v) end,
    position = function(self, v) engine.setPosition(self.id, v:unpack()) end,
    rotation = function(self, v) engine.setRotation(self.id, v:unpack()) end,
    scale    = function(self, v) engine.setScale(self.id, v:unpack()) end,
}

local fns = {}

function fns:add(name)
    engine.addComponent(self.id, name)
    return component(self.id, name)
end

function fns:get(name)
    if not engine.hasComponent(self.id, name) then return nil end
    return component(self.id, name)
end

function fns:addScript(file)
    local added = self:add("Script")
    added.file = file
    return added
end

function fns:getAttribute(name)
    return attributeValue(engine.getAttribute(self.id, name))
end

function fns:setAttribute(name, value)
    engine.setAttribute(self.id, name, value)
end

function fns:getAttributes()
    local out = engine.getAttributes(self.id)
    for name, value in pairs(out) do out[name] = attributeValue(value) end
    return out
end

function fns:getAttributeChangedSignal(name)
    return attributeSignal(self.id, name)
end

function fns:spawn(name)
    return __actor(engine.spawn(name, self.id))
end

function fns:children()
    local out = {}
    for i, id in ipairs(engine.children(self.id)) do out[i] = __actor(id) end
    return out
end

function fns:child(name)
    for _, c in ipairs(self:children()) do
        if c.name == name then return c end
    end
    return nil
end

function fns:translate(v)
    engine.translate(self.id, v:unpack())
end

function fns:destroy()
    engine.destroy(self.id)
end

function fns:valid()
    return engine.valid(self.id)
end

actorMt.__index = function(self, key)
    local getter = get[key]
    if getter then return getter(self) end

    local fn = fns[key]
    if fn then return fn end

    if isComponent[key] then return self:get(key) end
    return nil
end

actorMt.__newindex = function(self, key, value)
    local setter = set[key]
    if setter == nil then
        error("actor has no property '" .. tostring(key) .. "'", 2)
    end
    setter(self, value)
end

actorMt.__tostring = function(self)
    return (engine.name(self.id) or "?") .. "(" .. self.id .. ")"
end

scene = {}

function scene:spawn(name, parent)
    return __actor(engine.spawn(name, parent and parent.id or nil))
end

function scene:find(name)
    return __actor(engine.find(name))
end

function scene:destroy(actor)
    engine.destroy(actor.id)
end
