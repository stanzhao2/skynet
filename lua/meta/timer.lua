---@meta timer

---@class timer
local timer = {}

---@param ms number 定时毫秒
---@param func fun() 任务函数
function timer:expires(ms, func) end

function timer:cancel() end