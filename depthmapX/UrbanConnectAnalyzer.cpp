#include "UrbanConnectAnalyzer.h"
#include <QString>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>
//#include "GraphDoc.h"
#include <QFileDialog>

UrbanConnectAnalyzer::UrbanConnectAnalyzer(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	ui.colorBackground->setVisible(false);

	//ui.widget_4->setVisible(Qt::Checked);
	//ui.widget_5->setVisible(Qt::Unchecked);
	ui.widget_37->setVisible(Qt::Unchecked);

	ui.widget_46->setVisible(Qt::Unchecked);
	ui.widget_78->setVisible(Qt::Unchecked);
	ui.widget_47->setVisible(Qt::Unchecked);
	ui.widget_264->setVisible(Qt::Unchecked);

	ui.widget_49->setVisible(Qt::Unchecked);
	ui.widget_51->setVisible(Qt::Unchecked);
	ui.widget_55->setVisible(Qt::Unchecked);
	ui.widget_59->setVisible(Qt::Unchecked);

	ui.widget_269->setVisible(Qt::Unchecked);
	ui.widget_271->setVisible(Qt::Unchecked);


	ui.widget_41->setVisible(Qt::Unchecked);
	ui.widget_43->setVisible(Qt::Unchecked);

	ui.widget_44->setVisible(Qt::Unchecked);
	
	ui.widget_120->setVisible(Qt::Unchecked);
	ui.widget_122->setVisible(Qt::Unchecked);
	ui.widget_126->setVisible(Qt::Unchecked);
	ui.widget_130->setVisible(Qt::Unchecked);

	ui.widget_134->setVisible(Qt::Unchecked);
	ui.widget_72->setVisible(Qt::Unchecked);

	ui.widget_80->setVisible(Qt::Unchecked);
	ui.widget_89->setVisible(Qt::Unchecked);
	ui.widget_82->setVisible(Qt::Unchecked);
	ui.widget_168->setVisible(Qt::Unchecked);

	ui.lineEdit_110->setStyleSheet("QLineEdit{background-color: rgb(150, 150, 150);color: rgb(255, 255, 255);border :0px solid rgb(150, 150, 150); font: 8pt Ebrima;} ");
	ui.lineEdit_110->setEnabled(false);

	ui.lineEdit_56->setStyleSheet("QLineEdit{background-color: rgb(150, 150, 150);color: rgb(255, 255, 255);border :0px solid rgb(150, 150, 150); font: 8pt Ebrima;} ");
	ui.lineEdit_56->setEnabled(false);

	ui.lineEdit->setStyleSheet("QLineEdit{background-color: rgb(150, 150, 150);color: rgb(255, 255, 255);border :0px solid rgb(150, 150, 150); font: 8pt Ebrima;} ");
	ui.lineEdit->setEnabled(false);

	//ui.lineEdit_32->setStyleSheet("QLineEdit{background-color: rgb(150, 150, 150);color: rgb(255, 255, 255);border :0px solid rgb(150, 150, 150); font: 8pt Ebrima;} ");
	//ui.lineEdit_32->setEnabled(false);

	ui.widget_139->setVisible(Qt::Unchecked);
	ui.widget_144->setVisible(Qt::Unchecked);

	ui.widget_147->setVisible(Qt::Unchecked);

	//advance analysis
	ui.widget_152->setVisible(Qt::Unchecked);
}

void UrbanConnectAnalyzer::on_pushButton_10_clicked(bool checked) {
	if (checked) {
		//ui.pushButton_10->setChecked(false);
		ui.pushButton_15->setChecked(false);
		ui.pushButton_16->setChecked(false);
		ui.pushButton_17->setChecked(false);
		ui.pushButton_4->setChecked(false);
		ui.pushButton_3->setChecked(false);
	}
}

void UrbanConnectAnalyzer::on_pushButton_15_clicked(bool checked) {
	if (checked) {
		ui.pushButton_10->setChecked(false);
		//ui.pushButton_15->setChecked(false);
		ui.pushButton_16->setChecked(false);
		ui.pushButton_17->setChecked(false);
		ui.pushButton_4->setChecked(false);
		ui.pushButton_3->setChecked(false);
	}
}

void UrbanConnectAnalyzer::on_pushButton_16_clicked(bool checked) {
	if (checked) {
		ui.pushButton_10->setChecked(false);
		ui.pushButton_15->setChecked(false);
		//ui.pushButton_16->setChecked(false);
		ui.pushButton_17->setChecked(false);
		ui.pushButton_4->setChecked(false);
		ui.pushButton_3->setChecked(false);
	}
}

