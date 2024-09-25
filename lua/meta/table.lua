---@meta table


--- 以只读方式共享table
---@param table table
---@param name string 共享的名字
function table.export(table, name) end

--- 导入一个共享的只读table
---@param name string
---@return table
function table.import(name) end