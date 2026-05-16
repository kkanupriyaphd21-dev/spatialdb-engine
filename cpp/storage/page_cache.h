#pragma once
#include <unordered_map>
#include <list>
#include <vector>
#include <string>
#include <mutex>
#include <cstdint>
#include <optional>

namespace spatialdb {
namespace storage {

static const size_t DEFAULT_PAGE_SIZE  = 4096;
static const size_t DEFAULT_CACHE_PAGES = 1024;

struct Page {
    uint64_t             page_id;
    std::vector<uint8_t> data;
    bool                 dirty = false;
    uint64_t             last_access = 0;
    uint64_t             created_at  = 0; // tick when page was created

    Page(uint64_t id, size_t page_size)
        : page_id(id), data(page_size, 0) {}
};

class PageCache {
public:
    explicit PageCache(size_t max_pages  = DEFAULT_CACHE_PAGES,
                       size_t page_size  = DEFAULT_PAGE_SIZE);
    ~PageCache();

    std::optional<std::vector<uint8_t>> read(uint64_t page_id);
    bool write(uint64_t page_id, const std::vector<uint8_t>& data);
    bool flush(uint64_t page_id);
    void flushAll();
    void invalidate(uint64_t page_id);

    size_t hitCount()  const { return hits_;  }
    size_t missCount() const { return misses_; }
    double hitRate()   const {
        size_t total = hits_ + misses_;
        return total == 0 ? 0.0 : (double)hits_ / total;
    }

    size_t dirtyCount() const;
    size_t evictCount() const { return evict_count_; }

private:
    using LRUList = std::list<uint64_t>;
    using LRUIt   = LRUList::iterator;

    size_t max_pages_;
    size_t page_size_;

    std::unordered_map<uint64_t, Page>   pages_;
    std::unordered_map<uint64_t, LRUIt>  lru_map_;
    LRUList                              lru_list_;
    mutable std::mutex                   mu_;

    size_t hits_   = 0;
    size_t misses_ = 0;
    size_t evict_count_ = 0;
    uint64_t tick_ = 0;

    void evict();
    void touch(uint64_t page_id);
};

} // namespace storage
} // namespace spatialdb