void UrbanConnectAnalyzer::on_pushButton_17_clicked(bool checked) {
	if (checked) {
		ui.pushButton_10->setChecked(false);
		ui.pushButton_15->setChecked(false);
		ui.pushButton_16->setChecked(false);
		//ui.pushButton_17->setChecked(false);
		ui.pushButton_4->setChecked(false);
		ui.pushButton_3->setChecked(false);
	}
}

void UrbanConnectAnalyzer::on_pushButton_4_clicked(bool checked) {
	if (checked) {
		ui.pushButton_10->setChecked(false);
		ui.pushButton_15->setChecked(false);
		ui.pushButton_16->setChecked(false);
		ui.pushButton_17->setChecked(false);
		//ui.pushButton_4->setChecked(false);
		ui.pushButton_3->setChecked(false);
	}
}

void UrbanConnectAnalyzer::on_pushButton_3_clicked(bool checked) {
	if (checked) {
		ui.pushButton_10->setChecked(false);
		ui.pushButton_15->setChecked(false);
		ui.pushButton_16->setChecked(false);
		ui.pushButton_17->setChecked(false);
		ui.pushButton_4->setChecked(false);
		//ui.pushButton_3->setChecked(false);
	}
}



void UrbanConnectAnalyzer::on_pushButton_12_clicked(bool checked)
{

	ui.widget_5->setVisible(checked);
	if (checked)
	{

		//ui.pushButton_5->setChecked(false);
		ui.pushButton_7->setChecked(false);

		ui.widget_46->setVisible(Qt::Unchecked);
		ui.widget_78->setVisible(Qt::Unchecked);
		ui.widget_264->setVisible(Qt::Unchecked);
		ui.widget_37->setVisible(Qt::Unchecked);

		//关闭advance
		ui.widget_72->setVisible(Qt::Unchecked);
	}
}

void UrbanConnectAnalyzer::on_pushButton_5_clicked(bool checked)
{
	ui.widget_4->setVisible(checked);
	//if (checked)
	//{
	//	ui.pushButton_12->setChecked(false);
	//	ui.pushButton_7->setChecked(false);

	//	ui.widget_5->setVisible(Qt::Unchecked);
	//	ui.widget_37->setVisible(Qt::Unchecked);
	//}
}

void UrbanConnectAnalyzer::on_pushButton_7_clicked(bool checked)
{
	ui.widget_37->setVisible(checked);
	if (checked)
	{
		ui.radioButton_2->setChecked(false);
		ui.radioButton_8->setChecked(false);
		ui.radioButton_12->setChecked(false);
		ui.radioButton->setChecked(false);

		//ui.widget_4->setVisible(Qt::Unchecked);
		//ui.widget_5->setVisible(Qt::Unchecked);
		ui.widget_46->setVisible(Qt::Unchecked);
		ui.widget_78->setVisible(Qt::Unchecked);
		ui.widget_47->setVisible(Qt::Unchecked);
		ui.widget_264->setVisible(Qt::Unchecked);

		//关闭advance
		ui.widget_72->setVisible(Qt::Unchecked);

		ui.lineEdit_56->setText("");
		ui.lineEdit_56->setEnabled(false);
		ui.lineEdit_56->setStyleSheet("QLineEdit{background-color: rgb(150, 150, 150);color: rgb(255, 255, 255);border :0px solid rgb(150, 150, 150); font: 8pt Ebrima;} ");

		ui.lineEdit_110->setText("");
		ui.lineEdit_110->setEnabled(false);
		ui.lineEdit_110->setStyleSheet("QLineEdit{background-color: rgb(150, 150, 150);color: rgb(255, 255, 255);border :0px solid rgb(150, 150, 150); font: 8pt Ebrima;} ");
	}
}

