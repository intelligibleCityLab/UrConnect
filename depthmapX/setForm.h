#pragma once

#include "ui_setForm.h"

class setForm : public QWidget
{
	Q_OBJECT

public:
	setForm(QWidget *parent = 0);
	~setForm();

	Ui::setForm ui;

private slots:
	void on_cancelButton_clicked();

	void on_radioButton_1_clicked(bool checked);
	void on_radioButton_2_clicked(bool checked);
	void on_radioButton_3_clicked(bool checked);
	void on_radioButton_4_clicked(bool checked);
};