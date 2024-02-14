#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <memory>
#include "../include/index.h"

namespace spatialdb {
namespace query {

struct CursorState {
    std::string                    id;
    std::vector<index::IndexEntry> remaining;
    size_t                         page_size  = 100;
    std::chrono::steady_clock::time_point expires_at;

    bool expired() const {
        return std::chrono::steady_clock::now() > expires_at;
    }
};

class CursorStore {
public:
    explicit CursorStore(int ttl_seconds = 60);

    std::string store(std::vector<index::IndexEntry> entries, size_t page_size);
    bool        next(const std::string& cursor_id,
                     std::vector<index::IndexEntry>& page_out,
                     bool& has_more);
    void        discard(const std::string& cursor_id);
    void        evictExpired();
    size_t      count() const;

private:
    int         ttl_seconds_;
    mutable std::mutex mu_;
    std::unordered_map<std::string, CursorState> cursors_;
    uint64_t    seq_ = 1;

    std::string generateID();
};

} // namespace query
} // namespace spatialdb
