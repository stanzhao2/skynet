---@meta os


---返回系统版本号
---@return string
function os.version() end

---加载一个lua文件虚拟机，返回一个module对象
---@param name string 路径
---@param ... any 透传参数
---@return boolean,string|module
function os.pload(name, ...) end

---@return string 返回当前目录
function os.getcwd() end

---编译一个lua脚本，等同于直接使用luac
---@param fname string
---@param oname? string
function os.compile(fname, oname) end

---返回程序运行时间
---@param what? string "s"|"ms"
---@return number|integer
function os.clock(what) end

---返回系统时间
---@param what? string "s"|"ms"
---@return number|integer
function os.system_clock(what) end

---@return string 模块名字
function os.name() end

---创建一个协程
---@return coroutine 协程
function os.coroutine() end

---创建一个定时器
---@return timer 定时器
function os.timer() end

---返回当前平台的路径分隔符
---@return string
function os.dirsep() end

---创建一个目录，不可以递归创建
---@return boolean 是否成功
function os.mkdir(path) end

---打开一个目录
---for file_name, is_dir in pairs(os.opendir(".")) do
---end
---@return userdata 遍历的对象
function os.opendir(path) end

---获取处理器核心数量
---@return integer
function os.processors() end

---当前模块的唯一标识
---@return integer
function os.id() end

---给当前模块派发一个异步任务
---@param func fun() 任务函数
function os.post(func) end

---驱动当前模块
---@param expires? integer 超时时间，缺省就是一直等待。(毫秒)
function os.wait(expires) end

---退出当前进程
function os.exit() end

---停止并退出当前模块
function os.stop() end

---判断当前模块是否已经停止
function os.stopped() end

---判断当前是不是调试模式
function os.debugging() end
