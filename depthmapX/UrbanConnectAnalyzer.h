#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_UrbanConnectAnalyzer.h"
#include "ui_newwindow.h"
#include "newwindow.h"

#include "ui_Highlight.h"
#include "Highlight.h"

class UrbanConnectAnalyzer : public QMainWindow
{
	Q_OBJECT

public:
	UrbanConnectAnalyzer(QWidget *parent = Q_NULLPTR);
	Ui::UrbanConnectAnalyzer ui;
	newwindow* son_window;

	Highlight* highlight_window;

private slots:
	void on_pushButton_5_clicked(bool checked);
	void on_pushButton_7_clicked(bool checked);

	//可视化按钮
	void on_pushButton_10_clicked(bool checked);
	void on_pushButton_15_clicked(bool checked);
	void on_pushButton_16_clicked(bool checked);
	void on_pushButton_17_clicked(bool checked);
	void on_pushButton_4_clicked(bool checked);
	void on_pushButton_3_clicked(bool checked);

	void on_radioButton_clicked(bool checked);
	void on_radioButton_2_clicked(bool checked);
	void on_radioButton_8_clicked(bool checked);
	void on_pushButton_12_clicked(bool checked);
			
	void on_radioButton_36_clicked(bool checked);
	void on_radioButton_37_clicked(bool checked);
	void on_radioButton_50_clicked(bool checked);
	void on_radioButton_40_clicked(bool checked);

	void on_radioButton_25_clicked(bool checked);
	void on_radioButton_26_clicked(bool checked);
	void on_radioButton_27_clicked(bool checked);
	void on_radioButton_28_clicked(bool checked);

	void on_radioButton_74_clicked(bool checked);
	void on_radioButton_75_clicked(bool checked);
	void on_radioButton_76_clicked(bool checked);

	void on_radioButton_21_clicked(bool checked);
	void on_radioButton_22_clicked(bool checked);
	void on_radioButton_23_clicked(bool checked);

	void on_radioButton_12_clicked(bool checked);
	void on_radioButton_3_clicked(bool checked);
	void on_pushButton_13_clicked(bool checked);


	void on_radioButton_41_clicked(bool checked);
	void on_radioButton_42_clicked(bool checked);
	void on_radioButton_43_clicked(bool checked);
	void on_radioButton_44_clicked(bool checked);


	void on_radioButton_24_clicked(bool checked);
	void on_radioButton_29_clicked(bool checked);

	//void on_pushButton_2_clicked(bool checked);
	
	//configuration
	void on_radioButton_48_clicked(bool checked);
	void on_radioButton_30_clicked(bool checked);
	void on_radioButton_32_clicked(bool checked);

	//from-to
	void on_radioButton_4_clicked(bool checked);
	void on_radioButton_7_clicked(bool checked);

	//layers
	void on_radioButton_5_clicked(bool checked);
	void on_radioButton_6_clicked(bool checked);

	////Highlight Display
	//void on_pushButton_20_clicked(bool checked);

	//背景颜色
	void on_pushButton_19_clicked();

	//多子集分析
	void on_pushButton_23_clicked(bool checked);
	void on_radioButton_33_clicked(bool checked);
	void on_radioButton_34_clicked(bool checked);
	void on_radioButton_35_clicked(bool checked);

	//advance analysis
	void on_radioButton_10_clicked(bool checked);	//advance
	void on_radioButton_11_clicked(bool checked);	//MR
	void on_radioButton_13_clicked(bool checked);	//DR
	void on_radioButton_14_clicked(bool checked);	//MDR
	void on_radioButton_15_clicked(bool checked);	//JnR

//public slots:
//	void infoRecv(QString);

};
