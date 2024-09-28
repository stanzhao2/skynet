

--[[
*********************************************************************************
** Copyright(C) 2020-2024 https://www.iccgame.com/
** Author: zhaozp@iccgame.com
*********************************************************************************
]]--

----------------------------------------------------------------------------

local function await(co, func, ...)
  local handler = bind(func, ...);
  local function callback(responser, ...)
    if responser == nil then
      handler(...);
    else
      responser(handler(...));
    end
  end
  return function(...)
    co:dispatch(bind(callback, rpc.responser(), ...));
  end
end

----------------------------------------------------------------------------

return await;

----------------------------------------------------------------------------
