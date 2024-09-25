---@meta json

json = {}
--- 将json字符串反序列成LUA的变量
---
--- Returns LUA的变量
---
--- 'str' 是一个json标准的字符串
---@param str string
---@return any
function json.decode(str) end

--- 将LUA的变量序列化成字符串
---
--- Returns json字符串
---
--- 'value' 是一个lua的一个变量，可以是以下类型
--- **`number`**
--- **`boolean`**
--- **`table`**
---@param value any
---@return string
function json.encode(value) end