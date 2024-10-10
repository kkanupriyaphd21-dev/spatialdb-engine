#include "pubsub.h"
#include <fnmatch.h>

namespace spatialdb {
namespace net {
namespace pubsub {

PubSubBroker::PubSubBroker() {}
PubSubBroker::~PubSubBroker() {}

bool PubSubBroker::matchPattern(const std::string& pattern,
                                 const std::string& channel) const {
    return fnmatch(pattern.c_str(), channel.c_str(), 0) == 0;
}

uint64_t PubSubBroker::subscribe(const std::string& channel, MessageHandler handler) {
    std::lock_guard<std::mutex> lock(mu_);
    uint64_t id = next_id_++;
    auto& sub      = subscribers_[id];
    sub.id         = id;
    sub.handler    = std::move(handler);
    sub.channels.insert(channel);
    channel_subs_[channel].insert(id);
    return id;
}

uint64_t PubSubBroker::psubscribe(const std::string& pattern, MessageHandler handler) {
    std::lock_guard<std::mutex> lock(mu_);
    uint64_t id = next_id_++;
    auto& sub      = subscribers_[id];
    sub.id         = id;
    sub.handler    = std::move(handler);
    sub.patterns.insert(pattern);
    pattern_subs_[pattern].insert(id);
    return id;
}

void PubSubBroker::unsubscribe(uint64_t sub_id, const std::string& channel) {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = subscribers_.find(sub_id);
    if (it == subscribers_.end()) return;

    if (channel.empty()) {
        for (const auto& ch : it->second.channels)
            channel_subs_[ch].erase(sub_id);
        subscribers_.erase(it);
    } else {
        it->second.channels.erase(channel);
        channel_subs_[channel].erase(sub_id);
        if (it->second.channels.empty() && it->second.patterns.empty())
            subscribers_.erase(it);
    }
}

void PubSubBroker::punsubscribe(uint64_t sub_id, const std::string& pattern) {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = subscribers_.find(sub_id);
    if (it == subscribers_.end()) return;

    if (pattern.empty()) {
        for (const auto& p : it->second.patterns)
            pattern_subs_[p].erase(sub_id);
        subscribers_.erase(it);
    } else {
        it->second.patterns.erase(pattern);
        pattern_subs_[pattern].erase(sub_id);
    }
}

int PubSubBroker::publish(const std::string& channel, const std::string& message) {
    std::vector<MessageHandler> handlers;
    {
        std::lock_guard<std::mutex> lock(mu_);

        auto it = channel_subs_.find(channel);
        if (it != channel_subs_.end()) {
            for (uint64_t id : it->second) {
                auto sit = subscribers_.find(id);
                if (sit != subscribers_.end()) handlers.push_back(sit->second.handler);
            }
        }

        for (const auto& [pattern, ids] : pattern_subs_) {
            if (matchPattern(pattern, channel)) {
                for (uint64_t id : ids) {
                    auto sit = subscribers_.find(id);
                    if (sit != subscribers_.end()) handlers.push_back(sit->second.handler);
                }
            }
        }
    }

    for (auto& h : handlers) h(channel, message);
    return (int)handlers.size();
}

bool PubSubBroker::hasSubscribers(const std::string& channel) const {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = channel_subs_.find(channel);
    return it != channel_subs_.end() && !it->second.empty();
}

size_t PubSubBroker::subscriberCount(const std::string& channel) const {
    std::lock_guard<std::mutex> lock(mu_);
    if (channel.empty()) return subscribers_.size();
    auto it = channel_subs_.find(channel);
    return it == channel_subs_.end() ? 0 : it->second.size();
}

std::vector<std::string> PubSubBroker::activeChannels() const {
    std::lock_guard<std::mutex> lock(mu_);
    std::vector<std::string> out;
    for (const auto& [ch, ids] : channel_subs_) {
        if (!ids.empty()) out.push_back(ch);
    }
    return out;
}

} // namespace pubsub
} // namespace net
} // namespace spatialdb
