#include "AttributesWin.h"

AttributesWin::AttributesWin(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
}

AttributesWin::~AttributesWin()
{
}

void AttributesWin::update_win(QVector<QString> &AttributesVec) {
	ui.listWidget->clear();

	int pos = 0;
	for (QString name : AttributesVec) {
		ui.listWidget->addItem(name);
		ui.listWidget->setCurrentRow(pos);
		ui.listWidget->currentItem()->setCheckState(Qt::Unchecked);
		++pos;
	}
}

void AttributesWin::on_checkBox_clicked(bool checked) {
	for (int i = 0; i < ui.listWidget->count(); i++)
	{
		ui.listWidget->setCurrentRow(i);
		if (checked)
			ui.listWidget->currentItem()->setCheckState(Qt::Checked);
		else
			ui.listWidget->currentItem()->setCheckState(Qt::Unchecked);
	}
}

void AttributesWin::on_pushButton_2_clicked() {
	this->close();
}