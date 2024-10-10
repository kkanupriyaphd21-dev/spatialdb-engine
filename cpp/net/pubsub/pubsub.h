#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <mutex>
#include <memory>

namespace spatialdb {
namespace net {
namespace pubsub {

using MessageHandler = std::function<void(const std::string& channel,
                                           const std::string& message)>;

struct Subscriber {
    uint64_t              id;
    MessageHandler        handler;
    std::unordered_set<std::string> channels;
    std::unordered_set<std::string> patterns;
};

class PubSubBroker {
public:
    PubSubBroker();
    ~PubSubBroker();

    uint64_t subscribe(const std::string& channel, MessageHandler handler);
    uint64_t psubscribe(const std::string& pattern, MessageHandler handler);
    void     unsubscribe(uint64_t sub_id, const std::string& channel = "");
    void     punsubscribe(uint64_t sub_id, const std::string& pattern = "");

    int  publish(const std::string& channel, const std::string& message);
    bool hasSubscribers(const std::string& channel) const;

    size_t subscriberCount(const std::string& channel = "") const;
    std::vector<std::string> activeChannels() const;

private:
    mutable std::mutex mu_;
    uint64_t           next_id_ = 1;

    std::unordered_map<std::string, std::unordered_set<uint64_t>> channel_subs_;
    std::unordered_map<std::string, std::unordered_set<uint64_t>> pattern_subs_;
    std::unordered_map<uint64_t, Subscriber>                       subscribers_;

    bool matchPattern(const std::string& pattern, const std::string& channel) const;
};

} // namespace pubsub
} // namespace net
} // namespace spatialdb