void UrbanConnectAnalyzer::on_radioButton_clicked(bool checked)
{
	ui.widget_264->setVisible(checked);
	if (checked)
	{
		//设置多线程
		ui.radioButton_3->setChecked(true);

		ui.radioButton_12->setChecked(false);

		ui.widget_47->setVisible(Qt::Unchecked);
		ui.radioButton_2->setChecked(false);
		ui.radioButton_8->setChecked(false);
		ui.widget_46->setVisible(Qt::Unchecked);
		ui.widget_78->setVisible(Qt::Unchecked);
		ui.widget_37->setVisible(Qt::Unchecked);

		ui.radioButton_21->setChecked(false);
		ui.radioButton_22->setChecked(false);
		ui.radioButton_23->setChecked(false);

		ui.lineEdit_56->setText("");
		ui.lineEdit_56->setEnabled(false);
		ui.lineEdit_56->setStyleSheet("QLineEdit{background-color: rgb(150, 150, 150);color: rgb(255, 255, 255);border :0px solid rgb(150, 150, 150); font: 8pt Ebrima;} ");

		ui.lineEdit_110->setEnabled(true);
		ui.lineEdit_110->setStyleSheet("QLineEdit{background-color: rgb(67, 74, 80);color: rgb(255, 255, 255);border :0px solid rgb(67, 74, 80); font: 8pt Ebrima;} ");
		ui.lineEdit_110->setFocus();//闪烁光标

		//关闭advance
		ui.widget_72->setVisible(Qt::Unchecked);
	}
	else {
		ui.lineEdit_110->setEnabled(false);
		ui.lineEdit_110->setStyleSheet("QLineEdit{background-color: rgb(150, 150, 150);color: rgb(255, 255, 255);border :0px solid rgb(150, 150, 150); font: 8pt Ebrima;} ");
	}
}

void UrbanConnectAnalyzer::on_radioButton_2_clicked(bool checked)
{
	ui.widget_46->setVisible(checked);

	if (checked)
	{
		//设置多线程
		ui.radioButton_3->setChecked(true);

		ui.radioButton->setChecked(false);	
		ui.radioButton_12->setChecked(false);

		ui.widget_47->setVisible(Qt::Unchecked);
		ui.widget_264->setVisible(Qt::Unchecked);
		ui.widget_78->setVisible(Qt::Unchecked);
		ui.widget_37->setVisible(Qt::Unchecked);

		//关闭advance
		ui.widget_72->setVisible(Qt::Unchecked);

		ui.radioButton_21->setChecked(false);
		ui.radioButton_22->setChecked(false);
		ui.radioButton_23->setChecked(false);

		ui.lineEdit_56->setText("");
		ui.lineEdit_56->setEnabled(false);
		ui.lineEdit_56->setStyleSheet("QLineEdit{background-color: rgb(150, 150, 150);color: rgb(255, 255, 255);border :0px solid rgb(150, 150, 150); font: 8pt Ebrima;} ");

		ui.lineEdit_110->setText("");
		ui.lineEdit_110->setEnabled(false);
		ui.lineEdit_110->setStyleSheet("QLineEdit{background-color: rgb(150, 150, 150);color: rgb(255, 255, 255);border :0px solid rgb(150, 150, 150); font: 8pt Ebrima;} ");
	}
}

void UrbanConnectAnalyzer::on_radioButton_8_clicked(bool checked)
{
	ui.widget_78->setVisible(checked);

	if (checked)
	{
		//设置多线程
		ui.radioButton_3->setChecked(true);

		ui.radioButton->setChecked(false);
		ui.radioButton_12->setChecked(false);

		ui.widget_47->setVisible(Qt::Unchecked);
		ui.widget_264->setVisible(Qt::Unchecked);
		ui.widget_46->setVisible(Qt::Unchecked);
		ui.widget_37->setVisible(Qt::Unchecked);

		//关闭advance
		ui.widget_72->setVisible(Qt::Unchecked);

		ui.radioButton_21->setChecked(false);
		ui.radioButton_22->setChecked(false);
		ui.radioButton_23->setChecked(false);

		ui.lineEdit_56->setText("");
		ui.lineEdit_56->setEnabled(false);
		ui.lineEdit_56->setStyleSheet("QLineEdit{background-color: rgb(150, 150, 150);color: rgb(255, 255, 255);border :0px solid rgb(150, 150, 150); font: 8pt Ebrima;} ");

		ui.lineEdit_110->setText("");
		ui.lineEdit_110->setEnabled(false);
		ui.lineEdit_110->setStyleSheet("QLineEdit{background-color: rgb(150, 150, 150);color: rgb(255, 255, 255);border :0px solid rgb(150, 150, 150); font: 8pt Ebrima;} ");
	}
}

void UrbanConnectAnalyzer::on_radioButton_36_clicked(bool checked)
{
	ui.widget_80->setVisible(checked);
	if (checked)
	{
		ui.radioButton_37->setChecked(false);
		ui.radioButton_50->setChecked(false);

		ui.widget_82->setVisible(Qt::Unchecked);
		ui.widget_89->setVisible(Qt::Unchecked);

	}
}

void UrbanConnectAnalyzer::on_radioButton_37_clicked(bool checked)
{
	ui.widget_82->setVisible(checked);
	if (checked)
	{
		ui.radioButton_36->setChecked(false);
		ui.radioButton_50->setChecked(false);


		ui.widget_80->setVisible(Qt::Unchecked);
		ui.widget_89->setVisible(Qt::Unchecked);

	}
}

