---@meta socket

---@class socket
local socket = {}

--- 连接一个服务器
---@param host string 远端主机地址
---@param port integer 远端主机端口
---@param func? fun(ec:string|nil) 提供回调函数，则是异步连接
---@return boolean? 阻塞连接成功返回true
function socket:connect(host, port, func) end

--- 关闭套接字
function socket:close() end

--- 套接字id
function socket:id() end

--- 阻塞的方式读
---@return string|nil,string? 失败时会有错误描述
function socket:read() end

--- 阻塞的方式写
---@param data string
---@return integer|nil,string? 失败时会有错误描述，成功返回写入的字节数
function socket:write(data) end

--- 异步的方式发送
---@param data string
---@param func? fun(size:integer,err:string|nil) 失败时会有错误描述
function socket:send(data, func) end

--- 异步的方式接收
---@param func fun(data:string, err:string|nil) 失败时会有错误描述
function socket:receive(func) end

--- 获取远端或者本地端点信息
---@param what "remote"|"local"
---@return {address:string,port:integer}|nil, string? 如果失败第二个参数是错误描述
function socket:endpoint(what) end

--- 获取uri
---@return string
function socket:geturi() end

--- 获取http头选项信息
---@param name string 选项的名字
---@param what "request"|"response" 缺省是request
function socket:getheader(name, what) end

--- 设置uri
---@param uri string
function socket:seturi(uri) end

--- 设置http头选项信息
---@param name string 选项的名字
---@param value string 选项的值
function socket:setheader(name, value) end