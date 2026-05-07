#include "newwindow.h"
#include "qradiobutton.h"
#include <QVBoxLayout> 
#include <qdebug.h>
#include "qpushbutton.h"
#include <QButtonGroup>
#include <QScrollArea>


newwindow::newwindow(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	//this->resize(400, 300);  //定义窗口大小

}

QWidget * newwindow::setupAttributesListWidget(QVector<QString> ggg) {
	QWidget *widget = new QWidget(this);

	QLayout *vlayout = new QVBoxLayout(widget);
	vlayout->setMargin(1);

	return widget;
}

void newwindow::update_win(QVector<QString> ggg)
{
	number = ggg.size();

	QVBoxLayout *verticalLayout_all = new QVBoxLayout(this);//最大的布局
	QVBoxLayout *verticalLayout_top = new QVBoxLayout(this);//最上半部分的布局
	QVBoxLayout *verticalLayout_bottom = new QVBoxLayout(this);//下半部分的布局

	QWidget  *top_widget = new QWidget(this);//上窗口
	QWidget  *bottom_widget = new QWidget(this);//上窗口

	//上半部分操作
	top_widget->setLayout(verticalLayout_top);
	verticalLayout_top->setSpacing(1);//button与button之间距离
	verticalLayout_top->setContentsMargins(1, 4, 5, 6);//与内部边框距离
	for (int i = 0; i < ggg.size(); i++)
	{
		QWidget  *top_small_widget = new QWidget(this);
		top_small_widget->setStyleSheet("QWidget{background-color: rgb(104, 104, 104);border-top: 1px solid rgb(0, 0, 0);border-bottom: 1px solid transparent; }");
		top_small_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

		QRadioButton* my_button = new QRadioButton(this);//ggg[i]
		my_button->setText(ggg[i]);
		my_button->setStyleSheet("QRadioButton{font: 8pt Ebrima; color:white;background-color: rgb(104, 104, 104);padding-left:18px;padding-bottom:2 px;padding-top:2 px; }"
			"QRadioButton:hover{background-color: rgb(180, 53, 180);}"
			"QRadioButton:pressed{background-color: rgb(180, 53, 180);}");

		my_button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		my_vector.push_back(my_button);
		connect(my_button, SIGNAL(toggled(bool)), this, SLOT(get_checked(bool)));

		QVBoxLayout *verticalLayout_top_small = new QVBoxLayout(this);
		verticalLayout_top_small->setContentsMargins(0, 0, 0, 0);
		verticalLayout_top_small->addWidget(my_button);

		top_small_widget->setLayout(verticalLayout_top_small);
		verticalLayout_top->addWidget(top_small_widget);
	}

	//***********************************************************
		//下半部分操作
	verticalLayout_bottom->setContentsMargins(10, 10, 10, 10);
	bottom_widget->setLayout(verticalLayout_bottom);

	QWidget  *bottom_small_widget = new QWidget();
	bottom_small_widget->setStyleSheet("QWidget{background-color: rgb(104, 104, 104);border-top: 1px solid rgb(0, 0, 0);border-bottom: 1px solid transparent; }");
	bottom_small_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	QRadioButton* my_button = new QRadioButton("select all");
	my_button->setStyleSheet("QRadioButton{font: 8pt Ebrima; color:white;background-color: rgb(104, 104, 104);padding-left:18px;padding-bottom:2 px;padding-top:2 px; }"
		"QRadioButton:hover{background-color: rgb(180, 53, 180);}"
		"QRadioButton:pressed{background-color: rgb(180, 53, 180);}");

	my_button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	QVBoxLayout *verticalLayout_bottom_small = new QVBoxLayout();
	verticalLayout_bottom_small->setContentsMargins(0, 0, 0, 0);
	verticalLayout_bottom_small->addWidget(my_button);
	bottom_small_widget->setLayout(verticalLayout_bottom_small);
	connect(my_button, SIGNAL(toggled(bool)), this, SLOT(get_all_checked(bool)));

	QPushButton* my_button_ok = new QPushButton("save");
	my_button_ok->setStyleSheet("QPushButton{font: 10pt Ebrima; color:white;background-color: rgb(104, 104, 104);padding-left:18px;padding-bottom:2 px;padding-top:2 px; }"
		"QPushButton:hover{background-color: rgb(180, 53, 180);}"
		"QPushButton:pressed{background-color: rgb(180, 53, 180);}");
	connect(my_button_ok, SIGNAL(clicked(bool)), this, SLOT(sendData()));

	verticalLayout_bottom->addWidget(bottom_small_widget);
	verticalLayout_bottom->addWidget(my_button_ok);


	//***************************************************************************
	//最大布局
	verticalLayout_all->setSpacing(40);//上下两部分之间的距离
	verticalLayout_all->setContentsMargins(2, 2, 2, 2);//和内部窗体间距
	verticalLayout_all->addWidget(top_widget);
	verticalLayout_all->addWidget(bottom_widget);
	verticalLayout_all->addStretch();
	verticalLayout_all->setMargin(1);
	this->setLayout(verticalLayout_all);
}

newwindow::~newwindow()
{
}
void newwindow::get_checked(bool toggled)
{
	QRadioButton *RBtn = qobject_cast<QRadioButton*>(sender()); //得到当前信号来源的对象    
	for(int i=0;i< number;i++)
	{        
		if (toggled == true && RBtn == my_vector[i])//选中   
		{
				qDebug("RadioButton%d is selected",i);  
				result[i]=true ;
		}
		if (toggled == false && RBtn == my_vector[i])//取消选中     
		{
				qDebug("RadioButton%d is abort selected", i);
				result[i, false];
		}
	}
}


void newwindow::get_all_checked(bool toggled)
{
	if (toggled == true)//选中   
	{
		for (int i = 0; i < number; i++)
		{
			result[i] = true;
			my_vector[i]->setChecked(true);
		}
	}
	if (toggled == false)//取消选中     
	{
		for (int i = 0; i < number; i++)
		{
			result[i] = false;
			my_vector[i]->setChecked(false);
		}
	}
}
void newwindow::sendData()
{
	this->close();
	QString sInfo;
	for (QMap<int ,bool>::iterator iter  = result.begin(); iter != result.end(); iter++)
			sInfo += iter.key();
		emit infoSend(sInfo); 
}

