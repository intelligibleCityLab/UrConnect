#pragma once

#include "ui_Select.h"
#include <QWidget>
#include <vector>
#include <string>

class Select : public QWidget
{
	Q_OBJECT

public:
	Select(QWidget *parent = 0);
	~Select();

	Ui::Select ui;

public:
	void update_win(std::vector<std::string> &settings);

};