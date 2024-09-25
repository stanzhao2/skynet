---@meta string



--- 切割字符串
---@param str string 要切割的字符串
---@param seq string 要切割的符号
---@return string[]
function string.split(str, seq) end

--- 去处两端空格
---@param str string
---@return string
function string.trim(str) end

--- 忽略大小写比较
---@param str1 string
---@param str2 string
---@return boolean
function string.icmp(str1, str2) end

--- 判断是否是字母组成
---@param str string
---@return boolean
function string.isalpha(str) end

--- 判断是否是数字组成
---@param str string
---@return boolean
function string.isalnum(str) end