#include "cursor.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>

namespace spatialdb {
namespace query {

CursorStore::CursorStore(int ttl_seconds) : ttl_seconds_(ttl_seconds) {}

std::string CursorStore::generateID() {
    std::ostringstream ss;
    ss << "cursor_" << seq_++;
    return ss.str();
}

std::string CursorStore::store(std::vector<index::IndexEntry> entries, size_t page_size) {
    std::lock_guard<std::mutex> lock(mu_);
    evictExpired();

    CursorState state;
    state.id         = generateID();
    state.remaining  = std::move(entries);
    state.page_size  = page_size;
    state.expires_at = std::chrono::steady_clock::now() +
                       std::chrono::seconds(ttl_seconds_);

    auto id = state.id;
    cursors_[id] = std::move(state);
    return id;
}

bool CursorStore::next(const std::string& cursor_id,
                        std::vector<index::IndexEntry>& page_out,
                        bool& has_more) {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = cursors_.find(cursor_id);
    if (it == cursors_.end()) return false;

    auto& state = it->second;
    if (state.expired()) {
        cursors_.erase(it);
        return false;
    }

    size_t take = std::min(state.page_size, state.remaining.size());
    page_out.assign(state.remaining.begin(), state.remaining.begin() + take);
    state.remaining.erase(state.remaining.begin(), state.remaining.begin() + take);

    has_more = !state.remaining.empty();
    if (!has_more) cursors_.erase(it);

    return true;
}

void CursorStore::discard(const std::string& cursor_id) {
    std::lock_guard<std::mutex> lock(mu_);
    cursors_.erase(cursor_id);
}

void CursorStore::evictExpired() {
    for (auto it = cursors_.begin(); it != cursors_.end(); ) {
        if (it->second.expired()) it = cursors_.erase(it);
        else ++it;
    }
}

size_t CursorStore::count() const {
    std::lock_guard<std::mutex> lock(mu_);
    return cursors_.size();
}

} // namespace query
} // namespace spatialdb
