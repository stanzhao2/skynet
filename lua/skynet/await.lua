

--[[
*********************************************************************************
** Copyright(C) 2020-2024 https://www.iccgame.com/
** Author: zhaozp@iccgame.com
*********************************************************************************
]]--

----------------------------------------------------------------------------

local random = math.random;

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
    local what = type(co);
    if what == "table" then
      co = co[random(1, #co)];
    elseif what == "function" then
      co = co();
    end
    co:dispatch(bind(callback, rpc.responser(), ...));
  end
end

----------------------------------------------------------------------------

return await;

----------------------------------------------------------------------------
