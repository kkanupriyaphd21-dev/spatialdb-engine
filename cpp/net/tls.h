#pragma once
#include <string>
#include <memory>
#include <functional>

namespace spatialdb {
namespace net {

struct TLSConfig {
    std::string cert_file;
    std::string key_file;
    std::string ca_file;
    bool        verify_client  = false;
    bool        verify_server  = true;
    std::string min_version    = "TLS1.2";
    std::string cipher_list    = "HIGH:!aNULL:!MD5:!RC4:!3DES:!DES:!EXPORT";

    // Validate configuration and return error message if invalid
    std::string validate() const;
    bool        isValid() const { return validate().empty(); }
};

struct TLSConn {
    int         raw_fd   = -1;
    void*       ssl      = nullptr; // SSL* — opaque to avoid OpenSSL header dep
    bool        is_server = false;
    std::string peer_cn;
    bool        closed   = false;

    ssize_t read(void* buf, size_t len);
    ssize_t write(const void* buf, size_t len);
    void    close();
};

class TLSContext {
public:
    explicit TLSContext(TLSConfig cfg);
    ~TLSContext();

    bool isValid() const { return ctx_ != nullptr; }

    std::unique_ptr<TLSConn> wrapServer(int fd);
    std::unique_ptr<TLSConn> wrapClient(int fd, const std::string& hostname);

    const TLSConfig& config() const { return cfg_; }

private:
    TLSConfig cfg_;
    void*     ctx_ = nullptr; // SSL_CTX* — opaque

    bool loadCertAndKey();
    bool loadCA();
};

} // namespace net
} // namespace spatialdb
