#include "Highlight.h"

Highlight::Highlight(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	
	ui.checkBox->setChecked(true);

	ui.checkBox_2->setChecked(false);
	ui.comboBox->setEnabled(false);

	ui.spinBox_2->setEnabled(false);

	ui.radioButton->setEnabled(false);
	ui.lineEdit->setEnabled(false);

	ui.radioButton_2->setEnabled(false);
	ui.comboBox_2->setEnabled(false);
}

Highlight::~Highlight()
{
}

void Highlight::on_checkBox_clicked(bool checked) {
	ui.spinBox->setEnabled(checked);
}

void Highlight::on_checkBox_2_clicked(bool checked) {
	ui.comboBox->setEnabled(checked);

	ui.spinBox_2->setEnabled(checked);

	ui.radioButton->setEnabled(checked);
	ui.radioButton_2->setEnabled(checked);
}

void Highlight::on_radioButton_clicked(bool checked) {
	ui.lineEdit->setEnabled(checked);
	if (checked) {
		//ui.lineEdit->setEnabled(false);
		ui.comboBox_2->setEnabled(false);
	}
}

void Highlight::on_radioButton_2_clicked(bool checked) {
	ui.comboBox_2->setEnabled(checked);
	if (checked) {
		ui.lineEdit->setEnabled(false);
		//ui.comboBox_2->setEnabled(false);
	}
}

void Highlight::on_radioButton_3_clicked(bool checked) {
	ui.comboBox_2->setEnabled(checked);
	if (checked) {
		ui.lineEdit->setEnabled(false);
		ui.comboBox_2->setEnabled(false);
		//ui.comboBox_3->setEnabled(false);
	}
}

void Highlight::update_win(std::vector<std::string> &settings) {
	ui.comboBox->clear(); //清除列表

	for (std::string name:settings) {
		ui.comboBox->addItem(QString(QString::fromLocal8Bit(name.c_str()))); //不带图标
	}
}

//void Highlight::on_okButton_clicked() {
//	this->close();
//}
//
//void Highlight::on_cancelButton_clicked() {
//	this->close();
//}