#include "CalculateUI.h"
#include "ui_CalculateUI.h"

CalculateUI::CalculateUI(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::CalculateUI)
{	
	ui->setupUi(this);
}

CalculateUI::~CalculateUI()
{
	delete ui;
}