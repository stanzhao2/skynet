---@meta context

---@class context
local context = {}

---@param str string 证书的内容,必须是pem格式证书
function context:certificate(str) end
---@param key string 证书的key
function context:key(key) end

---@param pwd string 证书的key对应的秘钥
function context:password(pwd) end