---@meta io


--- 创建一个socket
---@param family "tcp"|"ssl"|"ws"|"wss"
---@param context? context 如果是带s的必须给context
---@return socket
function io.socket(family, context) end

--- 创建一个网络服务
---@param family "tcp"|"ssl"|"ws"|"wss"
---@param context? context 如果是带s的必须给context
---@return server
function io.server(family, context) end

--- 创建一个ssl上下文
---@return context
function io.context() end


io.http = {}

--- url编码
---@param url string
---@return string
function io.http.escape(url) end

--- url解码
---@param url string
---@return string
function io.http.unescape(url) end