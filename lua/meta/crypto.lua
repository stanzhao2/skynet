---@meta crypto

crypto = {}

crypto.aes = {}

---aes加密
---
--- Returns 加密后数据
---
--- 'data' 要加密的数据
---
--- 'key' 密钥
---@param data string
---@param key string
---@return string
function crypto.aes.encrypt(data, key) end


---aes解密
---
--- Returns 解密后数据
---
--- 'data' 要解密的数据
---
--- 'key' 密钥
---@param data string
---@param key string
---@return string
function crypto.aes.decrypt(data, key) end

crypto.rsa = {}
---RSA签名
---
--- Returns 签名串
---
--- 'data' 要签名的数据
---
--- 'key' 私钥
---@param data string
---@param key string
---@return string
function crypto.rsa.sign(data, key) end

---RSA签名验证
---
--- Returns 是否验证成功
---
--- 'src' 原始数据
---
--- 'sign' 签名数据
---
--- 'key' 私钥
---@param src  string
---@param sign string
---@param key  string
---@return boolean
function crypto.rsa.verify(src, sign, key) end

---RSA加密
---
--- Returns 加密后数据
---
--- 'data' 公钥/私钥
---
--- 'key' public"/"private
---
--- 'type' 私钥
---@param data  string
---@param key  string
---@return boolean
function crypto.rsa.encrypt(data, key) end

---RSA解密
---
--- Returns 要解密的数据
---
--- 'data' 公钥/私钥
---
--- 'key' public"/"private
---
---@param data  string
---@param key  string
---@return boolean
function crypto.rsa.decrypt(data, key) end

--- 获取字符串的hash值
---@param str any
function crypto.hash32(str) end

---计算字符串的sha1值
---
--- Returns sha1值,如果raw缺省或者false，则返回sha1的字符串，否则返回二进制值
---
--- 'data' 计算的数据
---
--- 'raw' 是否原值
---@overload fun(data):string
---@param data string
---@param raw? boolean
---@return string
function crypto.sha1(data, raw) end

---计算字符串的sha224值
---
--- Returns sha224值,如果raw缺省或者false，则返回sha224的字符串，否则返回二进制值
---
--- 'data' 计算的数据
---
--- 'raw' 是否原值
---@overload fun(data):string
---@param data string
---@param raw? boolean
---@return string
function crypto.sha224(data, raw) end

---计算字符串的sha256值
---
--- Returns sha256值,如果raw缺省或者false，则返回sha256的字符串，否则返回二进制值
---
--- 'data' 计算的数据
---
--- 'raw' 是否原值
---@overload fun(data):string
---@param data string
---@param raw? boolean
---@return string
function crypto.sha256(data, raw) end

---计算字符串的sha384值
---
--- Returns sha384值,如果raw缺省或者false，则返回sha384的字符串，否则返回二进制值
---
--- 'data' 计算的数据
---
--- 'raw' 是否原值
---@overload fun(data):string
---@param data string
---@param raw? boolean
---@return string
function crypto.sha384(data, raw) end

---计算字符串的sha512值
---
--- Returns sha512值,如果raw缺省或者false，则返回sha512的字符串，否则返回二进制值
---
--- 'data' 计算的数据
---
--- 'raw' 是否原值
---@overload fun(data):string
---@param data string
---@param raw? boolean
---@return string
function crypto.sha512(data, raw) end

---计算字符串的md5值
---
--- Returns md5值,如果raw缺省或者false，则返回md5的字符串，否则返回二进制值
---
--- 'data' 计算的数据
---
--- 'raw' 是否原值
---@overload fun(data):string
---@param data string
---@param raw? boolean
---@return string
function crypto.md5(data, raw) end

crypto.hmac = {}

---计算字符串的签名hmac_sha1值
---
--- Returns hmac_sha1值,如果raw缺省或者false，则返回hmac_sha1的字符串，否则返回二进制值
---
--- 'data' 计算的数据
---
--- 'key' 密钥
---
--- 'raw' 是否原值
---@overload fun(data, key):string
---@param data string
---@param key string
---@param raw? boolean
---@return string
function crypto.hmac.sha1(data, key, raw) end

---计算字符串的签名hmac_sha224值
---
--- Returns hmac_sha224值,如果raw缺省或者false，则返回hmac_sha224的字符串，否则返回二进制值
---
--- 'data' 计算的数据
---
--- 'key' 密钥
---
--- 'raw' 是否原值
---@overload fun(data, key):string
---@param data string
---@param key string
---@param raw? boolean
---@return string
function crypto.hmac.sha224(data, key, raw) end

---计算字符串的签名hmac_sha256值
---
--- Returns hmac_sha256值,如果raw缺省或者false，则返回hmac_sha256的字符串，否则返回二进制值
---
--- 'data' 计算的数据
---
--- 'key' 密钥
---
--- 'raw' 是否原值
---@overload fun(data, key):string
---@param data string
---@param key string
---@param raw? boolean
---@return string
function crypto.hmac.sha256(data, key, raw) end

---计算字符串的签名hmac_sha384值
---
--- Returns hmac_sha384值,如果raw缺省或者false，则返回hmac_sha384的字符串，否则返回二进制值
---
--- 'data' 计算的数据
---
--- 'key' 密钥
---
--- 'raw' 是否原值
---@overload fun(data, key):string
---@param data string
---@param key string
---@param raw? boolean
---@return string
function crypto.hmac.sha384(data, key, raw) end

---计算字符串的签名hmac_sha512值
---
--- Returns hmac_sha512值,如果raw缺省或者false，则返回hmac_sha512的字符串，否则返回二进制值
---
--- 'data' 计算的数据
---
--- 'key' 密钥
---
--- 'raw' 是否原值
---@overload fun(data, key):string
---@param data string
---@param key string
---@param raw? boolean
---@return string
function crypto.hmac.sha512(data, key, raw) end

---计算字符串的签名hmac_md5值
---
--- Returns hmac_md5值,如果raw缺省或者false，则返回hmac_md5的字符串，否则返回二进制值
---
--- 'data' 计算的数据
---
--- 'key' 密钥
---
--- 'raw' 是否原值
---@overload fun(data, key):string
---@param data string
---@param key string
---@param raw? boolean
---@return string
function crypto.hmac.md5(data, key, raw) end