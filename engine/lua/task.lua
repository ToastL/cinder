local threads = {}
local current = nil

local function resume(owner, co, ...)
    local previous = current
    current = owner
    local ok, result = coroutine.resume(co, ...)
    current = previous
    return ok, result
end

local function stopped(owner)
    return owner ~= nil and owner.stopped
end

local function schedule(owner, fn, ...)
    local co = coroutine.create(fn)
    local ok, delay = resume(owner, co, ...)
    if not ok then
        print("[lua] task: " .. tostring(delay))
        return
    end
    if coroutine.status(co) == "dead" then return end
    if stopped(owner) then
        coroutine.close(co)
        return
    end
    threads[#threads + 1] = { co = co, wait = delay or 0, owner = owner }
end

task = {}

function task.wait(seconds)
    return coroutine.yield(seconds or 0)
end

function task.spawn(fn, ...)
    schedule(current, fn, ...)
end

function task.delay(seconds, fn, ...)
    local args = table.pack(...)
    task.spawn(function()
        task.wait(seconds)
        fn(table.unpack(args, 1, args.n))
    end)
end

local signalMt = {}
signalMt.__index = signalMt

local function disconnect(entry)
    if not entry.connected then return end
    entry.connected = false
    if entry.owner then entry.owner.connections[entry] = nil end

    local handlers = entry.signal.handlers
    for i = 1, #handlers do
        if handlers[i] == entry then
            table.remove(handlers, i)
            return
        end
    end
end

function signal()
    return setmetatable({ handlers = {} }, signalMt)
end

function signalMt:connect(fn)
    if stopped(current) then return { disconnect = function() end } end

    local entry = { fn = fn, signal = self, owner = current, connected = true }
    self.handlers[#self.handlers + 1] = entry
    if current then current.connections[entry] = true end
    return { disconnect = function() disconnect(entry) end }
end

function signalMt:fire(...)
    local handlers = self.handlers
    local snapshot = table.move(handlers, 1, #handlers, 1, {})
    for i = #snapshot, 1, -1 do
        local entry = snapshot[i]
        if entry.connected then
            local previous = current
            current = entry.owner
            local ok, err = pcall(entry.fn, ...)
            current = previous
            if not ok then print("[lua] signal: " .. tostring(err)) end
        end
    end
end

stepped = signal()
rendered = signal()

function __taskOwner()
    return { connections = {}, stopped = false }
end

function __taskSpawnAs(owner, fn, ...)
    schedule(owner, fn, ...)
end

function __taskStop(owner)
    if owner.stopped then return end
    owner.stopped = true
    for entry in pairs(owner.connections) do disconnect(entry) end
    for _, t in ipairs(threads) do
        if t.owner == owner and coroutine.status(t.co) == "suspended" then coroutine.close(t.co) end
    end
end

function __taskStep(dt)
    local i = 1
    while i <= #threads do
        local t = threads[i]
        if stopped(t.owner) or coroutine.status(t.co) == "dead" then
            table.remove(threads, i)
        else
            t.wait = t.wait - dt
            if t.wait > 0 then
                i = i + 1
            else
                local ok, delay = resume(t.owner, t.co)
                if not ok then
                    print("[lua] task: " .. tostring(delay))
                    table.remove(threads, i)
                elseif coroutine.status(t.co) == "dead" then
                    table.remove(threads, i)
                elseif stopped(t.owner) then
                    coroutine.close(t.co)
                    table.remove(threads, i)
                else
                    t.wait = delay or 0
                    i = i + 1
                end
            end
        end
    end
end

function __step(dt)
    __taskStep(dt)
    stepped:fire(dt)
end

function __render(alpha)
    rendered:fire(alpha)
end
