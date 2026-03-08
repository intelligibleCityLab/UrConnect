#pragma once
#include "ui_AttributesWin.h"
#include <QWidget>

class AttributesWin : public QWidget
{
	Q_OBJECT

public:
	AttributesWin(QWidget *parent = 0);
	~AttributesWin();

	Ui::AttributesWin ui;

public:
	void update_win(QVector<QString> &AttributesVec);

private slots:
	void on_checkBox_clicked(bool checked);
	void on_pushButton_2_clicked();
};