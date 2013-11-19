#include "SessionsManagerDialog.h"
#include "../core/SessionsManager.h"

#include "ui_SessionsManagerDialog.h"

#include <QtWidgets/QTableWidgetItem>

namespace Otter
{

SessionsManagerDialog::SessionsManagerDialog(QWidget *parent) : QDialog(parent),
	m_ui(new Ui::SessionsManagerDialog)
{
	m_ui->setupUi(this);

	const QStringList sessions = SessionsManager::getSessions();
	QMultiHash<QString, SessionInformation> information;

	for (int i = 0; i < sessions.count(); ++i)
	{
		const SessionInformation session = SessionsManager::getSession(sessions.at(i));

		information.insert((session.title.isEmpty() ? tr("(Untitled)") : session.title), session);
	}

	const QList<SessionInformation> sorted = information.values();
	const QString currentSession = SessionsManager::getCurrentSession();
	int index = 0;

	m_ui->sessionsWidget->setRowCount(sorted.count());

	for (int i = 0; i < sorted.count(); ++i)
	{
		int windows = 0;

		for (int j = 0; j < sorted.at(i).windows.count(); ++j)
		{
			windows += sorted.at(i).windows.at(j).windows.count();
		}

		if (sorted.at(i).path == currentSession)
		{
			index = i;
		}

		m_ui->sessionsWidget->setItem(i, 0, new QTableWidgetItem(sorted.at(i).title.isEmpty() ? tr("(Untitled)") : sorted.at(i).title));
		m_ui->sessionsWidget->setItem(i, 1, new QTableWidgetItem(sorted.at(i).path));
		m_ui->sessionsWidget->setItem(i, 2, new QTableWidgetItem(QString("%1 (%2)").arg(sorted.at(i).windows.count()).arg(windows)));
	}

	m_ui->sessionsWidget->setCurrentCell(index, 0);
}

SessionsManagerDialog::~SessionsManagerDialog()
{
	delete m_ui;
}

void SessionsManagerDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			break;
		default:
			break;
	}
}

}
