#pragma once

#include "ui_Subset.h"

class Subset : public QWidget
{
	Q_OBJECT

public:
	Subset(QWidget *parent = 0);
	~Subset();

	Ui::Subset ui;

private slots:
	void on_cancelButton_clicked();

	void on_radioButton_clicked(bool checked);
	void on_radioButton_2_clicked(bool checked);
};