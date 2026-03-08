#pragma once

#include "ui_Highlight.h"
#include <QWidget>
#include <vector>
#include <string>

class Highlight : public QWidget
{
	Q_OBJECT

public:
	Highlight(QWidget *parent = 0);
	~Highlight();

	Ui::Highlight ui;

public:
	void update_win(std::vector<std::string> &settings);

private slots:
	void on_checkBox_clicked(bool checked);
	void on_checkBox_2_clicked(bool checked);

	void on_radioButton_clicked(bool checked);
	void on_radioButton_2_clicked(bool checked);
	void on_radioButton_3_clicked(bool checked);

	//void on_okButton_clicked();
	//void on_cancelButton_clicked();
};