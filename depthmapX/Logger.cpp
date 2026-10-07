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

#include "Logger.h"

#include <sys/stat.h>
#include <iostream>
#include <chrono>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <QDir>
#include <QString>
#endif

namespace UrConnect {

// 单例获取
Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

// 跨平台创建目录
bool Logger::ensureLogDirectory(const std::string& dirPath) const {
    if (dirPath.empty()) return true;

#ifdef _WIN32
    // Windows: 递归创建目录
    DWORD attr = GetFileAttributesA(dirPath.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES &&
        (attr & FILE_ATTRIBUTE_DIRECTORY)) {
        return true;
    }

    // 分割路径，递归创建
    std::string partialPath;
    size_t pos = 0;
    size_t prev = 0;
    while ((pos = dirPath.find_first_of("\\/", prev)) != std::string::npos) {
        partialPath = dirPath.substr(0, pos);
        if (!partialPath.empty()) {
            // 检查目录是否存在
            attr = GetFileAttributesA(partialPath.c_str());
            if (attr == INVALID_FILE_ATTRIBUTES) {
                if (!CreateDirectoryA(partialPath.c_str(), NULL) &&
                    GetLastError() != ERROR_ALREADY_EXISTS) {
                    return false;
                }
            }
        }
        prev = pos + 1;
    }
    // 创建最后一级目录
    attr = GetFileAttributesA(dirPath.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectoryA(dirPath.c_str(), NULL) &&
            GetLastError() != ERROR_ALREADY_EXISTS) {
            return false;
        }
    }
    return true;
#else
    // Unix-like: 使用 mkdir，递归创建
    auto mkdirOne = [](const std::string& path) -> bool {
        struct stat st;
        if (stat(path.c_str(), &st) == 0) {
            return S_ISDIR(st.st_mode);
        }
        if (mkdir(path.c_str(), 0755) == 0) return true;
        return errno == EEXIST;
    };

    std::string partial;
    size_t prev = 0;
    size_t pos = 0;
    if (dirPath[0] == '/') {
        partial = "/";
        prev = 1;
    }
    while ((pos = dirPath.find('/', prev)) != std::string::npos) {
        partial = dirPath.substr(0, pos);
        if (!partial.empty() && !mkdirOne(partial)) {
            return false;
        }
        prev = pos + 1;
    }
    return mkdirOne(dirPath);
#endif
}

// 构建完整日志文件路径
std::string Logger::buildLogFilePath(const std::string& dir,
                                     const std::string& filename) const {
    if (dir.empty()) return filename;
    std::string path = dir;
    // 确保路径分隔符
    if (path.back() != '/' && path.back() != '\\') {
#ifdef _WIN32
        path += "\\";
#else
        path += "/";
#endif
    }
    path += filename;
    return path;
}

