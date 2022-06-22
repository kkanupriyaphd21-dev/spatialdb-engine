#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include <functional>
#include <cstdint>

namespace spatialdb {
namespace storage {

enum class WalOpType : uint8_t {
    SET    = 1,
    DELETE = 2,
    FLUSH  = 3,
};

struct WalEntry {
    WalOpType   op;
    std::string collection;
    std::string id;
    double      lat;
    double      lon;
    uint64_t    timestamp;
};

class WAL {
public:
    explicit WAL(const std::string& path, size_t sync_every = 100);
    ~WAL();

    bool append(const WalEntry& entry);
    bool flush();
    bool replay(std::function<void(const WalEntry&)> handler);
    bool truncate();
    size_t pendingCount() const { return pending_; }
    std::string path()    const { return path_; }

private:
    std::string    path_;
    std::ofstream  file_;
    std::mutex     mu_;
    size_t         pending_   = 0;
    size_t         sync_every_;

    bool writeEntry(const WalEntry& e);
    bool readEntry(std::ifstream& in, WalEntry& out);
};

} // namespace storage
} // namespace spatialdb
