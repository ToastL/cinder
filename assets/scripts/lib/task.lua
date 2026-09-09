local threads = {}

task = {}

function task.wait(seconds)
    return coroutine.yield(seconds or 0)
end

function task.spawn(fn, ...)
    local co = coroutine.create(fn)
    local ok, delay = coroutine.resume(co, ...)
    if not ok then
        print("[lua] task: " .. tostring(delay))
        return
    end
    if coroutine.status(co) ~= "dead" then
        threads[#threads + 1] = { co = co, wait = delay or 0 }
    end
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

function signal()
    return setmetatable({ handlers = {} }, signalMt)
end

function signalMt:connect(fn)
    local handlers = self.handlers
    handlers[#handlers + 1] = fn
    return {
        disconnect = function()
            for i = 1, #handlers do
                if handlers[i] == fn then
                    table.remove(handlers, i)
                    return
                end
            end
        end,
    }
end

function signalMt:fire(...)
    local handlers = self.handlers
    for i = #handlers, 1, -1 do
        local ok, err = pcall(handlers[i], ...)
        if not ok then print("[lua] signal: " .. tostring(err)) end
    end
end

stepped = signal()
rendered = signal()

function __taskStep(dt)
    local i = 1
    while i <= #threads do
        local t = threads[i]
        t.wait = t.wait - dt
        if t.wait > 0 then
            i = i + 1
        else
            local ok, delay = coroutine.resume(t.co)
            if not ok then
                print("[lua] task: " .. tostring(delay))
                table.remove(threads, i)
            elseif coroutine.status(t.co) == "dead" then
                table.remove(threads, i)
            else
                t.wait = delay or 0
                i = i + 1
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