void UrbanConnectAnalyzer::on_radioButton_50_clicked(bool checked)
{
	ui.widget_89->setVisible(checked);
	if (checked)
	{
		ui.radioButton_36->setChecked(false);
		ui.radioButton_37->setChecked(false);


		ui.widget_82->setVisible(Qt::Unchecked);
		ui.widget_80->setVisible(Qt::Unchecked);

	}
}

void UrbanConnectAnalyzer::on_radioButton_40_clicked(bool checked)
{
	ui.widget_168->setVisible(checked);
}


void UrbanConnectAnalyzer::on_radioButton_25_clicked(bool checked)
{
	ui.widget_49->setVisible(checked);
	if (checked)
	{
		ui.radioButton_26->setChecked(false);
		ui.radioButton_27->setChecked(false);
		ui.radioButton_28->setChecked(false);

		ui.widget_51->setVisible(Qt::Unchecked);
		ui.widget_55->setVisible(Qt::Unchecked);
		ui.widget_59->setVisible(Qt::Unchecked);
		 
	}
}

void UrbanConnectAnalyzer::on_radioButton_26_clicked(bool checked)
{
	ui.widget_51->setVisible(checked);
	if (checked)
	{
		ui.radioButton_25->setChecked(false);
		ui.radioButton_27->setChecked(false);
		ui.radioButton_28->setChecked(false);
		 

		ui.widget_49->setVisible(Qt::Unchecked);
		ui.widget_55->setVisible(Qt::Unchecked);
		ui.widget_59->setVisible(Qt::Unchecked);
		 
	}
}

void UrbanConnectAnalyzer::on_radioButton_27_clicked(bool checked)
{
	ui.widget_55->setVisible(checked);
	if (checked)
	{
		ui.radioButton_26->setChecked(false);
		ui.radioButton_25->setChecked(false);
		ui.radioButton_28->setChecked(false);
		 

		ui.widget_51->setVisible(Qt::Unchecked);
		ui.widget_49->setVisible(Qt::Unchecked);
		ui.widget_59->setVisible(Qt::Unchecked);
		 
	}
}

void UrbanConnectAnalyzer::on_radioButton_28_clicked(bool checked)
{
	ui.widget_59->setVisible(checked);
	if (checked)
	{
		ui.radioButton_26->setChecked(false);
		ui.radioButton_27->setChecked(false);
		ui.radioButton_25->setChecked(false);
		 

		ui.widget_51->setVisible(Qt::Unchecked);
		ui.widget_55->setVisible(Qt::Unchecked);
		ui.widget_49->setVisible(Qt::Unchecked);
		 
	}
}

void UrbanConnectAnalyzer::on_radioButton_74_clicked(bool checked)
{
	if (checked)
	{
		ui.radioButton_75->setChecked(false);
		ui.radioButton_76->setChecked(false);

		ui.widget_269->setVisible(Qt::Unchecked);
		ui.widget_271->setVisible(Qt::Unchecked);
	}
}
void UrbanConnectAnalyzer::on_radioButton_75_clicked(bool checked)
{
	ui.widget_269->setVisible(checked);
	if (checked)
	{
		ui.radioButton_76->setChecked(false);
		ui.radioButton_74->setChecked(false);

		ui.widget_271->setVisible(Qt::Unchecked);
	}
}

void UrbanConnectAnalyzer::on_radioButton_76_clicked(bool checked)
{
	ui.widget_271->setVisible(checked);
	if (checked)
	{
		ui.radioButton_75->setChecked(false);
		ui.radioButton_74->setChecked(false);

		ui.widget_269->setVisible(Qt::Unchecked);
	}
}

void UrbanConnectAnalyzer::on_radioButton_21_clicked(bool checked)
{
	//ui.widget_12->setVisible(checked);
	if (checked)
	{
		//设置单线程
		ui.radioButton_3->setChecked(false);

		ui.radioButton_22->setChecked(false);
		ui.radioButton_23->setChecked(false);

		ui.widget_41->setVisible(Qt::Unchecked);
		ui.widget_43->setVisible(Qt::Unchecked);

		ui.radioButton_2->setChecked(false);
		ui.radioButton_8->setChecked(false);
		ui.widget_46->setVisible(Qt::Unchecked);
		ui.widget_78->setVisible(Qt::Unchecked);
		ui.widget_47->setVisible(Qt::Unchecked);
		ui.radioButton->setChecked(false);
		ui.widget_264->setVisible(Qt::Unchecked);

	}
}

