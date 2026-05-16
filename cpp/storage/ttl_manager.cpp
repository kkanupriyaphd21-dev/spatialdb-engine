#include "ttl_manager.h"
#include <algorithm>

namespace spatialdb {
namespace storage {

TTLManager::TTLManager(int sweep_interval_ms)
    : sweep_interval_ms_(sweep_interval_ms) {}

TTLManager::~TTLManager() { stop(); }

void TTLManager::setTTL(const std::string& collection,
                         const std::string& id,
                         uint64_t ttl_ms) {
    std::lock_guard<std::mutex> lock(mu_);
    Key key{collection, id};
    auto expires = std::chrono::steady_clock::now() +
                   std::chrono::milliseconds(ttl_ms);

    // remove old entry from index if exists
    auto it = expiry_map_.find(key);
    if (it != expiry_map_.end()) {
        auto range = expiry_index_.equal_range(it->second);
        for (auto r = range.first; r != range.second; ) {
            if (r->second == key) {
                r = expiry_index_.erase(r);
                break;
            } else {
                ++r;
            }
        }
    }

    expiry_map_[key] = expires;
    expiry_index_.emplace(expires, key);
}

void TTLManager::clearTTL(const std::string& collection, const std::string& id) {
    std::lock_guard<std::mutex> lock(mu_);
    Key key{collection, id};
    auto it = expiry_map_.find(key);
    if (it == expiry_map_.end()) return;

    auto range = expiry_index_.equal_range(it->second);
    for (auto r = range.first; r != range.second; ++r) {
        if (r->second == key) { expiry_index_.erase(r); break; }
    }
    expiry_map_.erase(it);
}

bool TTLManager::isExpired(const std::string& collection, const std::string& id) const {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = expiry_map_.find({collection, id});
    if (it == expiry_map_.end()) return false;
    return std::chrono::steady_clock::now() >= it->second;
}

uint64_t TTLManager::remainingMs(const std::string& collection, const std::string& id) const {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = expiry_map_.find({collection, id});
    if (it == expiry_map_.end()) return 0;
    auto now = std::chrono::steady_clock::now();
    if (now >= it->second) return 0;
    return std::chrono::duration_cast<std::chrono::milliseconds>(it->second - now).count();
}

void TTLManager::sweepLoop() {
    while (running_) {
        auto now = std::chrono::steady_clock::now();
        std::vector<Key> expired;

        {
            std::lock_guard<std::mutex> lock(mu_);
            auto it = expiry_index_.begin();
            while (it != expiry_index_.end() && it->first <= now) {
                expired.push_back(it->second);
                expiry_map_.erase(it->second);
                it = expiry_index_.erase(it);
            }
        }

        std::vector<ExpiryCallback> callbacks_to_fire;
        {
            std::lock_guard<std::mutex> lock(mu_);
            callbacks_to_fire = callbacks_;
        }

        for (const auto& [col, id] : expired) {
            for (const auto& cb : callbacks_to_fire) {
                cb(col, id);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(sweep_interval_ms_));
    }
}

void TTLManager::start() {
    running_ = true;
    sweep_thread_ = std::thread([this]() { sweepLoop(); });
}

void TTLManager::stop() {
    running_ = false;
    if (sweep_thread_.joinable()) sweep_thread_.join();
}

void TTLManager::onExpiry(ExpiryCallback cb) {
    std::lock_guard<std::mutex> lock(mu_);
    callbacks_.push_back(std::move(cb));
}

size_t TTLManager::pendingCount() const {
    std::lock_guard<std::mutex> lock(mu_);
    return expiry_map_.size();
}

} // namespace storage
} // namespace spatialdb
