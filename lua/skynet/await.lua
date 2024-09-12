

----------------------------------------------------------------------------

local function await(co, func, ...)
  local handler = bind(func, ...);
  local function callback(responser, ...)
    responser(handler(...));
  end
  return function(...)
    co:dispatch(bind(callback, os.responser(), ...));
  end
end

----------------------------------------------------------------------------

return await;

----------------------------------------------------------------------------
