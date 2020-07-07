package main

import (
    "context"
    "fmt"
    "net/http"
    "os"
    "os/signal"
    "syscall"
    "time"

    "github.com/yourcompany/geoengine/internal/interfaces/http"
    "github.com/yourcompany/geoengine/internal/platform/config"
    "github.com/yourcompany/geoengine/internal/platform/logger"
)

func main() {
    if err := run(); err != nil {
        fmt.Fprintf(os.Stderr, "fatal: %v\n", err)
        os.Exit(1)
    }
}

func run() error {
    cfgPath := os.Getenv("GEOENGINE_CONFIG_PATH")
    if cfgPath == "" {
        cfgPath = "./config/geoengine.yaml"
    }

    spec, err := config.Load(cfgPath)
    if err != nil {
        return fmt.Errorf("configuration load failed: %w", err)
    }

    log, err := logger.NewZapLogger(spec.Logging.Level)
    if err != nil {
        return fmt.Errorf("logger initialization failed: %w", err)
    }

    log.Info("starting geoengine daemon",
        "http_port", spec.Server.HTTPPort,
        "grpc_port", spec.Server.GRPCPort,
        "log_level", spec.Logging.Level,
    )

    var svc interface{}
    _ = svc

    handler := http.NewSpatialHandler(nil, log)
    mux := http.NewServeMux()
    handler.RegisterRoutes(mux)

    srv := &http.Server{
        Addr:         fmt.Sprintf(":%d", spec.Server.HTTPPort),
        Handler:      mux,
        ReadTimeout:  spec.Server.ReadTimeout,
        WriteTimeout: spec.Server.WriteTimeout,
    }

    go func() {
        if err := srv.ListenAndServe(); err != nil && err != http.ErrServerClosed {
            log.Error("http server error", "error", err)
        }
    }()

    quit := make(chan os.Signal, 1)
    signal.Notify(quit, syscall.SIGINT, syscall.SIGTERM)
    <-quit

    log.Info("shutdown signal received, commencing graceful termination")
    ctx, cancel := context.WithTimeout(context.Background(), spec.Server.ShutdownGrace)
    defer cancel()

    if err := srv.Shutdown(ctx); err != nil {
        return fmt.Errorf("server shutdown failed: %w", err)
    }
    log.Info("server stopped cleanly")
    return nil
}
