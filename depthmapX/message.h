#pragma once
#include <QMessageBox>
#include <QEventLoop>

class JasonQt_ShowInformationMessageBoxFromOtherThread : public QObject
{
	Q_OBJECT

private:
	const QString m_title;
	const QString m_message;

public:
	JasonQt_ShowInformationMessageBoxFromOtherThread(const QString &title, const QString &message);

	static void show(const QString &title, const QString &message);

private:
	void readyShow(void);

private slots:
	void onShow(void);
};