---@meta global


--- 给一个函数绑定参数
---@param func fun()
---@param ... any
---@return fun(...)
function bind(func, ...) end

--- 以安全的方式执行一个函数
--- 如果func是函数，则执行func,如果func是字符串，则执行rpc调用一个远端函数，这时候第二个参数可以是一个回调函数，如果设置了，则是一个异步rpc，否则是一个阻塞rpc
---@overload fun(name:string, callbakc?:fun(), ...)
---@param func fun()
---@param ... any 透传给要执行的function的参数
function pcall(func, ...) end

--- 打印调试信息，进程必须被编译成debug模式。
function trace(...) end

--- 抛出一个异常
function throw(...) end

--- 打包任意个lua的标准类型，返回一个打包后的字符串
---@param ... number|integer|string|table|boolean|nil
---@return string
function wrap(...) end

--- 解包一个由warp打包的数据
---@param data string
---@return ...
function unwrap(data) end

--- 压缩一段数据
---@param data string
---@param what? "deflate"|"gzip" 缺省为"deflate"
---@return string
function compress(data, what) end


--- 解压缩一段数据由compress压缩的数据
---@param data string
---@param what? "deflate"|"gzip" 缺省为"deflate"
---@return string
function uncompress(data, what) end
