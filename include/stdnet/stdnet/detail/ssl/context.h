

#ifndef STDNET_SSL_CONTEXT_H
#define STDNET_SSL_CONTEXT_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "stdnet/detail/service.h"

#if !defined(STDNET_USE_OPENSSL)

#define error_if_nossl error::service_not_found

/***********************************************************************************/

namespace ssl {
  class context {
  public:
    enum {
      default_workarounds,
      single_dh_use,
      no_sslv2,
      no_sslv3
    };
    enum file_format {
      asn1,
      pem
    };
    enum method {
      sslv2,
      sslv2_client,
      sslv2_server,
      sslv3,
      sslv3_client,
      sslv3_server,
      tlsv1,
      tlsv1_client,
      tlsv1_server,
      sslv23,
      sslv23_client,
      sslv23_server,
      tlsv11,
      tlsv11_client,
      tlsv11_server,
      tlsv12,
      tlsv12_client,
      tlsv12_server,
      tlsv13,
      tlsv13_client,
      tlsv13_server,
      tls,
      tls_client,
      tls_server
    };
    struct password_purpose {};
  public:
    typedef int verify_mode;
    typedef uint64_t options;
    typedef void* native_handle_type;
    ASIO_DECL explicit context(method m) {}
    ASIO_DECL explicit context(native_handle_type native_handle) {}
    ASIO_DECL context(context&& other) {}
    ASIO_DECL context& operator=(context&& other) { return *this; }
    ASIO_DECL ~context() {};
    ASIO_DECL native_handle_type native_handle() { return nullptr; }
    ASIO_DECL void clear_options(options o) {}
    ASIO_DECL ASIO_SYNC_OP_VOID clear_options(options o, error_code& ec) {}
    ASIO_DECL void set_options(options o) {}
    ASIO_DECL ASIO_SYNC_OP_VOID set_options(options o, error_code& ec) { ec = error_if_nossl; }
    ASIO_DECL void set_verify_mode(verify_mode v) {}
    ASIO_DECL ASIO_SYNC_OP_VOID set_verify_mode(verify_mode v, error_code& ec) { ec = error_if_nossl; }
    ASIO_DECL void set_verify_depth(int depth) {}
    ASIO_DECL ASIO_SYNC_OP_VOID set_verify_depth(int depth, error_code& ec) { ec = error_if_nossl; }

