#include "MRDlg.h"
#include "mainwindow.h"

Dialog::Dialog(QWidget *parent) : QDialog(parent) 
{
	setupUi(this);
	setWindowFlags(Qt::WindowStaysOnTopHint);
}
