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

#include "AddressCompletionModel.h"
#include "BookmarksManager.h"
#include "SettingsManager.h"

#include <QtCore/QCoreApplication>

namespace Otter
{

AddressCompletionModel* AddressCompletionModel::m_instance = NULL;

AddressCompletionModel::AddressCompletionModel(QObject *parent) : QAbstractListModel(parent),
	m_updateTimer(0)
{
	m_updateTimer = startTimer(250);

	connect(BookmarksManager::getInstance(), SIGNAL(modelModified()), this, SLOT(updateCompletion()));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString)));
}

void AddressCompletionModel::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_updateTimer)
	{
		killTimer(m_updateTimer);

		m_updateTimer = 0;

		QList<QUrl> urls;
		urls << QUrl(QLatin1String("about:bookmarks")) << QUrl(QLatin1String("about:cache")) << QUrl(QLatin1String("about:config")) << QUrl(QLatin1String("about:cookies")) << QUrl(QLatin1String("about:history")) << QUrl(QLatin1String("about:notes")) << QUrl(QLatin1String("about:transfers"));
		urls << BookmarksManager::getUrls();

		beginResetModel();

		m_urls = urls;

		endResetModel();
	}
}

void AddressCompletionModel::optionChanged(const QString &option)
{
	if (option.contains(QLatin1String("AddressField/Suggest")))
	{
		updateCompletion();
	}
}

void AddressCompletionModel::updateCompletion()
{
	if (m_updateTimer == 0)
	{
		m_updateTimer = startTimer(250);
	}
}

AddressCompletionModel* AddressCompletionModel::getInstance()
{
	if (!m_instance)
	{
		m_instance = new AddressCompletionModel(QCoreApplication::instance());
	}

	return m_instance;
}

QVariant AddressCompletionModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::DisplayRole && index.column() == 0 && index.row() >= 0 && index.row() < m_urls.count())
	{
		return m_urls.at(index.row());
	}

	return QVariant();
}

QVariant AddressCompletionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	Q_UNUSED(section)
	Q_UNUSED(orientation)
	Q_UNUSED(role)

	return QVariant();
}

int AddressCompletionModel::rowCount(const QModelIndex &index) const
{
	return (index.isValid() ? 0 : m_urls.count());
}

}
