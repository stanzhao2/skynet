

#ifndef __LUA_OPENSSL_H
#define __LUA_OPENSSL_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "../skynet_lua.h"

/*
** crypto.aes_encrypt(data, key);
** crypto.aes_decrypt(data, key);
** crypto.rsa_sign(data, private-key);
** crypto.rsa_verify(data, sign, public-key);
** crypto.rsa_encrypt(data, public-key/private-key);
** crypto.rsa_decrypt(data, public-key/private-key);
** crypto.hash32(data);
** crypto.sha1(data[, raw]);
** crypto.hmac_sha1(data, key[, raw]);
** crypto.sha224(data[, raw]);
** crypto.hmac_sha224(data, key[, raw]);
** crypto.sha256(data[, raw]);
** crypto.hmac_sha256(data, key[, raw]);
** crypto.sha384(data[, raw]);
** crypto.hmac_sha384(data, key[, raw]);
** crypto.sha512(data[, raw]);
** crypto.hmac_sha512(data, key[, raw]);
** crypto.md5(data[, raw]);
** crypto.hmac_md5(data, key[, raw]);
**
** base64.encode(data);
** base64.decode(data);
*/

/********************************************************************************/

SKYNET_API int luaopen_base64(lua_State* L);
SKYNET_API int luaopen_crypto(lua_State* L);

/********************************************************************************/

#endif //__LUA_OPENSSL_H
