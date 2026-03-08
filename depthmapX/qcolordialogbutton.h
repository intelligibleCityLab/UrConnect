#ifndef COLORDIALOGBUTTON_H
#define COLORDIALOGBUTTON_H

#include <QWidget>
#include <QPushButton>
#include <QColorDialog>

class QColorDialogButton : public QPushButton
{
	Q_OBJECT

public:
	explicit QColorDialogButton(QWidget *parent = 0);
	~QColorDialogButton();

	QColor color() { return __currentColor; }

	void setColor(QColor newColor); //Set new color outside this class

signals:
	void colorChanged(QColor newColor); //Color was changed

protected slots:
	void selectNewColor(); //Show color dialog and select new color after click
	void setColorHex(QString colorHex);

private:
	QColor __currentColor;

	void updateButtonColor();

};

#endif // COLORDIALOGBUTTON_H