// 初始化日志系统
bool Logger::init(const LoggerConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized) {
        // 已经初始化，先关闭旧的
        try { spdlog::shutdown(); } catch (...) {}
        m_logger.reset();
        m_initialized = false;
    }

    m_config = config;

    try {
        // 1. 确保日志目录存在
        if (!ensureLogDirectory(config.logDir)) {
            std::cerr << "[Logger] Failed to create log directory: "
                      << config.logDir << std::endl;
            return false;
        }

        // 2. 准备多个 sink（输出目标）
        std::vector<spdlog::sink_ptr> sinks;

        // 2.1 控制台 sink（带颜色）
        if (config.consoleEnabled) {
            auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            consoleSink->set_level(static_cast<spdlog::level::level_enum>(config.level));
            sinks.push_back(consoleSink);
        }

        // 2.2 按大小轮转的文件 sink
        std::string rotFilePath = buildLogFilePath(config.logDir, config.logFileName);
        auto rotatingSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            rotFilePath, config.maxFileSize, config.maxFileCount);
        rotatingSink->set_level(static_cast<spdlog::level::level_enum>(config.level));
        sinks.push_back(rotatingSink);

        // 2.3 按时间（每日）轮转的文件 sink
        std::string dailyFilePath = buildLogFilePath(
            config.logDir,
            config.dailyLogBaseName + "." + config.dailyLogExt);
        auto dailySink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
            dailyFilePath,
            config.dailyRotationHour,
            config.dailyRotationMinute);
        dailySink->set_level(static_cast<spdlog::level::level_enum>(config.level));
        sinks.push_back(dailySink);

        // 3. 创建 logger
        if (config.asyncEnabled) {
            // 初始化异步线程池
            spdlog::init_thread_pool(config.asyncQueueSize, 1);
            // 注册为默认 logger
            m_logger = std::make_shared<spdlog::async_logger>(
                "urconnect",
                sinks.begin(), sinks.end(),
                spdlog::thread_pool(),
                spdlog::async_overflow_policy::block);
            spdlog::set_default_logger(m_logger);
        } else {
            m_logger = std::make_shared<spdlog::logger>(
                "urconnect",
                sinks.begin(), sinks.end());
            spdlog::set_default_logger(m_logger);
        }

        // 4. 设置全局格式
        m_logger->set_pattern(config.pattern);

        // 5. 设置全局日志级别
        m_logger->set_level(static_cast<spdlog::level::level_enum>(config.level));
        // 设置刷新级别：错误及以上立即刷新
        m_logger->flush_on(spdlog::level::warn);

        // 6. 定时自动刷新（每3秒）
        spdlog::flush_every(std::chrono::seconds(3));

        m_initialized = true;

        m_logger->info("============================================");
        m_logger->info("UrConnect logging system initialized");
        m_logger->info("Log directory : {}", config.logDir);
        m_logger->info("Rotating file : {} (max {} bytes x {} files)",
                       rotFilePath, config.maxFileSize, config.maxFileCount);
        m_logger->info("Daily file   : {} (rotate at {:02}:{:02})",
                       dailyFilePath, config.dailyRotationHour,
                       config.dailyRotationMinute);
        m_logger->info("Async mode   : {}", config.asyncEnabled ? "enabled" : "disabled");
        m_logger->info("Log level    : {}", spdlog::level::to_short_c_str(
                       m_logger->level()));
        m_logger->info("============================================");
    }
    catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "[Logger] Spdlog initialization failed: " << ex.what()
                  << std::endl;
        m_initialized = false;
        return false;
    }
    catch (const std::exception& ex) {
        std::cerr << "[Logger] Initialization failed: " << ex.what() << std::endl;
        m_initialized = false;
        return false;
    }

    return true;
}

// 关闭日志系统
void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized) return;

    try {
        if (m_logger) {
            m_logger->info("UrConnect logging system shutting down...");
            m_logger->flush();
        }
        spdlog::shutdown();
    }
    catch (...) {
        // 关闭时异常忽略
    }

    m_logger.reset();
    m_initialized = false;
}

// 设置日志级别
void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_logger) return;

    try {
        m_logger->set_level(static_cast<spdlog::level::level_enum>(level));
        m_config.level = level;
    }
    catch (...) {}
}

// 获取当前日志级别
LogLevel Logger::getLevel() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_logger) return m_config.level;

    try {
        return static_cast<LogLevel>(m_logger->level());
    }
    catch (...) {
        return m_config.level;
    }
}

// 强制刷新日志
void Logger::flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_logger) return;

    try { m_logger->flush(); }
    catch (...) {}
}

// 便捷日志接口实现
#define URC_DEFINE_LOG_METHOD(LEVEL_NAME, SPDLOG_FUNC) \
    void Logger::LEVEL_NAME(const std::string& msg) { \
        if (!m_initialized) return; \
        try { \
            if (m_logger) m_logger->SPDLOG_FUNC(msg); \
        } catch (...) {} \
    }

URC_DEFINE_LOG_METHOD(trace, trace)
URC_DEFINE_LOG_METHOD(debug, debug)
URC_DEFINE_LOG_METHOD(info, info)
URC_DEFINE_LOG_METHOD(warn, warn)
URC_DEFINE_LOG_METHOD(error, error)
URC_DEFINE_LOG_METHOD(critical, critical)

#undef URC_DEFINE_LOG_METHOD

} // namespace UrConnect