  public:
    template <typename VerifyCallback>
    void set_verify_callback(VerifyCallback callback) {}
    template <typename VerifyCallback>
    ASIO_SYNC_OP_VOID set_verify_callback(VerifyCallback callback, error_code& ec) { ec = error_if_nossl; }
    ASIO_DECL void load_verify_file(const std::string& filename) {}
    ASIO_DECL ASIO_SYNC_OP_VOID load_verify_file(const std::string& filename, error_code& ec) { ec = error_if_nossl; }
    ASIO_DECL void add_certificate_authority(const const_buffer& ca) {}
    ASIO_DECL ASIO_SYNC_OP_VOID add_certificate_authority(const const_buffer& ca, error_code& ec) { ec = error_if_nossl; }
    ASIO_DECL void set_default_verify_paths() {}
    ASIO_DECL ASIO_SYNC_OP_VOID set_default_verify_paths(error_code& ec) { ec = error_if_nossl; }
    ASIO_DECL void add_verify_path(const std::string& path) {}
    ASIO_DECL ASIO_SYNC_OP_VOID add_verify_path(const std::string& path, error_code& ec) { ec = error_if_nossl; }
    ASIO_DECL void use_certificate(const const_buffer& certificate, file_format format) {}
    ASIO_DECL ASIO_SYNC_OP_VOID use_certificate(const const_buffer& certificate, file_format format, error_code& ec) { ec = error_if_nossl; }
    ASIO_DECL void use_certificate_file(const std::string& filename, file_format format) {}
    ASIO_DECL ASIO_SYNC_OP_VOID use_certificate_file(const std::string& filename, file_format format, error_code& ec) { ec = error_if_nossl; }
    ASIO_DECL void use_certificate_chain(const const_buffer& chain) {}
    ASIO_DECL ASIO_SYNC_OP_VOID use_certificate_chain(const const_buffer& chain, error_code& ec) { ec = error_if_nossl; }
    ASIO_DECL void use_certificate_chain_file(const std::string& filename) {}
    ASIO_DECL ASIO_SYNC_OP_VOID use_certificate_chain_file(const std::string& filename, error_code& ec) { ec = error_if_nossl; }
    ASIO_DECL void use_private_key(const const_buffer& private_key, file_format format) {}
    ASIO_DECL ASIO_SYNC_OP_VOID use_private_key(const const_buffer& private_key, file_format format, error_code& ec) { ec = error_if_nossl; }
    ASIO_DECL void use_private_key_file(const std::string& filename, file_format format) {}
    ASIO_DECL ASIO_SYNC_OP_VOID use_private_key_file(const std::string& filename, file_format format, error_code& ec) { ec = error_if_nossl; }
    ASIO_DECL void use_rsa_private_key(const const_buffer& private_key, file_format format) {}
    ASIO_DECL ASIO_SYNC_OP_VOID use_rsa_private_key(const const_buffer& private_key, file_format format, error_code& ec) { ec = error_if_nossl; }
    ASIO_DECL void use_rsa_private_key_file(const std::string& filename, file_format format) {}
    ASIO_DECL ASIO_SYNC_OP_VOID use_rsa_private_key_file(const std::string& filename, file_format format, error_code& ec) { ec = error_if_nossl; }
    ASIO_DECL void use_tmp_dh(const const_buffer& dh) {}
    ASIO_DECL ASIO_SYNC_OP_VOID use_tmp_dh(const const_buffer& dh, error_code& ec) { ec = error_if_nossl; }
    ASIO_DECL void use_tmp_dh_file(const std::string& filename) {}
    ASIO_DECL ASIO_SYNC_OP_VOID use_tmp_dh_file(const std::string& filename, error_code& ec) { ec = error_if_nossl; }
    template <typename PasswordCallback>
    ASIO_SYNC_OP_VOID set_password_callback(PasswordCallback callback) {}
    template <typename PasswordCallback>
    ASIO_SYNC_OP_VOID set_password_callback(PasswordCallback callback, error_code& ec) { ec = error_if_nossl; }
  };
}

#endif //STDNET_USE_OPENSSL

/***********************************************************************************/

ASIO_DECL void init_ssl_context(
  ssl::context& ctx, error_code& ec) {
  ctx.set_options(ssl::context::default_workarounds
    | ssl::context::no_sslv2
    | ssl::context::no_sslv3
    | ssl::context::single_dh_use, ec
  );
}

ASIO_DECL void use_certificate_file(
  ssl::context& ctx, const char* filename, error_code& ec) {
  ctx.use_certificate_chain_file(filename, ec);
}

ASIO_DECL void use_certificate_chain(
  ssl::context& ctx, const char* cert, size_t size, error_code& ec) {
  ctx.use_certificate_chain(buffer(cert, size), ec);
}

ASIO_DECL void use_private_key_pwd(
  ssl::context& ctx, const char* password, error_code& ec) {
  std::string passwd(password);
  ctx.set_password_callback(
    [passwd](size_t, ssl::context::password_purpose) {
      return passwd;
    }, ec
  );
}

ASIO_DECL void use_private_key(
  ssl::context& ctx, const char* key, size_t size, error_code& ec) {
  ctx.use_private_key(buffer(key, size), ssl::context::pem, ec);
}

ASIO_DECL void use_private_key_file(
  ssl::context& ctx, const char* filename, error_code& ec) {
  ctx.use_private_key_file(filename, ssl::context::pem, ec);
}

/***********************************************************************************/

#endif //STDNET_SSL_CONTEXT_H
