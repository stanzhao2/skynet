

--[[
*********************************************************************************
** Copyright(C) 2020-2024 https://www.iccgame.com/
** Author: zhaozp@iccgame.com
*********************************************************************************
]]--

--------------------------------------------------------------------------------

local protocol = {
  cluster  = {
    host   = "cluster-port", --name of http header
    port   = "cluster-host", --name of http header
    join   = "cluster-join"  --name of http header
  },

  ready    = "ready",
  forword  = "forword",
  deliver  = "deliver",
  bind     = "bind",
  unbind   = "unbind",
  response = "response",
};

function protocol.encode(t)
  return wrap(t);   
end

function protocol.decode(v)
  return unwrap(v);   
end

--------------------------------------------------------------------------------

return protocol;

--------------------------------------------------------------------------------
