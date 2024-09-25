---@meta coroutine

---@class coroutine
local coroutine = {}

---关闭当前协程协程
---@param func? fun() 协程退出回调
function coroutine:close(func) end

---给协程派发一个任务
---@param func fun() 任务函数
function coroutine:dispatch(func) end