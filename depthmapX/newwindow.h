#pragma once

#include <QWidget>
#include "ui_newwindow.h"
#include "qradiobutton.h"
#include <QDockWidget>

class newwindow : public QWidget
{
	Q_OBJECT

public:
	newwindow(QWidget *parent = Q_NULLPTR);
	~newwindow();
	QDockWidget *AttributesListDock;
	QWidget * setupAttributesListWidget(QVector<QString> ggg);

	void update_win(QVector<QString> Attri);
	QVector<QRadioButton*> my_vector;

private:
	Ui::newwindow ui;
	int number;
	QMap<int,bool> result;
signals:
	void infoSend(QString);
private slots:
	void get_checked(bool );
	void get_all_checked(bool);
	void  sendData();

	
};
