 **skynet-lua usage**
-   skynet name [...]

 **skynet cluster**
-   skynet cluster.leader [port] [host]

 **skynet http broker**
-   skynet http.broker [port] [host] [ca] [key] [pwd] 

 **global functions**
-   bind(func, [, ...])
-   trace(...)
-   throw(...)
-   wrap(...)
-   unwrap(str)
-   compress(str [,<"deflate">/<"gzip">])
-   uncompress(str [,<"deflate">/<"gzip">])

 **os functions** 
-   os.version()
-   os.pload(name [, ...]) #1
-   os.declare(name, func [, <true>/<false>])
-   os.undeclare(name)
-   os.rpcall([func, ] name [, ...])
-   os.deliver(name, mask, receiver [, ...])
-   os.caller()
-   os.responser()
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
-   os.restart()
-   os.exit()
-   os.stop()
-   os.stopped()
-   os.debugging()

 **coroutine functions**
-   co:close(func)
-   co:dispatch(func)

 **std functions**
-   std.list() #3

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
-   io.socket(<tcp/ssl/ws/wss>, [context]]) #4
-   io.server(<tcp/ssl/ws/wss>, [context]) #5
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

 **storage functions**
-   storage.exist(key)
-   storage.set(key, value [, ...])
-   storage.set_if(key, func, value [, ...])
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
-   crypto.sha1(str)
-   crypto.sha224(str)
-   crypto.sha256(str)
-   crypto.sha384(str)
-   crypto.sha512(str)
-   crypto.md5(str)
-   crypto.hmac.sha1(str, key)
-   crypto.hmac.sha224(str, key)
-   crypto.hmac.sha256(str, key)
-   crypto.hmac.sha384(str, key)
-   crypto.hmac.sha512(str, key)
-   crypto.hmac.md5(str, key)

 **usage remarks**
-  _#1: return job object
-  _#2: return timer object
-  _#3: return list object
-  _#4: return socket object
-  _#5: return server object
-  _#6: return ssl context object
-  _#7: return coroutine object
