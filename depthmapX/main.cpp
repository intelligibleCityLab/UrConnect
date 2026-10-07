// Copyright (C) 2011-2012, Tasos Varoudis
// Copyright (C) 2017 Christian Sailer
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

// UrbanConnect is an independent spatial syntax analysis tool developed by scholars from Shenzhen University and Georgia Institute of Technology.
// Based on depthmapX, it implements innovative spatial analysis methods combining topological and metric distance concepts.

#include <QPixmap>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QtGlobal>
#include <QtWidgets/QApplication>
#include "coreapplication.h"
#include "Logger.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _MSC_VER
// 在应用程序退出时确保日志系统被关闭
struct LoggerShutdownGuard {
    ~LoggerShutdownGuard() {
        try { ::UrConnect::Logger::instance().shutdown(); }
        catch (...) {}
    }
};
#endif

// Qt 消息处理器：将 Qt 日志（qDebug/qWarning/qCritical 等）转发到 spdlog
// 这样所有 Qt 内部日志和 qDebug() 等输出都会被统一记录
// 注意：跨 DLL 边界传递 std::string 会因 CRT 不一致 (Qt=/MD, app=/MT) 而
// 出现 ABI 不匹配，所以这里用 toUtf8().constData() 直接传 const char*
static void urconnectQtMessageHandler(QtMsgType type, const QMessageLogContext &context,
                                       const QString &msg) {
    const char *file = context.file ? context.file : "unknown";
    int line = context.line;
    // 用 lambda 内联构造单次格式化字符串，避免多次跨 DLL 传递 std::string
    auto format_msg = [&]() -> std::string {
        return std::string(msg.toUtf8().constData()) + " (" + file + ":" +
               std::to_string(line) + ")";
    };
    const std::string formatted = format_msg();

    switch (type) {
        case QtDebugMsg:
            URC_LOG_DEBUG("Qt: {}", formatted);
            break;
        case QtInfoMsg:
            URC_LOG_INFO("Qt: {}", formatted);
            break;
        case QtWarningMsg:
            URC_LOG_WARN("Qt: {}", formatted);
            break;
        case QtCriticalMsg:
            URC_LOG_ERROR("Qt: {}", formatted);
            break;
        case QtFatalMsg:
            URC_LOG_CRITICAL("Qt Fatal: {}", formatted);
            break;
        default:
            URC_LOG_INFO("Qt: {}", formatted);
            break;
    }
}

int main(int argc, char *argv[])
{
    // 初始化日志系统（在 Qt 资源初始化之前）
    UrConnect::LoggerConfig logConfig;
    QCoreApplication::setOrganizationName("IntelligibleCityLab");
    QCoreApplication::setApplicationName("UrConnect");
    logConfig.logDir = (QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                       + "/logs").toStdString();
    logConfig.logFileName = "urconnect.log";
    logConfig.dailyLogBaseName = "urconnect_daily";
    logConfig.dailyLogExt = "log";
    logConfig.maxFileSize = 50 * 1024 * 1024;  // 50 MB
    logConfig.maxFileCount = 10;
    logConfig.dailyMaxDays = 30;
    // Debug 构建使用 Debug 级别，Release 使用 Info 级别
#ifdef NDEBUG
    logConfig.level = UrConnect::LogLevel::Info;
#else
    logConfig.level = UrConnect::LogLevel::Debug;
#endif
    logConfig.asyncEnabled = false;  // 同步模式：避免异步队列在跨 DLL 场景下的崩溃
    logConfig.asyncQueueSize = 8192;

    if (!UrConnect::Logger::instance().init(logConfig)) {
        std::cerr << "Warning: Failed to initialize logging system." << std::endl;
    }

    // 安装 Qt 消息处理器：将 Qt 内部日志重定向到 spdlog
    qInstallMessageHandler(urconnectQtMessageHandler);

    // 日志系统关闭守卫：在 main 返回时确保日志被刷新和关闭
    // 使用静态对象，在程序退出时自动析构
#ifdef _MSC_VER
    static LoggerShutdownGuard shutdownGuard;
#endif

    URC_LOG_INFO("UrConnect application starting up...");
    URC_LOG_DEBUG("Application arguments count: {}", argc);

    Q_INIT_RESOURCE(resource);
    Q_INIT_RESOURCE(settingsdialog);

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    CoreApplication app(argc, argv);

    int retCode = 0;
    try {
        retCode = app.exec();
        URC_LOG_INFO("Application exited with code {}", retCode);
    }
    catch (const std::exception& ex) {
        URC_LOG_CRITICAL("Application crashed with std::exception: {}", ex.what());
        retCode = -1;
    }
    catch (...) {
        URC_LOG_CRITICAL("Application crashed with unknown exception");
        retCode = -1;
    }

    // 安全关闭日志系统（与守卫作用一致，确保异步日志被刷新）
    UrConnect::Logger::instance().shutdown();
    return retCode;
}
