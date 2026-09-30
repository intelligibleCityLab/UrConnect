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
#include <QDesktopWidget>

int CoreApplication::exec() {
    SettingsImpl settings(new DefaultSettingsFactory);

    //if (!settings.readSetting(SettingTag::licenseAccepted, false).toBool())
    //{
    //    auto dummy = MainWindowFactory::getLicenseDialog();
    //    dummy->setModal(true);
    //    dummy->setWindowTitle(TITLE_BASE);
    //    dummy->exec();
    //    if ( dummy->result() == QDialog::Rejected) {
    //        return 0;
    //    }
    //    settings.writeSetting(SettingTag::licenseAccepted, true);
    //}

    auto args = arguments();
    QString fileToLoad;
    if (args.length() == 2)
    {
        // Ignore flag-like arguments, e.g. the -psn_0_xxx process serial
        // number Finder passes when launching the app on macOS.
        if (!args[1].startsWith('-')) {
            fileToLoad = args[1];
        }
    }

    // Window construction may itself dispatch a FileOpen event.
    mMainWindow = MainWindowFactory::getMainWindow(QString(), settings);
    if (!mFileToLoad.isEmpty()) {
        fileToLoad = mFileToLoad;
        mFileToLoad.clear();
    }
    if (!fileToLoad.isEmpty()) {
        mMainWindow->loadFile(fileToLoad);
    }
    mMainWindow->show();
    return QApplication::exec();
}
