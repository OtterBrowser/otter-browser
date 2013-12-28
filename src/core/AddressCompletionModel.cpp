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

	connect(BookmarksManager::getInstance(), SIGNAL(folderModified(int)), this, SLOT(updateCompletion()));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString)));
}

void AddressCompletionModel::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_updateTimer)
	{
		killTimer(m_updateTimer);

		m_updateTimer = 0;

		QStringList urls;
		urls << QLatin1String("about:bookmarks") << QLatin1String("about:cache") << QLatin1String("about:config") << QLatin1String("about:cookies") << QLatin1String("about:history") << QLatin1String("about:transfers");
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
