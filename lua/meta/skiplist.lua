---@meta skiplist


---@class skiplist
local skiplist = {}


---插入data 和 分数，用于比较
function skiplist:insert(data, score) end

---更新data 和 分数，用于比较， 不存在则更新
---也可以用[data]=score设置
function skiplist:update(data, score) end

---删除data的数据
---等价于[data]=nil
function skiplist:delete(data) end

---判断data是否存在
---@nodiscard
function skiplist:exists(data) end

---返回该名次上的data, score
function skiplist:get_by_rank(rank) end

---删除rank所指向的数据
function skiplist:del_by_rank(rank) end

---删除rankMin~rankMax之间的所有数据
function skiplist:del_by_rank_range(rankMin, rankMax) end

---data对应的名次
---@return integer
---@nodiscard
function skiplist:rank_of(data) end

---返回rankMin~rankMax之间的数据以table的形式,key是排名rank
---table {[rankMin] = data1, [rankMin + 1] = data2, ..., [rankMax] = dataN}
---@return table<number, any>
---@nodiscard
function skiplist:rank_range(rankMin, rankMax) end

---获取data数据对应的score
---@return integer
---@nodiscard
function skiplist:get_score(data) end


---获取scoreMin~scoreMax分数对应的数据，以及最小和最大排名
---list, rankMin, rankMax
---list = {data1, data2, ..., dataN}
---@return table, integer, integer
---@nodiscard
function skiplist:score_range(scoreMin, scoreMax) end


-- function skiplist:next(data)
-- return next value for rank of data

-- function skiplist:prev()
-- return prev value for rank of data

---跳表的长度
---等价于#操作
---@return integer
---@nodiscard
function skiplist:size() end

---遍历跳表
---for rank, data, score in rank_pairs(rankMin, rankMax) do
---@generic K, V
---@overload fun(self, rankMin, rankMax):any
---@return fun(tbl: table<K, V>):integer, K, V
function skiplist:rank_pairs(rankMin) end