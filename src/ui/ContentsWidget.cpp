/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "ContentsWidget.h"
#include "ContentsDialog.h"

namespace Otter
{

ContentsWidget::ContentsWidget(Window *window) : QWidget(window)
{
	if (window)
	{
		connect(window, SIGNAL(aboutToClose()), this, SLOT(close()));
	}
}

void ContentsWidget::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);

	for (int i = 0; i < m_dialogs.count(); ++i)
	{
		if (m_dialogs.at(i))
		{
			m_dialogs.at(i)->raise();
			m_dialogs.at(i)->adjustSize();
			m_dialogs.at(i)->move(geometry().center() - QRect(QPoint(0, 0), m_dialogs.at(i)->size()).center());
		}
	}
}

void ContentsWidget::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);

	for (int i = 0; i < m_dialogs.count(); ++i)
	{
		if (m_dialogs.at(i))
		{
			m_dialogs.at(i)->raise();
			m_dialogs.at(i)->move(geometry().center() - QRect(QPoint(0, 0), m_dialogs.at(i)->size()).center());
		}
	}
}

void ContentsWidget::triggerAction(int identifier, const QVariantMap &parameters)
{
	Q_UNUSED(identifier)
	Q_UNUSED(parameters)
}

void ContentsWidget::triggerAction()
{
	Action *action = qobject_cast<Action*>(sender());

	if (action)
	{
		QVariantMap parameters;

		if (action->isCheckable())
		{
			parameters[QLatin1String("isChecked")] = action->isChecked();
		}

		triggerAction(action->getIdentifier(), parameters);
	}
}

void ContentsWidget::close()
{
	for (int i = 0; i < m_dialogs.count(); ++i)
	{
		if (m_dialogs.at(i))
		{
			m_dialogs.at(i)->close();
		}
	}
}

void ContentsWidget::lockContents()
{
}

void ContentsWidget::unlockContents()
{
}

void ContentsWidget::cleanupDialog()
{
	ContentsDialog *dialog = qobject_cast<ContentsDialog*>(sender());

	if (dialog)
	{
		m_dialogs.removeAll(dialog);

		dialog->hide();
		dialog->deleteLater();
	}

	if (m_dialogs.isEmpty())
	{
		unlockContents();
	}
}

void ContentsWidget::showDialog(ContentsDialog *dialog, bool lockEventLoop)
{
	if (!dialog)
	{
		return;
	}

	Window *window = qobject_cast<Window*>(parent());

	if (window && window->isAboutToClose())
	{
		dialog->deleteLater();

		return;
	}

	lockContents();

	m_dialogs.append(dialog);

	connect(dialog, SIGNAL(finished()), this, SLOT(cleanupDialog()));

	dialog->setParent(this);
	dialog->show();
	dialog->raise();
	dialog->setFocus();
	dialog->move(geometry().center() - QRect(QPoint(0, 0), dialog->size()).center());

	if (lockEventLoop)
	{
		QEventLoop eventLoop;

		connect(dialog, SIGNAL(finished()), &eventLoop, SLOT(quit()));
		connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

		eventLoop.exec();
	}
}

void ContentsWidget::goToHistoryIndex(int index)
{
	Q_UNUSED(index)
}

void ContentsWidget::setHistory(const WindowHistoryInformation &history)
{
	Q_UNUSED(history)
}

void ContentsWidget::setZoom(int zoom)
{
	Q_UNUSED(zoom)
}

void ContentsWidget::setUrl(const QUrl &url, bool typed)
{
	Q_UNUSED(url)
	Q_UNUSED(typed)
}

void ContentsWidget::setParent(Window *window)
{
	if (window)
	{
		connect(window, SIGNAL(aboutToClose()), this, SLOT(close()));
	}

	QWidget::setParent(window);
}

ContentsWidget* ContentsWidget::clone(bool cloneHistory)
{
	Q_UNUSED(cloneHistory)

	return NULL;
}

Action* ContentsWidget::getAction(int identifier)
{
	Q_UNUSED(identifier)

	return NULL;
}

Window* ContentsWidget::getParent()
{
	return qobject_cast<Window*>(parent());
}

QString ContentsWidget::getVersion() const
{
	return QCoreApplication::applicationVersion();
}

QString ContentsWidget::getStatusMessage() const
{
	return QString();
}

QPixmap ContentsWidget::getThumbnail() const
{
	return QPixmap();
}

WindowHistoryInformation ContentsWidget::getHistory() const
{
	WindowHistoryEntry entry;
	entry.url = getUrl().toString();
	entry.title = getTitle();
	entry.position = QPoint(0, 0);
	entry.zoom = 100;

	WindowHistoryInformation information;
	information.index = 0;
	information.entries.append(entry);

	return information;
}

QList<LinkUrl> ContentsWidget::getFeeds() const
{
	return QList<LinkUrl>();
}

QList<LinkUrl> ContentsWidget::getSearchEngines() const
{
	return QList<LinkUrl>();
}

int ContentsWidget::getZoom() const
{
	return 100;
}

bool ContentsWidget::canClone() const
{
	return false;
}

bool ContentsWidget::canZoom() const
{
	return false;
}

bool ContentsWidget::isLoading() const
{
	return false;
}

bool ContentsWidget::isPrivate() const
{
	return false;
}

}
