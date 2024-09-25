 **SKYNET for Lua5.4**
- 支持单机和集群模式运行
- 支持单机和集群内 RPC 调用(同步/异步)
- 支持协程
- 单进程可启动多个 Lua 微服务
- 每个微服务从 main 函数开始运行
- 内置 Timer 和 Socket (支持SSL和WSS)
- 内置 Lua table 的共享机制, 方便配置数据的多模块共享访问

 **加入集群范例**
```  
  function main(host, port)
    host = host or "127.0.0.1";
    port = port or 80;
    local ok, cluster = os.pload("cluster.adapter", host, port);
    if not ok then  
      error(cluster);
	  return;
    end
    while not os.stopped() do
	  os.wait();
	end
    if cluster then
      cluster:close();
    end
  end
```

 **skynet-lua usage**
-   skynet name [...]

 **skynet cluster**
-   skynet cluster.master [port] [host]

 **skynet http broker**
-   skynet http.broker [port] [host] [ca] [key] [pwd] 

 **global functions**
-   bind(func, [, ...])
-   pcall(<func [, ...]>/<name [, callback] [, ...]>)
-   trace(...)
-   throw(...)
-   wrap(...)
-   unwrap(str)
-   compress(str [,<"deflate">/<"gzip">])
-   uncompress(str [,<"deflate">/<"gzip">])

 **os functions** 
-   os.version()
-   os.pload(name [, ...]) #1
-   os.compile(fname [, oname])
-   os.getcwd()
-   os.name()
-   os.coroutine() #7
-   os.timer() #2
-   os.dirsep()
-   os.mkdir(name)
-   os.opendir([name])
-   os.processors()
-   os.id()
-   os.post(func [, ...])
-   os.wait([expires])
-   os.exit()
-   os.stop()
-   os.stopped()
-   os.debugging()

 **rpc functions** 
-   rpc.create(name, func [, <true>/<false>])
-   rpc.remove(name)
-   rpc.deliver(name, mask, receiver [, ...])
-   rpc.caller()
-   rpc.responser()
 
 **coroutine functions**
-   co:close(func)
-   co:dispatch(func)

 **std functions**
-   std.list() #3
-   std.skiplist([compfunc]) #8

 **list functions**
-   list:empty()
-   list:size()
-   list:clear()
-   list:erase(iter)
-   list:front()
-   list:back()
-   list:reverse()
-   list:push_back(v)
-   list:pop_back()
-   list:push_front(v)
-   list:pop_front()

 **job functions**
-   job:close()
-   job:id()
-   job:state()

 **io functions** 
-   io.socket(<"tcp">/<"ssl">/<"ws">/<"wss">, [context]]) #4
-   io.server(<"tcp">/<"ssl">/<"ws">/<"wss">>, [context]) #5
-   io.context() #6
-   io.http.request_parser(options)
-   io.http.response_parser(options)
-   io.http.parse_url(url)
-   io.http.escape(url)
-   io.http.unescape(url)

 **socket functions**
-   socket:connect(host, port [, func])
-   socket:close()
-   socket:id()
-   socket:read()
-   socket:write(data)
-   socket:send(data [,func])
-   socket:receive(func)
-   socket:endpoint([<"remote">/<"local">])
-   socket:geturi()
-   socket:getheader(name [,<"request">/<"response">])
-   socket:seturi(uri)
-   socket:setheader(name, value)

 **server functions**
-   server:listen(host, port, func)
-   server:id()
-   server:close();
-   server:endpoint()

 **context functions**
-   context:certificate(str)
-   context:key(str)
-   context:password(str)
 
 **timer functions**
-   timer:expires(ms, func)
-   timer:cancel()

 **string functions**
-   string.split(str, seq)
-   string.trim(str)
-   string.icmp(str1, str2)
-   string.isalpha(str)
-   string.isalnum(str)

 **table functions**
-   table.export(table, name)
-   table.import(name)

 **storage functions**
-   storage.exist(key)
-   storage.set(key, value [, ...])
-   storage.set_if(key, checkfunc, value [, ...])
-   storage.get(key)
-   storage.erase(key)
-   storage.empty()
-   storage.size()
-   storage.clear()

 **json functions** 
-   json.encode(value)
-   json.decode(str)

 **base64 functions** 
-   base64.encode(str)
-   base64.decode(str)

 **crypto functions** 
-   crypto.aes.encrypt(str, key)
-   crypto.aes.decrypt(str, key)
-   crypto.rsa.sign(str, key)
-   crypto.rsa.verify(src, sign, key)
-   crypto.rsa.encrypt(str, key)
-   crypto.rsa.decrypt(str, key)
-   crypto.hash32(str)
-   crypto.sha1(str [,<false>/<true>])
-   crypto.sha224(str [,<false>/<true>])
-   crypto.sha256(str [,<false>/<true>])
-   crypto.sha384(str [,<false>/<true>])
-   crypto.sha512(str [,<false>/<true>])
-   crypto.md5(str [,<false>/<true>])
-   crypto.hmac.sha1(str, key [,<false>/<true>])
-   crypto.hmac.sha224(str, key [,<false>/<true>])
-   crypto.hmac.sha256(str, key [,<false>/<true>])
-   crypto.hmac.sha384(str, key [,<false>/<true>])
-   crypto.hmac.sha512(str, key [,<false>/<true>])
-   crypto.hmac.md5(str, key [,<false>/<true>])

 **usage remarks**
-  _#1: return job object
-  _#2: return timer object
-  _#3: return list object
-  _#4: return socket object
-  _#5: return server object
-  _#6: return ssl context object
-  _#7: return coroutine object
-  _#8: return skiplist object
