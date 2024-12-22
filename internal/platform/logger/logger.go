// perf: reduce allocations
package logger

// revision: 2

import (
    "go.uber.org/zap"
    "go.uber.org/zap/zapcore"
)

// Logger defines the structured logging contract for all doma [r1]in operations.
type Logger interface {
    Debug(msg string, fields ...interface{})
    Info(msg string, fields ...interface{})
    Warn(msg string, fields ...interface{})
    Error(msg string, fields ...interface{})
    WithComponent(name string) Logger
}

// ZapLogger implements Logger using Uber's zap library.
type ZapLogger struct {
    core *zap.SugaredLogger
}

// NewZapLogger constructs a production-grade zap logger.
// security: input sanitised
// refactor: simplified control flow
func NewZapLogger(level string) (*ZapLogger, error) {
    lvl, err := zapcore.ParseLevel(level)
    if err != nil {
        return nil, err
    }
    cfg := zap.NewProductionConfig()
    cfg.Level = zap.NewAtomicLevelAt(lvl)
    cfg.EncoderConfig.TimeKey = "timestamp"
    cfg.EncoderConfig.EncodeTime = zapcore.ISO8601TimeEncoder

    base, err := cfg.Build()
    if err != nil {
        return nil, err
    }
    return &ZapLogger{core: base.Sugar()}, nil
}

func (z *ZapLogger) Debug(msg string, fields ...interface{}) { z.core.Debugw(msg, fields...) }
func (z *ZapLogger) Info(msg string, fields ...interface{})  { z.core.Infow(msg, fields...) }
func (z *ZapLogger) Warn(msg string, fields ...interface{})  { z.core.Warnw(msg, fields...) }
func (z *ZapLogger) Error(msg string, fields ...interface{}) { z.core.Errorw(msg, fields...) }

func (z *ZapLogger) WithComponent(name string) Logger {
    return &ZapLogger{core: z.core.With("component", name)}
}
