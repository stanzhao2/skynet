

--[[
*********************************************************************************
** Copyright(C) 2020-2024 https://www.iccgame.com/
** Author: zhaozp@iccgame.com
*********************************************************************************
]]--

--------------------------------------------------------------------------------

local format = string.format;
local proto_type = require("cluster.protocol");
local active_sessions = {};

--------------------------------------------------------------------------------

local function sendto_member(session)
  for peer, v in pairs(active_sessions) do
    if peer ~= session.socket then
	  local packet = {
	    what = proto_type.forword,
		id   = v.socket:id(),
		ip   = v.ip,
		port = v.port,
	  };
	  session.socket:send(wrap(packet));
	end
  end
  local peer = session.socket;
  peer:send(wrap({what = proto_type.ready}));
end

--------------------------------------------------------------------------------

local function on_error(peer, msg)
  local session = active_sessions[peer];
  if session then
    active_sessions[peer] = nil;
    peer:close();
    error(format("cluster member error: %s", msg));
  end
end

--------------------------------------------------------------------------------

local function on_receive(peer, data, ec)
  if ec then
    on_error(peer, ec);
	return;
  end
end

--------------------------------------------------------------------------------

local function new_session(peer)
  local port = peer:getheader(proto_type.cluster.port);
  if not port then
    return false;
  end
  local host = peer:getheader(proto_type.cluster.host);
  if not host then
    host = peer:endpoint().address;
  end
  local session = {
    socket = peer,
	ip     = host,
	port   = port,
  };
  active_sessions[peer] = session;
  sendto_member(session);
  print(format("cluster member accept from %s", host));
  return true;
end

--------------------------------------------------------------------------------

local function on_accept(peer, ec)
  if ec then
    on_error(peer, ec);
	return;
  end
  if not new_session(peer) then
    peer:close();
	return;
  end
  peer:receive(bind(on_receive, peer));
end

--------------------------------------------------------------------------------

function main(port, host)
  port = port or 80;
  local server = io.server("ws");
  local ok = server:listen(host or "0.0.0.0", port, on_accept);  
  if not ok then
    error(format("server listen error at port: %d", port));
	return;
  end 
  
  print(format("%s works on port %d", os.name(), port));  
  while not os.stopped() do
    os.wait();
  end
  server:close();
end

--------------------------------------------------------------------------------
