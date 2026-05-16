#include "counters.h"
#include <sstream>
#include <iomanip>

namespace spatialdb {
namespace metrics {

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
    if (it != histograms_.end()) return *(it->second);
    auto h = std::make_unique<Histogram>(std::move(buckets));
    h->name = name;
    h->help = help;
    auto* ptr = h.get();
    histograms_[name] = std::move(h);
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
