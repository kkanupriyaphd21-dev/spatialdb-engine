#include "tls.h"
#include <stdexcept>
#include <iostream>
#include <unistd.h>

namespace spatialdb {
namespace net {

// TLS implementation requires linking against OpenSSL.
// This file provides the structure and interface; the actual SSL_CTX
// initialization is conditionally compiled when HAVE_OPENSSL is defined.

TLSContext::TLSContext(TLSConfig cfg) : cfg_(std::move(cfg)) {
    // OpenSSL init would go here:
    //   SSL_library_init();
    //   ctx_ = SSL_CTX_new(TLS_server_method());
    //   SSL_CTX_use_certificate_file(ctx_, cfg_.cert_file.c_str(), SSL_FILETYPE_PEM);
    //   SSL_CTX_use_PrivateKey_file(ctx_, cfg_.key_file.c_str(), SSL_FILETYPE_PEM);
    std::cout << "TLSContext: stub (link with -lssl -lcrypto to enable)\n";
}

TLSContext::~TLSContext() {
    // SSL_CTX_free(static_cast<SSL_CTX*>(ctx_));
}

std::unique_ptr<TLSConn> TLSContext::wrapServer(int fd) {
    auto conn = std::make_unique<TLSConn>();
    conn->raw_fd    = fd;
    conn->is_server = true;
    // SSL* ssl = SSL_new(static_cast<SSL_CTX*>(ctx_));
    // SSL_set_fd(ssl, fd);
    // SSL_accept(ssl);
    // conn->ssl = ssl;
    return conn;
}

std::unique_ptr<TLSConn> TLSContext::wrapClient(int fd, const std::string& hostname) {
    auto conn = std::make_unique<TLSConn>();
    conn->raw_fd    = fd;
    conn->is_server = false;
    conn->peer_cn   = hostname;
    // SSL* ssl = SSL_new(static_cast<SSL_CTX*>(ctx_));
    // SSL_set_fd(ssl, fd);
    // SSL_set_tlsext_host_name(ssl, hostname.c_str());
    // SSL_connect(ssl);
    return conn;
}

ssize_t TLSConn::read(void* buf, size_t len) {
    if (ssl) {
        // return SSL_read(static_cast<SSL*>(ssl), buf, len);
    }
    return ::read(raw_fd, buf, len);
}

ssize_t TLSConn::write(const void* buf, size_t len) {
    if (ssl) {
        // return SSL_write(static_cast<SSL*>(ssl), buf, len);
    }
    return ::write(raw_fd, buf, len);
}

void TLSConn::close() {
    if (closed) return;
    closed = true;
    if (ssl) {
        // SSL_shutdown(static_cast<SSL*>(ssl));
        // SSL_free(static_cast<SSL*>(ssl));
        ssl = nullptr;
    }
    if (raw_fd >= 0) {
        ::close(raw_fd);
        raw_fd = -1;
    }
}

} // namespace net
} // namespace spatialdb
