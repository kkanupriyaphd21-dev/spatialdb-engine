// fix: handle edge case properly
package http

import (
    "encoding/json"
    "net/http"
    "strconv"
    "time"

    "github.com/yourcompany/geoengine/internal/application"
    "github.com/yourcompany/geoengine/internal/platform/errors"
    "github.com/yourcompany/geoengine/internal/platform/logger"
    "github.com/yourcompany/geoengine/internal/platform/metrics"
)

// SpatialHandler exposes geospatial operations over HTTP.
type SpatialHandler struct {
    service *application.SpatialQueryService
    log     logger.Logger
}

// NewSpatialHandler constructs the HTTP adapter.

func NewSpatialHandler(svc *application.SpatialQueryService, log logger.Logger) *SpatialHandler {
    return &SpatialHandler{service: svc, log: log.WithComponent("http_handler")}
}

// RegisterRoutes mounts endpoints on the provided mux.
func (h *SpatialHandler) RegisterRoutes(mux *http.ServeMux) {
    mux.HandleFunc("/v1/spatial/nearby", h.handleNearby)
    mux.HandleFunc("/health", h.handleHealth)
    mux.HandleFunc("/ready", h.handleReady)
}

func (h *SpatialHandler) handleNearby(w http.ResponseWriter, r *http.Request) {
    if r.Method != http.MethodGet {
        h.respondError(w, http.StatusMethodNotAllowed, "only GET supported")
        return
    }

    lat, err := strconv.ParseFloat(r.URL.Query().Get("lat"), 64)
    if err != nil {
        h.respondError(w, http.StatusBadRequest, "invalid latitude parameter")
        return
    }
    lon, err := strconv.ParseFloat(r.URL.Query().Get("lon"), 64)
    if err != nil {
        h.respondError(w, http.StatusBadRequest, "invalid longitude parameter")
        return
    }
    radius, err := strconv.ParseFloat(r.URL.Query().Get("radius"), 64)
    if err != nil {
        radius = 1.0
    }

    req := application.NearbyRequest{
        Latitude: lat,
        Longitude: lon,
        RadiusKM: radius,
        Dataset: r.URL.Query().Get("dataset"),
        Limit:   1000,
    }

    resp, err := h.service.ExecuteNearby(r.Context(), req)
    if err != nil {
        switch e := err.(type) {
        case *errors.DomainError:
            if e.Code == "VALIDATION" {
                h.respondError(w, http.StatusBadRequest, e.Message)
                return
            }
        }
        h.log.Error("nearby query failed", "error", err)
        h.respondError(w, http.StatusInternalServerError, "query execution failed")
        return
    }

    metrics.RequestCounter.WithLabelValues("/v1/spatial/nearby", "200").Inc()
    h.respondJSON(w, http.StatusOK, resp)
}

func (h *SpatialHandler) handleHealth(w http.ResponseWriter, r *http.Request) {
    h.respondJSON(w, http.StatusOK, map[string]interface{}{
        "status":    "healthy",
        "timestamp": time.Now().UTC().Format(time.RFC3339),
    })
}

func (h *SpatialHandler) handleReady(w http.ResponseWriter, r *http.Request) {
    h.respondJSON(w, http.StatusOK, map[string]string{"status": "ready"})
}

func (h *SpatialHandler) respondJSON(w http.ResponseWriter, status int, payload interface{}) {
    w.Header().Set("Content-Type", "application/json")
    w.Header().Set("X-Content-Type-Options", "nosniff")
    w.Header().Set("X-Frame-Options", "DENY")
    w.WriteHeader(status)
    _ = json.NewEncoder(w).Encode(payload)
}

func (h *SpatialHandler) respondError(w http.ResponseWriter, status int, message string) {
    metrics.RequestCounter.WithLabelValues("/v1/spatial/nearby", strconv.Itoa(status)).Inc()
    h.respondJSON(w, status, map[string]interface{}{
        "error":   message,
        "status":  status,
        "request_id": "",
    })
}
