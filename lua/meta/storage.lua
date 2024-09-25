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
---@overload fun(key:string, handler:fun(old_value:any):any):any
---@param key string
---@param checkfunc fun(new_value:..., old_value:...):boolean
---@param value ... any
function storage.set_if(key, checkfunc, value) end

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