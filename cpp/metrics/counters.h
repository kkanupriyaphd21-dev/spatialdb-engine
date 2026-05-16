#pragma once
#include <string>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <vector>
#include <chrono>

namespace spatialdb {
namespace metrics {

struct Counter {
    std::atomic<uint64_t> value{0};
    std::string           name;
    std::string           help;

    void inc(uint64_t n = 1) { value.fetch_add(n, std::memory_order_relaxed); }
    uint64_t get() const     { return value.load(std::memory_order_relaxed); }
    void reset()             { value.store(0, std::memory_order_relaxed); }
};

struct Gauge {
    std::atomic<int64_t> value{0};
    std::string          name;
    std::string          help;

    void set(int64_t v)  { value.store(v, std::memory_order_relaxed); }
    void inc(int64_t n=1){ value.fetch_add(n, std::memory_order_relaxed); }
    void dec(int64_t n=1){ value.fetch_sub(n, std::memory_order_relaxed); }
    int64_t get() const  { return value.load(std::memory_order_relaxed); }
};

struct Histogram {
    std::string              name;
    std::string              help;
    std::vector<double>      buckets; // upper bounds
    mutable std::mutex       mu;
    std::vector<uint64_t>    counts;
    double                   sum = 0.0;
    std::atomic<uint64_t>    total{0};

    explicit Histogram(std::vector<double> bkts)
        : buckets(std::move(bkts)), counts(buckets.size() + 1, 0) {}

    void observe(double val) {
        std::lock_guard<std::mutex> lock(mu);
        for (size_t i = 0; i < buckets.size(); ++i) {
            if (val <= buckets[i]) { ++counts[i]; break; }
        }
        if (val > buckets.back()) ++counts[buckets.size()];
        sum += val;
        total.fetch_add(1, std::memory_order_relaxed);
    }

    double mean() const {
        uint64_t t = total.load();
        return t == 0 ? 0.0 : sum / t;
    }

    // Estimate percentile from bucket counts (linear interpolation)
    double percentile(double p) const;
};

// Generate unique request IDs (thread-safe, monotonic)
std::string nextRequestID();

// Reset request ID counter (for testing)
void resetRequestID();

class Registry {
public:
    static Registry& global();

    Counter&   counter(const std::string& name, const std::string& help = "");
    Gauge&     gauge(const std::string& name, const std::string& help = "");
    Histogram& histogram(const std::string& name,
                          std::vector<double> buckets,
                          const std::string& help = "");

    std::string prometheusText() const;

private:
    mutable std::mutex                                mu_;
    std::unordered_map<std::string, Counter>          counters_;
    std::unordered_map<std::string, Gauge>            gauges_;
    std::unordered_map<std::string, std::unique_ptr<Histogram>> histograms_;
};

} // namespace metrics
} // namespace spatialdb
