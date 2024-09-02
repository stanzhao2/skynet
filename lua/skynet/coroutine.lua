

--[[
*********************************************************************************
** Copyright(C) 2020-2024 https://www.iccgame.com/
** Author: zhaozp@iccgame.com
*********************************************************************************
]]--

--------------------------------------------------------------------------------

local format   = string.format;
local class    = require("skynet.classy");
local co_class = class("co_class");

--------------------------------------------------------------------------------

local function print_if_error(name, ok, err)
  if ok ~= true then
    error(format("coroutine(%s) error: %s", name, err));
  end
end

--------------------------------------------------------------------------------

function co_class:__init(name)
  self.tasklist = std.list();
  self.name     = name or "no name";
  self.closed   = false;
  self.co = coroutine.create(bind(self.comain, self));
end

--------------------------------------------------------------------------------

function co_class:__gc()
  if self.co then
    coroutine.close(self.co);
    self.co = nil;
  end
end

--------------------------------------------------------------------------------

function co_class:close(callback)
  assert(callback == nil or type(callback) == "function");
  if not self.closed then
    self.closed = true;
    self:dispatch(callback or function() end);
  end   
end

--------------------------------------------------------------------------------

function co_class:comain()
  while true do
    if self.tasklist:empty() then
      if self.closed then
        break;
      end
      coroutine.yield();
    else
      print_if_error(self.name, pcall(self.tasklist:front()));
      self.tasklist:pop_front();
    end
  end
end

--------------------------------------------------------------------------------

function co_class:dispatch(task)
  assert(type(task) == "function");
  if not self.closed then
    self.tasklist:push_back(task);
    if coroutine.status(self.co) == "suspended" and self.tasklist:size() == 1 then
      os.post(coroutine.resume, self.co);
    end
  end
end

--------------------------------------------------------------------------------

return co_class;

--------------------------------------------------------------------------------
