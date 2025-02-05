#pragma once
#include <string>
#include <unordered_map>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <chrono>

namespace spatialdb {
namespace storage {

using ExpiryCallback = std::function<void(const std::string& collection,
                                           const std::string& id)>;

class TTLManager {
public:
    explicit TTLManager(int sweep_interval_ms = 1000);
    ~TTLManager();

    void   setTTL(const std::string& collection,
                  const std::string& id,
                  uint64_t ttl_ms);
    void   clearTTL(const std::string& collection, const std::string& id);
    bool   isExpired(const std::string& collection, const std::string& id) const;
    uint64_t remainingMs(const std::string& collection, const std::string& id) const;

    void   onExpiry(ExpiryCallback cb);
    void   start();
    void   stop();

    size_t pendingCount() const;

private:
    using TimePoint = std::chrono::steady_clock::time_point;
    using Key       = std::pair<std::string, std::string>; // {collection, id}

    struct KeyHash {
        size_t operator()(const Key& k) const {
            return std::hash<std::string>{}(k.first + ":" + k.second);
        }
    };

    int                  sweep_interval_ms_;
    std::atomic<bool>    running_{false};
    std::thread          sweep_thread_;
    mutable std::mutex   mu_;

    std::unordered_map<Key, TimePoint, KeyHash> expiry_map_;
    std::multimap<TimePoint, Key>               expiry_index_;
    std::vector<ExpiryCallback>                 callbacks_;

    void sweepLoop();
    void fireCallbacks(const std::string& collection, const std::string& id);
};

} // namespace storage
} // namespace spatialdb