void UrbanConnectAnalyzer::on_radioButton_22_clicked(bool checked)
{
	//ui.widget_12->setVisible(checked);
	ui.widget_41->setVisible(checked);
	if (checked)
	{
		//设置单线程
		ui.radioButton_3->setChecked(false);

		ui.radioButton_21->setChecked(false);
		ui.radioButton_23->setChecked(false);

		ui.widget_43->setVisible(Qt::Unchecked);

		ui.radioButton_2->setChecked(false);
		ui.radioButton_8->setChecked(false);
		ui.widget_46->setVisible(Qt::Unchecked);
		ui.widget_78->setVisible(Qt::Unchecked);
		ui.widget_47->setVisible(Qt::Unchecked);
		ui.radioButton->setChecked(false);
		ui.widget_264->setVisible(Qt::Unchecked);
	}
}

void UrbanConnectAnalyzer::on_radioButton_23_clicked(bool checked)
{
	//ui.widget_12->setVisible(checked);
	ui.widget_43->setVisible(checked);
	if (checked)
	{
		//设置单线程
		ui.radioButton_3->setChecked(false);

		ui.radioButton_21->setChecked(false);
		ui.radioButton_22->setChecked(false);

		ui.widget_41->setVisible(Qt::Unchecked);

		ui.radioButton_2->setChecked(false);
		ui.radioButton_8->setChecked(false);
		ui.widget_46->setVisible(Qt::Unchecked);
		ui.widget_78->setVisible(Qt::Unchecked);
		ui.widget_47->setVisible(Qt::Unchecked);
		ui.radioButton->setChecked(false);
		ui.widget_264->setVisible(Qt::Unchecked);
	}
}


void UrbanConnectAnalyzer::on_radioButton_12_clicked(bool checked)
{
	ui.widget_47->setVisible(checked);

	if (checked)
	{
		//设置单线程
		ui.radioButton_3->setChecked(false);

		ui.widget_37->setVisible(Qt::Unchecked);

		ui.radioButton->setChecked(false);
		ui.widget_264->setVisible(Qt::Unchecked);
		ui.radioButton_2->setChecked(false);
		ui.radioButton_8->setChecked(false);
		ui.widget_46->setVisible(Qt::Unchecked);
		ui.widget_78->setVisible(Qt::Unchecked);

		ui.lineEdit_56->setEnabled(true);
		ui.lineEdit_56->setStyleSheet("QLineEdit{background-color: rgb(67, 74, 80);color: rgb(255, 255, 255);border :0px solid rgb(67, 74, 80); font: 8pt Ebrima;} ");
		ui.lineEdit_56->setFocus();//闪烁光标

		ui.lineEdit_110->setText("");
		ui.lineEdit_110->setEnabled(false);
		ui.lineEdit_110->setStyleSheet("QLineEdit{background-color: rgb(150, 150, 150);color: rgb(255, 255, 255);border :0px solid rgb(150, 150, 150); font: 8pt Ebrima;} ");

		//关闭advance
		ui.widget_72->setVisible(Qt::Unchecked);
	}
	else
	{
		ui.lineEdit_56->setText("");
		ui.lineEdit_56->setEnabled(false);
		ui.lineEdit_56->setStyleSheet("QLineEdit{background-color: rgb(150, 150, 150);color: rgb(255, 255, 255);border :0px solid rgb(150, 150, 150); font: 8pt Ebrima;} ");

		ui.lineEdit_110->setText("");
		ui.lineEdit_110->setEnabled(false);
		ui.lineEdit_110->setStyleSheet("QLineEdit{background-color: rgb(150, 150, 150);color: rgb(255, 255, 255);border :0px solid rgb(150, 150, 150); font: 8pt Ebrima;} ");
	}
}


void UrbanConnectAnalyzer::on_radioButton_3_clicked(bool checked)
{
	if (checked)
	{
		ui.lineEdit->setEnabled(true);
		ui.lineEdit->setStyleSheet("QLineEdit{background-color: rgb(67, 74, 80);color: rgb(255, 255, 255);border :0px solid rgb(67, 74, 80); font: 8pt Ebrima;} ");
		ui.lineEdit->setFocus();//闪烁光标
	}
	else
	{
		ui.lineEdit->setEnabled(false);
		ui.lineEdit->setStyleSheet("QLineEdit{background-color: rgb(150, 150, 150);color: rgb(255, 255, 255);border :0px solid rgb(150, 150, 150); font: 8pt Ebrima;} ");

	}
}

void UrbanConnectAnalyzer::on_pushButton_13_clicked(bool checked)
{
	ui.widget_44->setVisible(checked);
	ui.radioButton_24->setChecked(checked);
	ui.radioButton_29->setChecked(checked);
	ui.widget_134->setVisible(checked);
}



