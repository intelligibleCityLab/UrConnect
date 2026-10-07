// Copyright (C) 2017 Petros Koutsolampros

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

#include "depthmapX/coreapplication.h"
#include "Logger.h"
#include <QDesktopWidget>

int CoreApplication::exec() {
    URC_LOG_INFO("CoreApplication::exec() starting...");
    SettingsImpl settings(new DefaultSettingsFactory);
    URC_LOG_DEBUG("Settings factory initialized");

    auto args = arguments();
    QString fileToLoad;
    URC_LOG_DEBUG("Arguments retrieved, count: {}", args.size());
    URC_LOG_DEBUG("About to create main window");
    if (args.length() == 2)
    {
        // Ignore flag-like arguments, e.g. the -psn_0_xxx process serial
        // number Finder passes when launching the app on macOS.
        if (!args[1].startsWith('-')) {
            fileToLoad = args[1];
            URC_LOG_INFO("Command line file to load: {}", fileToLoad.toUtf8().constData());
        }
    }

    // Window construction may itself dispatch a FileOpen event.
    mMainWindow = MainWindowFactory::getMainWindow(QString(), settings);
    URC_LOG_INFO("Main window created successfully");
    if (!mFileToLoad.isEmpty()) {
        fileToLoad = mFileToLoad;
        mFileToLoad.clear();
    }
    if (!fileToLoad.isEmpty()) {
        mMainWindow->loadFile(fileToLoad);
    }
    mMainWindow->show();
    URC_LOG_INFO("Main window shown, entering Qt event loop");

    int retCode = QApplication::exec();
    URC_LOG_INFO("Qt event loop exited with code {}", retCode);
    return retCode;
}
