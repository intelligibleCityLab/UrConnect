#include "ui_UrbanConnectAnalyzer.h"

#include <QApplication>
#include <QDir>
#include <QImage>
#include <QMainWindow>
#include <QPushButton>
#include <QTemporaryDir>
#include <iostream>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QTemporaryDir directory;
    if (!directory.isValid() || !QDir::setCurrent(directory.path())) return 1;
    QMainWindow window;
    Ui::UrbanConnectAnalyzer ui;
    ui.setupUi(&window);
    const char* buttons[] = {"pushButton_10", "pushButton_15", "pushButton_16", "pushButton_17",
                             "pushButton_4", "pushButton_3", "pushButton_18", "pushButton_30"};
    for (const char* name : buttons) {
        auto* button = window.findChild<QPushButton*>(name);
        if (!button || button->toolTip().isEmpty()) return 1;
        QImage icon = button->icon().pixmap(32, 32).toImage();
        if (icon.isNull()) {
            std::cerr << "Missing toolbar icon: " << name << '\n';
            return 1;
        }
        bool visible = false;
        for (int y = 0; y < icon.height(); ++y) {
            for (int x = 0; x < icon.width(); ++x) {
                const QColor color = icon.pixelColor(x, y);
                visible |= color.alpha() > 0 &&
                           (color.red() < 240 || color.green() < 240 || color.blue() < 240);
            }
        }
        if (!visible) {
            std::cerr << "Blank toolbar icon: " << name << '\n';
            return 1;
        }
    }
    const char* indicators[] = {"arrow_down.png", "arrow_right.png", "checked.png", "unchecked.png",
                                "radiobutton.png", "no_radiobutton.png", "view_icons/cur00007.png",
                                "view_icons/cur00008.png", "view_icons/cur00009.png"};
    for (const char* name : indicators) {
        if (QPixmap(QString(":/ico/") + name).isNull()) return 1;
    }
    if (argc == 2) {
        window.resize(360, 900);
        window.show();
        app.processEvents();
        if (!ui.widget_67->grab().save(QString::fromLocal8Bit(argv[1]))) return 1;
    }
    std::cout << "All 8 toolbar icons, tooltips and 9 indicators/cursors load from an empty working directory\n";
    return 0;
}
