---@meta rpc


rpc = {}

--- 注册一个rpc函数
---@param name string rpc函数名
---@param func fun()
---@param invoke_by_remote? boolean 是否被远端调用，必须是其他进程
function rpc.create(name, func, invoke_by_remote) end

---@param name string rpc函数名
---@param mask integer
---@param receiver integer
---@param callback? fun()
function rpc.call(name, mask, receiver, callback, ...) end

--- 注销一个rpc函数
---@param name string rpc函数名
function rpc.remove(name) end

--- 执行一个远端rpc函数,无需返回值
---@param name string rpc函数名
---@param mask integer 掩码。如果是0，则所有同名的rpc函数都会被调用。如果不是0，则挑一个rpc函数属性，必须receiver为0生效
---@param receiver integer 接受者，如果不是0，则只会发送给该接受者所在的模块。
---@param ... any 传递给rpc函数的参数
function rpc.deliver(name, mask, receiver, ...) end

---获取调用者的id
function rpc.caller() end

---获取应答函数
function rpc.responser() end
