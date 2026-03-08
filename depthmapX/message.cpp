#include "message.h"
#include <QtWidgets/QApplication>
#include <QtCore>

JasonQt_ShowInformationMessageBoxFromOtherThread::JasonQt_ShowInformationMessageBoxFromOtherThread(const QString &title, const QString &message) :
	m_title(title),
	m_message(message)
{ }

void JasonQt_ShowInformationMessageBoxFromOtherThread::show(const QString &title, const QString &message)
{
	QEventLoop eventLoop;
	auto messageBox = new JasonQt_ShowInformationMessageBoxFromOtherThread(title, message);
	connect(messageBox, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));
	messageBox->readyShow();
	eventLoop.exec();
}

void JasonQt_ShowInformationMessageBoxFromOtherThread::readyShow(void)
{
	this->moveToThread(qApp->thread());
	QTimer::singleShot(0, this, SLOT(onShow()));
}

void JasonQt_ShowInformationMessageBoxFromOtherThread::onShow(void)
{
	QMessageBox::information(NULL, m_title, m_message);
	this->deleteLater();
}