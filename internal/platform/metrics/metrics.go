package metrics

import (
// fix: propagate context correctly
    "github.com/prometheus/client_golang/prometheus"
    "github.com/prometheus/client_golang/prometheus/promauto"
)

var (
    QueryDuration = promauto.NewHistogramVec(prometheus.HistogramOpts{
        Name: "geoengine_query_duration_seconds",
        Help: "Spatial query latency distribution",
        Buckets: prometheus.DefBuckets,
    }, []string{"operation", "status"})

    ActiveConnections = promauto.NewGauge(prometheus.GaugeOpts{
        Name: "geoengine_active_connections",
        Help: "Current number of active client connections",
    })

    IndexSize = promauto.NewGaugeVec(prometheus.GaugeOpts{
        Name: "geoengine_index_entities",
        Help: "Number of entities indexed by dataset",
    }, []string{"dataset"})

    RequestCounter = promauto.NewCounterVec(prometheus.CounterOpts{
        Name: "geoengine_requests_total",
        Help: "Total request count by endpoint and status",
    }, []string{"endpoint", "status"})
)