void UrbanConnectAnalyzer::on_radioButton_41_clicked(bool checked)
{
	ui.widget_120->setVisible(checked);
	if (checked)
	{

		//ui.radioButton_41->setChecked(false);
		ui.radioButton_42->setChecked(false);
		ui.radioButton_43->setChecked(false);
		ui.radioButton_44->setChecked(false);

		//ui.widget_120->setVisible(Qt::Unchecked);
		ui.widget_122->setVisible(Qt::Unchecked);
		ui.widget_126->setVisible(Qt::Unchecked);
		ui.widget_130->setVisible(Qt::Unchecked);
	}
}
void UrbanConnectAnalyzer::on_radioButton_42_clicked(bool checked)
{
	ui.widget_122->setVisible(checked);
	if (checked)
	{
		ui.radioButton_41->setChecked(false);
		//ui.radioButton_42->setChecked(false);
		ui.radioButton_43->setChecked(false);
		ui.radioButton_44->setChecked(false);


		ui.widget_120->setVisible(Qt::Unchecked);
		//ui.widget_122->setVisible(Qt::Unchecked);
		ui.widget_126->setVisible(Qt::Unchecked);
		ui.widget_130->setVisible(Qt::Unchecked);
	}
}
void UrbanConnectAnalyzer::on_radioButton_43_clicked(bool checked)
{
	ui.widget_126->setVisible(checked);
	if (checked)
	{

		ui.radioButton_41->setChecked(false);
		ui.radioButton_42->setChecked(false);
		//ui.radioButton_43->setChecked(false);
		ui.radioButton_44->setChecked(false);

		ui.widget_120->setVisible(Qt::Unchecked);
		ui.widget_122->setVisible(Qt::Unchecked);
		//ui.widget_126->setVisible(Qt::Unchecked);
		ui.widget_130->setVisible(Qt::Unchecked);
	}
}
void UrbanConnectAnalyzer::on_radioButton_44_clicked(bool checked)
{
	ui.widget_130->setVisible(checked);
	if (checked)
	{

		ui.radioButton_41->setChecked(false);
		ui.radioButton_42->setChecked(false);
		ui.radioButton_43->setChecked(false);
		//ui.radioButton_44->setChecked(false);

		ui.widget_120->setVisible(Qt::Unchecked);
		ui.widget_122->setVisible(Qt::Unchecked);
		ui.widget_126->setVisible(Qt::Unchecked);
		//ui.widget_130->setVisible(Qt::Unchecked);
	}


}

void UrbanConnectAnalyzer::on_radioButton_48_clicked(bool checked) {
	ui.widget_139->setVisible(checked);
	if (checked) {	//对于jncr需要自动覆盖参数
		if (ui.radioButton_43->isChecked())
			ui.lineEdit_77->setText(ui.lineEdit_53->text());
	}
}


void UrbanConnectAnalyzer::on_radioButton_30_clicked(bool checked) {
	ui.widget_144->setVisible(checked);
	if (checked) {	//对于dr、jncr需要自动覆盖参数
		if (ui.radioButton_22->isChecked())
			ui.lineEdit_79->setText(ui.lineEdit_29->text());
		else if (ui.radioButton_23->isChecked())
			ui.lineEdit_80->setText(ui.lineEdit_31->text());
	}
}

void UrbanConnectAnalyzer::on_radioButton_32_clicked(bool checked) {
	ui.widget_147->setVisible(checked);
	if (checked) {	//对于jncr需要自动覆盖参数
		//if (ui.radioButton_49->isChecked()) {
		//	ui.radioButton_32->setChecked(false);
		//	ui.widget_147->setVisible(Qt::Unchecked);
		//}

		if (ui.radioButton_27->isChecked())
			ui.lineEdit_83->setText(ui.lineEdit_41->text());
	}
}

void UrbanConnectAnalyzer::on_radioButton_4_clicked(bool checked) {
	if (checked) {
		ui.radioButton_7->setChecked(false);
	}
}

void UrbanConnectAnalyzer::on_radioButton_7_clicked(bool checked) {
	if (checked) {
		ui.radioButton_4->setChecked(false);
	}
}

void UrbanConnectAnalyzer::on_radioButton_5_clicked(bool checked) {
	if (checked) {
		ui.radioButton_6->setChecked(false);
		ui.radioButton_10->setChecked(false);
		ui.widget_152->setVisible(Qt::Unchecked);
	}
}

