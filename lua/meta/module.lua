---@meta module

---@class module
local module = {}
---@return
---| '"running"'   # 运行中.
---| '"error"'     # 运行时出错.
---| '"dead"'      # 已退出.
function module:state() end
---@return number
function module:id() end
function module:close() end