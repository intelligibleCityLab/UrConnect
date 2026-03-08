#include "Select.h"


Select::Select(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);


}

Select::~Select()
{
}


void Select::update_win(std::vector<std::string> &settings) {
	ui.comboBox->clear(); //清除列表

	for (std::string name : settings) {
		ui.comboBox->addItem(QString(QString::fromLocal8Bit(name.c_str()))); //不带图标
	}
}
