#pragma once

#ifndef CALCULATEUI_H

#define CALCULATEUI_H

#include <QtWidgets/QMainWindow>

namespace Ui {
	class CalculateUI;
}

class CalculateUI : public QMainWindow
{
	Q_OBJECT

public:

	explicit CalculateUI(QWidget *parent = 0);

	~CalculateUI();

//private:

	Ui::CalculateUI *ui;

};

#endif // CALCULATEUI_H
