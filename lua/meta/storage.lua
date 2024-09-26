---@meta storage

storage = {}


---判断key是否存在
---@param key string
---@return boolean
function storage.exist(key) end


---设置一个 key-value 对 vakue可以是多个元素
---@param key string
---@param ... any
function storage.set(key, ...) end


---设置一个 key-value 对
---@param key string
---@param func fun(old:...):any...
function storage.set_if(key, func) end

--- 获取一个 key-value 值
---@param key string
---@return any
function storage.get(key) end


--- 擦除指定的 key-value 值
---@param key string
---@return any
function storage.erase(key) end

--- 获取全局存储是否空
function storage.empty() end

--- 获取全局存储大小
function storage.size() end

--- 清除所有 key-value 值
function storage.clear() end