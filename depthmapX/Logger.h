// Copyright (C) 2026, UrbanConnect Team

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>

#include <memory>
#include <mutex>
#include <string>

namespace UrConnect {

// Log level definition
enum class LogLevel {
    Trace    = SPDLOG_LEVEL_TRACE,
    Debug    = SPDLOG_LEVEL_DEBUG,
    Info     = SPDLOG_LEVEL_INFO,
    Warn     = SPDLOG_LEVEL_WARN,
    Error    = SPDLOG_LEVEL_ERROR,
    Critical = SPDLOG_LEVEL_CRITICAL,
    Off      = SPDLOG_LEVEL_OFF
};

// Logger configuration
struct LoggerConfig {
    // Log file directory (relative to working directory)
    std::string logDir = "logs";
    // Main log file name (for rotating log)
    std::string logFileName = "urconnect.log";
    // Daily log file name prefix
    std::string dailyLogBaseName = "urconnect_daily";
    // Daily log file extension
    std::string dailyLogExt = "log";
    // Size-based rotation: max single file size (bytes)
    std::size_t maxFileSize = 50 * 1024 * 1024;  // 50 MB
    // Size-based rotation: number of log files to keep
    std::size_t maxFileCount = 10;
    // Daily rotation time (hour, minute) - default: midnight
    int dailyRotationHour = 0;
    int dailyRotationMinute = 0;
    // Max days to keep daily logs
    std::size_t dailyMaxDays = 30;
    // Minimum log level
    LogLevel level = LogLevel::Info;
    // Enable async logging (improves performance, may lose some logs on crash)
    bool asyncEnabled = true;
    // Async queue size
    std::size_t asyncQueueSize = 8192;
    // Log format pattern
    std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] [%n] %v";
    // Enable console output
    bool consoleEnabled = true;
};

class Logger {
public:
    // Get singleton instance
    static Logger& instance();

    // Initialize the logging system
    // config: configuration parameters, use defaults if not specified
    // Returns true if initialization succeeded
    bool init(const LoggerConfig& config = LoggerConfig{});

    // Shutdown the logging system, flush all logs
    void shutdown();

    // Check if the logging system is initialized
    bool isInitialized() const { return m_initialized; }

    // Set log level
    void setLevel(LogLevel level);

    // Get current log level
    LogLevel getLevel() const;

    // Flush logs (force write all buffered logs)
    void flush();

    // ============ Simple log interface ============
    void trace(const std::string& msg);
    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);
    void critical(const std::string& msg);

    // ============ Formatted log interface ============
    template<typename... Args>
    void trace(const char* fmt, const Args&... args) {
        log(LogLevel::Trace, fmt, args...);
    }
    template<typename... Args>
    void debug(const char* fmt, const Args&... args) {
        log(LogLevel::Debug, fmt, args...);
    }
    template<typename... Args>
    void info(const char* fmt, const Args&... args) {
        log(LogLevel::Info, fmt, args...);
    }
    template<typename... Args>
    void warn(const char* fmt, const Args&... args) {
        log(LogLevel::Warn, fmt, args...);
    }
    template<typename... Args>
    void error(const char* fmt, const Args&... args) {
        log(LogLevel::Error, fmt, args...);
    }
    template<typename... Args>
    void critical(const char* fmt, const Args&... args) {
        log(LogLevel::Critical, fmt, args...);
    }

    // Generic log interface (formatted)
    template<typename... Args>
    void log(LogLevel level, const char* fmt, const Args&... args) {
        if (!m_initialized) return;
        try {
            if (m_logger) {
                m_logger->log(static_cast<spdlog::level::level_enum>(level),
                              fmt, args...);
            }
        } catch (...) {
            // Log exceptions should not affect main flow
        }
    }

    // Get underlying spdlog logger (advanced usage)
    std::shared_ptr<spdlog::logger> getLogger() const { return m_logger; }

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Create log directory (cross-platform)
    bool ensureLogDirectory(const std::string& dirPath) const;

    // Build full log file path
    std::string buildLogFilePath(const std::string& dir,
                                  const std::string& filename) const;

    std::shared_ptr<spdlog::logger> m_logger;
    bool m_initialized = false;
    mutable std::mutex m_mutex;
    LoggerConfig m_config;
};

} // namespace UrConnect

// ============ Global convenience log macros ============
// Usage: URC_LOG_INFO("Application started");
//        URC_LOG_ERROR("Failed to load file: {}", filePath);
#define URC_LOG_TRACE(...)    ::UrConnect::Logger::instance().trace(__VA_ARGS__)
#define URC_LOG_DEBUG(...)    ::UrConnect::Logger::instance().debug(__VA_ARGS__)
#define URC_LOG_INFO(...)     ::UrConnect::Logger::instance().info(__VA_ARGS__)
#define URC_LOG_WARN(...)    ::UrConnect::Logger::instance().warn(__VA_ARGS__)
#define URC_LOG_ERROR(...)   ::UrConnect::Logger::instance().error(__VA_ARGS__)
#define URC_LOG_CRITICAL(...) ::UrConnect::Logger::instance().critical(__VA_ARGS__)
