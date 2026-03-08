#include "Subset.h"

Subset::Subset(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

}

Subset::~Subset()
{
}

void Subset::on_cancelButton_clicked() {
	this->close();
}

void Subset::on_radioButton_clicked(bool checked) {
	if (checked) {
		ui.radioButton_2->setChecked(false);
	}
}

void Subset::on_radioButton_2_clicked(bool checked) {
	if (checked) {
		ui.radioButton->setChecked(false);
	}
}