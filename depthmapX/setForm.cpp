#include "setForm.h"

setForm::setForm(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	ui.widget_49->setVisible(false);
	ui.widget_51->setVisible(false);
	ui.widget_55->setVisible(false);
	ui.widget_59->setVisible(false);
}

setForm::~setForm()
{
}

void setForm::on_cancelButton_clicked() {
	this->close();
}

void setForm::on_radioButton_1_clicked(bool checked)
{
	//ui.widget_49->setVisible(checked);
}

void setForm::on_radioButton_2_clicked(bool checked)
{
	ui.widget_51->setVisible(checked);
}

void setForm::on_radioButton_3_clicked(bool checked)
{
	ui.widget_55->setVisible(checked);
}

void setForm::on_radioButton_4_clicked(bool checked)
{
	ui.widget_59->setVisible(checked);
}