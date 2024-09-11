

local skiplist = std.skiplist;
local class    = require("skynet.classy");
local message  = class("message");

----------------------------------------------------------------------------

function message:__init()
  self.handlers = {};
end

function message:register(name, handler, priority)
  assert(type(handler) == "function");
  local event = self.handlers[name];
  if not priority then
    priority = 0;
  else
    priority = -priority;
  end
  if not event then
    event = skiplist();
    self.handlers[name] = event;
  end  
  event:insert(handler, priority);
  return handler;
end

function message:dispatch(name, ...)
  local event = self.handlers[name];
  if not event then
    return 0;
  end
  local count = 0;
  for i, handler in pairs(event:rank_range()) do
    local ok, err = pcall(handler, ...);
    if not ok then
      error(err);
    else
      count = count + 1;
    end
  end  
  return count;
end

function message:unregister(name, handler)
  assert(type(handler) == "function");
  local event = self.handlers[name];
  if not event then
    return;
  end  
  if event:delete(handler) and event:size() == 0 then
    self.handlers[name] = nil;
  end
end

----------------------------------------------------------------------------

return message;

----------------------------------------------------------------------------
