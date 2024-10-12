
###################################################################

CCOMPILE   := gcc
CPPCOMPILE := g++
LINK       := g++

CONFIG := ./config
include $(CONFIG)
PREFIXPATH = $(PREFIX)/bin

######################### OPTIONS BEGIN ###########################
#target file

TARGET  = skynet
VERSION = 1.30.2
OUTPUT  = ./bin/$(TARGET)
MAKE    = make
MKDIR   = mkdir -p
RM      = rm -f
INSTALLPATH = $(PREFIXPATH)

#source files
SOURCE  := src/core/lua_bind.o \
           src/core/lua_core.o \
		   src/core/lua_dofile.o \
		   src/core/lua_path.o \
		   src/core/lua_pcall.o \
		   src/core/lua_pload.o \
		   src/core/lua_print.o \
		   src/core/lua_require.o \
		   src/core/lua_rpcall.o \
		   src/core/lua_socket.o \
		   src/core/lua_format.o \
		   src/core/lua_timer.o \
		   src/core/lua_sheet.o \
		   src/core/lua_wrap.o \
		   src/core/lua_global.o \
		   src/extend/http/parser.o \
		   src/extend/rapidjson/document.o \
		   src/extend/rapidjson/rapidjson.o \
		   src/extend/skiplist/skiplist.o \
		   src/extend/rapidjson/schema.o \
		   src/extend/rapidjson/values.o \
		   src/extend/lua_compile.o \
		   src/extend/lua_deflate.o \
		   src/extend/lua_directory.o \
		   src/extend/lua_http.o \
		   src/extend/lua_json.o \
		   src/extend/lua_list.o \
		   src/extend/lua_skiplist.o \
		   src/extend/lua_openssl.o \
		   src/extend/lua_storage.o \
		   src/extend/lua_string.o \
           src/skynet.o \
           src/skynet_allotor.o \
		   src/skynet_lua.o \
		   src/skynet_main.o \
		   src/skynet_profiler.o
		   
#library path
LIBDIRS := -L$(LUA_LIBDIR)

#include path
INCDIRS := $(LUA_INCLUDE) -I./include -I./include/asio -I./include/stdnet

#precompile macro
CC_FLAG := -DSTDNET_USE_OPENSSL -DSTDNET_USE_DEFLATE

#compile options
COMPILEOPTION := -std=c++11 -fPIC -w -Wfatal-errors -O2

#rpath options
LIBLOADPATH := -Wl,-rpath=./ -Wl,-rpath=./lib -Wl,-rpath=L$(LUA_LIBDIR)

#link options
LINKOPTION := -o $(OUTPUT) $(LIBLOADPATH)

#dependency librarys
LIBS := -llua -lz -lcrypto -lssl -ldl -lpthread

########################## OPTIONS END ############################

$(OUTPUT): $(SOURCE)
	$(LINK) $(LINKOPTION) $(LIBDIRS) $(SOURCE) $(LIBS)

clean: 
	$(RM) $(SOURCE)
	$(RM) $(OUTPUT)
	
install:
	$(RM) $(INSTALLPATH)/$(TARGET)-$(VERSION)
	cp $(OUTPUT) $(INSTALLPATH)/$(TARGET)-$(VERSION)
	
	$(RM) -r $(INSTALLPATH)/lua
	cp -r ./lua $(INSTALLPATH)
	
	$(RM) $(PREFIXPATH)/$(TARGET)
	ln -s $(TARGET)-$(VERSION) $(PREFIXPATH)/$(TARGET)
	
all: clean $(OUTPUT)
.PRECIOUS:%.cpp %.c %.C
.SUFFIXES:
.SUFFIXES:  .c .o .cpp .ecpp .pc .ec .C .cc .cxx

.cpp.o:
	$(CPPCOMPILE) -c -o $*.o $(CC_FLAG) $(COMPILEOPTION) $(INCDIRS)  $*.cpp
	
.cc.o:
	$(CCOMPILE) -c -o $*.o $(CC_FLAG) $(COMPILEOPTION) $(INCDIRS)  $*.cx

.cxx.o:
	$(CPPCOMPILE) -c -o $*.o $(CC_FLAG) $(COMPILEOPTION) $(INCDIRS)  $*.cxx

.c.o:
	$(CCOMPILE) -c -o $*.o $(CC_FLAG) $(COMPILEOPTION) $(INCDIRS) $*.c

.C.o:
	$(CPPCOMPILE) -c -o $*.o $(CC_FLAG) $(COMPILEOPTION) $(INCDIRS) $*.C

########################## MAKEFILE END ############################
