package config

import (
	"testing"
	"time"
)

func TestServerConfig_Validate_Valid(t *testing.T) {
	c := ServerConfig{
		HTTPPort:      8080,
		GRPCPort:      9090,
		ReadTimeout:   30 * time.Second,
		WriteTimeout:  30 * time.Second,
		ShutdownGrace: 15 * time.Second,
	}
	if err := c.Validate(); err != nil {
		t.Errorf("valid config should pass: %v", err)
	}
}

func TestServerConfig_Validate_InvalidPorts(t *testing.T) {
	tests := []struct {
		name string
		cfg  ServerConfig
	}{
		{"http_port zero", ServerConfig{HTTPPort: 0, GRPCPort: 9090, ReadTimeout: time.Second, WriteTimeout: time.Second, ShutdownGrace: time.Second}},
		{"http_port negative", ServerConfig{HTTPPort: -1, GRPCPort: 9090, ReadTimeout: time.Second, WriteTimeout: time.Second, ShutdownGrace: time.Second}},
		{"http_port too high", ServerConfig{HTTPPort: 70000, GRPCPort: 9090, ReadTimeout: time.Second, WriteTimeout: time.Second, ShutdownGrace: time.Second}},
		{"grpc_port zero", ServerConfig{HTTPPort: 8080, GRPCPort: 0, ReadTimeout: time.Second, WriteTimeout: time.Second, ShutdownGrace: time.Second}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if err := tt.cfg.Validate(); err == nil {
				t.Error("expected validation error")
			}
		})
	}
}

func TestServerConfig_Validate_NegativeTimeouts(t *testing.T) {
	c := ServerConfig{
		HTTPPort:      8080,
		GRPCPort:      9090,
		ReadTimeout:   -1 * time.Second,
		WriteTimeout:  30 * time.Second,
		ShutdownGrace: 15 * time.Second,
	}
	if err := c.Validate(); err == nil {
		t.Error("negative read_timeout should fail")
	}
}

func TestStorageConfig_Validate_Valid(t *testing.T) {
	c := StorageConfig{
		DataPath:           "/data/geoengine",
		SnapshotInterval:   5 * time.Minute,
		MaxMemoryMB:        512,
		ConnectionPoolSize: 20,
	}
	if err := c.Validate(); err != nil {
		t.Errorf("valid config should pass: %v", err)
	}
}

func TestStorageConfig_Validate_InvalidValues(t *testing.T) {
	tests := []struct {
		name string
		cfg  StorageConfig
	}{
		{"empty data_path", StorageConfig{DataPath: "", SnapshotInterval: time.Minute, MaxMemoryMB: 512, ConnectionPoolSize: 20}},
		{"zero snapshot_interval", StorageConfig{DataPath: "/data", SnapshotInterval: 0, MaxMemoryMB: 512, ConnectionPoolSize: 20}},
		{"zero max_memory_mb", StorageConfig{DataPath: "/data", SnapshotInterval: time.Minute, MaxMemoryMB: 0, ConnectionPoolSize: 20}},
		{"max_memory_mb too large", StorageConfig{DataPath: "/data", SnapshotInterval: time.Minute, MaxMemoryMB: 70000, ConnectionPoolSize: 20}},
		{"zero connection_pool_size", StorageConfig{DataPath: "/data", SnapshotInterval: time.Minute, MaxMemoryMB: 512, ConnectionPoolSize: 0}},
		{"connection_pool_size too large", StorageConfig{DataPath: "/data", SnapshotInterval: time.Minute, MaxMemoryMB: 512, ConnectionPoolSize: 20000}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if err := tt.cfg.Validate(); err == nil {
				t.Error("expected validation error")
			}
		})
	}
}

func TestLoggingConfig_Validate_Valid(t *testing.T) {
	validLevels := []string{"debug", "info", "warn", "error", "fatal"}
	for _, level := range validLevels {
		c := LoggingConfig{Level: level, Format: "json"}
		if err := c.Validate(); err != nil {
			t.Errorf("level '%s' should be valid: %v", level, err)
		}
	}
}

func TestLoggingConfig_Validate_InvalidLevel(t *testing.T) {
	c := LoggingConfig{Level: "verbose", Format: "json"}
	if err := c.Validate(); err == nil {
		t.Error("invalid level should fail")
	}
}

func TestLoggingConfig_Validate_InvalidFormat(t *testing.T) {
	c := LoggingConfig{Level: "info", Format: "xml"}
	if err := c.Validate(); err == nil {
		t.Error("invalid format should fail")
	}
}

func TestSecurityConfig_Validate_WildcardOrigin(t *testing.T) {
	c := SecurityConfig{
		JWTSecretEnv:   "SECRET",
		AllowedOrigins: []string{"*"},
	}
	if err := c.Validate(); err == nil {
		t.Error("wildcard origin should fail")
	}
}

func TestSecurityConfig_Validate_EmptyJWTSecret(t *testing.T) {
	c := SecurityConfig{
		JWTSecretEnv:   "",
		AllowedOrigins: []string{"https://app.geoengine.io"},
	}
	if err := c.Validate(); err == nil {
		t.Error("empty JWT secret should fail")
	}
}

func TestSpecification_Validate_AllValid(t *testing.T) {
	s := Specification{
		Server: ServerConfig{
			HTTPPort:      8080,
			GRPCPort:      9090,
			ReadTimeout:   30 * time.Second,
			WriteTimeout:  30 * time.Second,
			ShutdownGrace: 15 * time.Second,
		},
		Storage: StorageConfig{
			DataPath:           "/data",
			SnapshotInterval:   5 * time.Minute,
			MaxMemoryMB:        512,
			ConnectionPoolSize: 20,
		},
		Logging: LoggingConfig{
			Level:  "info",
			Format: "json",
		},
		Security: SecurityConfig{
			JWTSecretEnv:   "SECRET",
			AllowedOrigins: []string{"https://app.geoengine.io"},
		},
	}
	if err := s.Validate(); err != nil {
		t.Errorf("valid specification should pass: %v", err)
	}
}