void UrbanConnectAnalyzer::on_radioButton_6_clicked(bool checked) {
	if (checked) {
		ui.radioButton_5->setChecked(false);
		ui.radioButton_10->setChecked(false);
		ui.widget_152->setVisible(Qt::Unchecked);
	}
}

void  UrbanConnectAnalyzer::on_radioButton_24_clicked(bool checked)
{
	ui.widget_134->setVisible(checked);
}

//多子集分析
void UrbanConnectAnalyzer::on_pushButton_23_clicked(bool checked) {
	ui.widget_72->setVisible(checked);
	if (checked) {
		//关闭analysis
		ui.radioButton_2->setChecked(false);
		ui.radioButton_8->setChecked(false);
		ui.radioButton_12->setChecked(false);
		ui.radioButton->setChecked(false);

		//ui.widget_4->setVisible(Qt::Unchecked);
		//ui.widget_5->setVisible(Qt::Unchecked);
		ui.widget_46->setVisible(Qt::Unchecked);
		ui.widget_78->setVisible(Qt::Unchecked);
		ui.widget_47->setVisible(Qt::Unchecked);
		ui.widget_264->setVisible(Qt::Unchecked);

		//关闭path
		ui.widget_37->setVisible(Qt::Unchecked);

		ui.lineEdit_56->setText("");
		ui.lineEdit_56->setEnabled(false);
		ui.lineEdit_56->setStyleSheet("QLineEdit{background-color: rgb(150, 150, 150);color: rgb(255, 255, 255);border :0px solid rgb(150, 150, 150); font: 8pt Ebrima;} ");

		ui.lineEdit_110->setText("");
		ui.lineEdit_110->setEnabled(false);
		ui.lineEdit_110->setStyleSheet("QLineEdit{background-color: rgb(150, 150, 150);color: rgb(255, 255, 255);border :0px solid rgb(150, 150, 150); font: 8pt Ebrima;} ");
	}
}

void UrbanConnectAnalyzer::on_radioButton_33_clicked(bool checked) {
	if (checked) {
		ui.radioButton_34->setChecked(false);
		ui.radioButton_35->setChecked(false);
	}
}

void UrbanConnectAnalyzer::on_radioButton_34_clicked(bool checked) {
	if (checked) {
		ui.radioButton_33->setChecked(false);
		ui.radioButton_35->setChecked(false);
	}
}

void UrbanConnectAnalyzer::on_radioButton_35_clicked(bool checked) {
	if (checked) {
		ui.radioButton_33->setChecked(false);
		ui.radioButton_34->setChecked(false);
	}
}

//advance
void UrbanConnectAnalyzer::on_radioButton_10_clicked(bool checked) {
	if (checked) {
		ui.widget_152->setVisible(Qt::Checked);

		ui.radioButton_5->setChecked(false);
		ui.radioButton_6->setChecked(false);
	}
	else {
		ui.widget_152->setVisible(Qt::Unchecked);
	}
}

//MR
void UrbanConnectAnalyzer::on_radioButton_11_clicked(bool checked) {
	if (checked) {
		//ui.radioButton_11->setChecked(false);
		ui.radioButton_13->setChecked(false);
		ui.radioButton_14->setChecked(false);
		ui.radioButton_15->setChecked(false);
	}
}

//DR
void UrbanConnectAnalyzer::on_radioButton_13_clicked(bool checked) {
	if (checked) {
		ui.radioButton_11->setChecked(false);
		//ui.radioButton_13->setChecked(false);
		ui.radioButton_14->setChecked(false);
		ui.radioButton_15->setChecked(false);
	}
}

//MDR
void UrbanConnectAnalyzer::on_radioButton_14_clicked(bool checked) {
	if (checked) {
		ui.radioButton_11->setChecked(false);
		ui.radioButton_13->setChecked(false);
		//ui.radioButton_14->setChecked(false);
		ui.radioButton_15->setChecked(false);
	}
}

//JnR
void UrbanConnectAnalyzer::on_radioButton_15_clicked(bool checked) {
	if (checked) {
		ui.radioButton_11->setChecked(false);
		ui.radioButton_13->setChecked(false);
		ui.radioButton_14->setChecked(false);
		//ui.radioButton_15->setChecked(false);
	}
}

void  UrbanConnectAnalyzer::on_radioButton_29_clicked(bool checked)
{
	ui.widget_138->setVisible(checked);

	if (checked)
	{
		//QColor color_red(0, 255, 0);
		//ui.colorPalette_2->setColor(color_red);

		//ui.lineEdit_32->setEnabled(true);
		//ui.lineEdit_32->setStyleSheet("QLineEdit{background-color: rgb(67, 74, 80);color: rgb(255, 255, 255);border :0px solid rgb(67, 74, 80); font: 8pt Ebrima;} ");
		//ui.lineEdit_32->setFocus();//闪烁光标
	}
	else
	{
		//ui.lineEdit_32->setEnabled(false);
		//ui.lineEdit_32->setStyleSheet("QLineEdit{background-color: rgb(150, 150, 150);color: rgb(255, 255, 255);border :0px solid rgb(150, 150, 150); font: 8pt Ebrima;} ");

	}
}

