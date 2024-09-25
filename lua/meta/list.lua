---@meta list



---@class list
local list = {}

--- 判断一个list是否是空
---@return boolean
function list:empty() end
--- 获取一个list的元素数量
---@return integer
function list:size() end
--- 清空一个list
function list:clear() end
--- 擦除list中pairs遍历到的当前元素
function list:erase() end
--- 获取list的第一个元素
---@return any
function list:front() end
--- 获取list的第最后一个元素
---@return any
function list:back() end
--- 获取list的第最后一个元素
---@return any
--- 翻转list中元素
function list:reverse() end
--- 尾部插入一个新元素
---@param v any
function list:push_back(v) end
--- 弹出最后一个元素
function list:pop_back() end
--- 头部插入一个新元素
---@param v any
function list:push_front(v) end
--- 弹出第一个元素
function list:pop_front() end