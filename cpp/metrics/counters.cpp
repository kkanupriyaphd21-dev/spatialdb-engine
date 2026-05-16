#include "counters.h"
#include <sstream>
#include <iomanip>
#include <atomic>
#include <cstdio>

namespace spatialdb {
namespace metrics {

// Request ID generation
static std::atomic<uint64_t> request_id_counter{0};

std::string nextRequestID() {
    uint64_t id = request_id_counter.fetch_add(1, std::memory_order_relaxed);
    char buf[24];
    snprintf(buf, sizeof(buf), "%016lx", (unsigned long)id);
    return std::string(buf);
}

void resetRequestID() {
    request_id_counter.store(0, std::memory_order_relaxed);
}

// Histogram percentile estimation from bucket counts
double Histogram::percentile(double p) const {
    std::lock_guard<std::mutex> lock(mu);
    if (total.load() == 0) return 0.0;

    uint64_t target = (uint64_t)(p / 100.0 * total.load());
    uint64_t cumulative = 0;

    for (size_t i = 0; i < counts.size(); ++i) {
        cumulative += counts[i];
        if (cumulative >= target) {
            if (i < buckets.size()) return buckets[i];
            return buckets.empty() ? sum : buckets.back() * 2; // overflow bucket
        }
    }
    return buckets.empty() ? sum : buckets.back();
}

Registry& Registry::global() {
    static Registry instance;
    return instance;
}

Counter& Registry::counter(const std::string& name, const std::string& help) {
    std::lock_guard<std::mutex> lock(mu_);
    auto& c = counters_[name];
    c.name = name;
    c.help = help;
    return c;
}

Gauge& Registry::gauge(const std::string& name, const std::string& help) {
    std::lock_guard<std::mutex> lock(mu_);
    auto& g = gauges_[name];
    g.name = name;
    g.help = help;
    return g;
}

Histogram& Registry::histogram(const std::string& name,
                                  std::vector<double> buckets,
                                  const std::string& help) {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = histograms_.find(name);
    if (it != histograms_.end()) return *it->second;
    auto h = std::make_unique<Histogram>(std::move(buckets));
    h->name = name;
    h->help = help;
    auto* ptr = h.get();
    histograms_.emplace(name, std::move(h));
    return *ptr;
}

std::string Registry::prometheusText() const {
    std::lock_guard<std::mutex> lock(mu_);
    std::ostringstream ss;

    for (const auto& [name, c] : counters_) {
        if (!c.help.empty())
            ss << "# HELP " << name << " " << c.help << "\n";
        ss << "# TYPE " << name << " counter\n";
        ss << name << " " << c.get() << "\n";
    }

    for (const auto& [name, g] : gauges_) {
        if (!g.help.empty())
            ss << "# HELP " << name << " " << g.help << "\n";
        ss << "# TYPE " << name << " gauge\n";
        ss << name << " " << g.get() << "\n";
    }

    for (const auto& [name, h_ptr] : histograms_) {
        const auto& h = *h_ptr;
        if (!h.help.empty())
            ss << "# HELP " << name << " " << h.help << "\n";
        ss << "# TYPE " << name << " histogram\n";
        std::lock_guard<std::mutex> hlock(h.mu);
        for (size_t i = 0; i < h.buckets.size(); ++i) {
            ss << name << "_bucket{le=\"" << h.buckets[i] << "\"} "
               << h.counts[i] << "\n";
        }
        ss << name << "_bucket{le=\"+Inf\"} " << h.total.load() << "\n";
        ss << name << "_sum "   << h.sum   << "\n";
        ss << name << "_count " << h.total.load() << "\n";
    }

    return ss.str();
}

} // namespace metrics
} // namespace spatialdb
