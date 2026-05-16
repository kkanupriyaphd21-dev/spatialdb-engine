#include "page_cache.h"
#include <stdexcept>

namespace spatialdb {
namespace storage {

PageCache::PageCache(size_t max_pages, size_t page_size)
    : max_pages_(max_pages), page_size_(page_size) {}

PageCache::~PageCache() {
    flushAll();
}

void PageCache::touch(uint64_t page_id) {
    auto it = lru_map_.find(page_id);
    if (it != lru_map_.end()) {
        lru_list_.erase(it->second);
    }
    lru_list_.push_front(page_id);
    lru_map_[page_id] = lru_list_.begin();
}

void PageCache::evict() {
    while (pages_.size() >= max_pages_ && !lru_list_.empty()) {
        uint64_t evict_id = lru_list_.back();
        lru_list_.pop_back();
        lru_map_.erase(evict_id);

        auto pit = pages_.find(evict_id);
        if (pit != pages_.end()) {
            // In production: write dirty pages to disk here
            pages_.erase(pit);
            ++evict_count_;
        }
    }
}

std::optional<std::vector<uint8_t>> PageCache::read(uint64_t page_id) {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = pages_.find(page_id);
    if (it == pages_.end()) {
        ++misses_;
        return std::nullopt;
    }
    ++hits_;
    touch(page_id);
    it->second.last_access = ++tick_;
    return it->second.data;
}

bool PageCache::write(uint64_t page_id, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mu_);

    auto it = pages_.find(page_id);
    if (it == pages_.end()) {
        if (pages_.size() >= max_pages_) evict();
        Page p(page_id, page_size_);
        p.created_at = tick_;
        pages_.emplace(page_id, std::move(p));
        it = pages_.find(page_id);
    }

    if (data.size() > page_size_) return false;

    std::copy(data.begin(), data.end(), it->second.data.begin());
    it->second.dirty       = true;
    it->second.last_access = ++tick_;
    touch(page_id);
    return true;
}

bool PageCache::flush(uint64_t page_id) {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = pages_.find(page_id);
    if (it == pages_.end()) return false;
    it->second.dirty = false;
    return true;
}

void PageCache::flushAll() {
    std::lock_guard<std::mutex> lock(mu_);
    for (auto& [id, page] : pages_) {
        page.dirty = false;
    }
}

void PageCache::invalidate(uint64_t page_id) {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = lru_map_.find(page_id);
    if (it != lru_map_.end()) {
        lru_list_.erase(it->second);
        lru_map_.erase(it);
    }
    pages_.erase(page_id);
}

size_t PageCache::dirtyCount() const {
    std::lock_guard<std::mutex> lock(mu_);
    size_t count = 0;
    for (const auto& [id, page] : pages_) {
        if (page.dirty) ++count;
    }
    return count;
}

} // namespace storage
} // namespace spatialdb
