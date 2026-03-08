#pragma once
#include "ui_ScaleWin.h"
#include <QWidget>

class ScaleWin : public QWidget
{
	Q_OBJECT

public:
	ScaleWin(QWidget *parent = 0);
	~ScaleWin();

	Ui::ScaleWin ui;

};