#pragma once
#include "ui_MRDlg.h"

class Dialog : public QDialog, public Ui::Dialog {
	Q_OBJECT
public:
	Dialog(QWidget *parent = 0);

	double mrlimit, Jnlimit, wgtlimit;
	bool isJn, isWgt;

	private slots:

};