////打开窗体
//void  UrbanConnectAnalyzer::on_pushButton_2_clicked(bool checked)
//{
//	extern QVector<QString> AttributesVec;
//
//	QMap<QString, bool> result;
//
//	son_window = new newwindow;//指针实例化
//	son_window->setWindowTitle(QString("Select Attributes"));
//	son_window->move(900, 30);	//设置窗口位置
//
//	connect(son_window, SIGNAL(infoSend(QString)), this, SLOT(infoRecv(QString)));
//
//	son_window->show(); 
//	son_window->update_win(AttributesVec);
//}
//
////获得结果
//void UrbanConnectAnalyzer::infoRecv(QString sInfo)
//{
//	QString saveas;
//	QFilePath path(windowFilePath());
//	//   saveas = path.m_path + (path.m_name.isEmpty() ? windowTitle() : path.m_name);
//	saveas = path.m_path + tr("Attributes.csv");
//
//	QString template_string = tr("CSV file (*.csv)\nTXT file (*.txt)\nAll files (*.*)");
//
//	QFileDialog::Options options = 0;
//	QString selectedFilter;
//	QString outfile = QFileDialog::getSaveFileName(
//		0, tr("Save Attributes File to"),
//		saveas,
//		template_string,
//		&selectedFilter,
//		options);
//
//	if (outfile.isEmpty())
//		return;
//
//	//1234 表示第1 2 3 4个属性被选中了，其他没选中
//	QString aaa = sInfo;
//
//	extern std::vector<int> SelectedAttributesIndex;
//	SelectedAttributesIndex.clear();
//
//	for (int i = 0; i < sInfo.size(); i++) {
//		QString tmp = (QString)(sInfo[i]);
//		std::string str= std::string((const char *)tmp.toLocal8Bit());
//		SelectedAttributesIndex.push_back(int(str[0]));	
//	}
//
//	extern QVector<QString> AttributesVec;
//	extern std::string AttributesFileName;
//
//	//判断是否输出文件
//	if (!AttributesVec.empty() && !SelectedAttributesIndex.empty() && AttributesFileName.length()) {
//
//		std::ifstream infile(AttributesFileName, std::ios::in);
//		std::string str, tmp;
//		getline(infile, str);
//		std::stringstream input(str);
//
//		//对需要输出的列id进行收集
//		std::unordered_set<std::string> AttributesSet;
//		std::unordered_set<int> PosSet;
//
//		for (int i = 0; i < SelectedAttributesIndex.size(); i++) {
//			std::string str= std::string((const char *)(AttributesVec[SelectedAttributesIndex[i]]).toLocal8Bit());
//			AttributesSet.insert(str);
//		}			
//
//		//输出文件名
//		//std::string outFileName = "Attributes.csv";
//		std::string outFileName = std::string((const char *)outfile.toLocal8Bit());
//		std::ofstream out;
//		out.open(outFileName);
//
//		int pos = 0;
//		std::string LineStr;
//		while (getline(input, tmp, '\t')) {
//			if (AttributesSet.count(tmp)) {
//				LineStr += tmp + ",";
//				PosSet.insert(pos);
//			}				
//			if (tmp == "ID") {
//				LineStr += tmp + ",";
//				PosSet.insert(pos);
//			}				
//			++pos;
//		}
//		LineStr = LineStr.substr(0, LineStr.length() - 1);
//		out << LineStr << std::endl;
//
//		while (getline(infile, str)) {
//			std::stringstream input(str);
//
//			LineStr = "";
//			int pos = 0;
//			while (getline(input, tmp, '\t')) {
//				if (PosSet.count(pos)) {
//					LineStr += tmp + ",";
//				}
//				++pos;
//			}
//			LineStr = LineStr.substr(0, LineStr.length() - 1);
//			out << LineStr << std::endl;
//		}
//
//		infile.close();
//		out.close();
//	}
//}

//void UrbanConnectAnalyzer::on_pushButton_20_clicked(bool checked) {
//	this->highlight_window = new Highlight;
//	this->highlight_window->show();
//}

void UrbanConnectAnalyzer::on_pushButton_19_clicked() {
	emit(ui.colorBackground->clicked());
}

