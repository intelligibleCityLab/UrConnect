#include "qcolordialogbutton.h"

#include <QColorDialog>
#include <QDebug>

QColorDialogButton::QColorDialogButton(QWidget *parent) :
	QPushButton(parent)
{
	QColor base(0, 0, 0);

	connect(this, SIGNAL(clicked()), this, SLOT(selectNewColor()));
	this->setAutoFillBackground(true);
	qDebug() << this->autoFillBackground();

	setColor(base);
	update();
}

QColorDialogButton::~QColorDialogButton()
{
}

void QColorDialogButton::setColor(QColor newColor)
{
	if (!newColor.isValid()) //Color validation
		return;

	__currentColor = newColor;
	updateButtonColor();

	emit colorChanged(newColor);
}

void QColorDialogButton::selectNewColor()
{
	return setColor(QColorDialog::getColor(Qt::green, this));//单击改变颜色事件
}

void QColorDialogButton::setColorHex(QString colorHex)
{
	if (colorHex.isEmpty())
		return;
	QString colorCode = colorHex;
	if (colorCode.at(0) != '#')
		colorCode.push_front('#');
	QColor color(colorCode);

	setColor(color);
}

void QColorDialogButton::updateButtonColor()
{
	QPalette palette = this->palette();
	palette.setColor(QPalette::Active, QPalette::Button, __currentColor);
	palette.setColor(QPalette::Inactive, QPalette::Button, __currentColor);
	this->setPalette(palette);
}
