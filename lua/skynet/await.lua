

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
    co:dispatch(bind(callback, os.responser(), ...));
  end
end

----------------------------------------------------------------------------

return await;

----------------------------------------------------------------------------
