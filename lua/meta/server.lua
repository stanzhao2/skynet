---@meta server

---@class server
local server = {}


--- 创建监听
---@param host string 监听地址
---@param port integer 监听端口
---@param func fun(peer:socket, ec:string|nil)
---@return boolean 是否成功
---@return string? 错误描述
function server:listen(host, port, func) end

--- 服务id
---@return integer
function server:id() end

--- 关闭服务
function server:close() end

--- 获取端点信息
---@return {address:string,port:integer}|nil, string? 如果失败第二个参数是错误描述
function server:endpoint() end
