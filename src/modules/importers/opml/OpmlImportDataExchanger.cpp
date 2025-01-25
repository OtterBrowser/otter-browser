/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "OpmlImportDataExchanger.h"
#include "OpmlImportOptionsWidget.h"
#include "../../../core/FeedsManager.h"

#include <QtCore/QDir>
#include <QtCore/QFile>

namespace Otter
{

OpmlImportDataExchanger::OpmlImportDataExchanger(QObject *parent) : ImportDataExchanger(parent),
	m_optionsWidget(nullptr)
{
}

void OpmlImportDataExchanger::importFolder(FeedsModel::Entry *source, FeedsModel::Entry *target, bool areDuplicatesAllowed)
{
	for (int i = 0; i < source->rowCount(); ++i)
	{
		FeedsModel::Entry *sourceEntry(source->getChild(i));

		if (!sourceEntry)
		{
			continue;
		}

		switch (static_cast<FeedsModel::EntryType>(sourceEntry->data(FeedsModel::TypeRole).toInt()))
		{
			case FeedsModel::FeedEntry:
				if (sourceEntry->getFeed() && (areDuplicatesAllowed || !FeedsManager::getModel()->hasFeed(sourceEntry->getFeed()->getUrl())))
				{
					FeedsManager::getModel()->addEntry(sourceEntry->getFeed(), target);
				}

				break;
			case FeedsModel::FolderEntry:
				importFolder(sourceEntry, FeedsManager::getModel()->addEntry(FeedsModel::FolderEntry, {{FeedsModel::TitleRole, sourceEntry->data(FeedsModel::TitleRole)}}, target), areDuplicatesAllowed);

				break;
			default:
				break;
		}
	}
}

QWidget* OpmlImportDataExchanger::createOptionsWidget(QWidget *parent)
{
	if (!m_optionsWidget)
	{
		m_optionsWidget = new OpmlImportOptionsWidget(parent);
	}

	return m_optionsWidget;
}

QString OpmlImportDataExchanger::getName() const
{
	return QLatin1String("opml");
}

QString OpmlImportDataExchanger::getTitle() const
{
	return tr("Import OPML Feeds");
}

QString OpmlImportDataExchanger::getDescription() const
{
	return tr("Imports feeds from OPML file");
}

QString OpmlImportDataExchanger::getVersion() const
{
	return QLatin1String("1.0");
}

QString OpmlImportDataExchanger::getSuggestedPath(const QString &path) const
{
	if (!path.isEmpty() && QFileInfo(path).isDir())
	{
		return QDir(path).filePath(QLatin1String("feeds.opml"));
	}

	return path;
}

QString OpmlImportDataExchanger::getGroup() const
{
	return QLatin1String("other");
}

QUrl OpmlImportDataExchanger::getHomePage() const
{
	return QUrl(QLatin1String("https://otter-browser.org/"));
}

QStringList OpmlImportDataExchanger::getFileFilters() const
{
	return {tr("OPML files (*.opml)")};
}

DataExchanger::ExchangeType OpmlImportDataExchanger::getExchangeType() const
{
	return FeedsExchange;
}

bool OpmlImportDataExchanger::hasOptions() const
{
	return true;
}

bool OpmlImportDataExchanger::importData(const QString &path)
{
	QFile file(getSuggestedPath(path));

	if (!file.open(QIODevice::ReadOnly))
	{
		emit exchangeFinished(FeedsExchange, FailedOperation, 0);

		return false;
	}

	emit exchangeStarted(FeedsExchange, -1);

	FeedsModel *model(new FeedsModel(path, this));
	const int estimatedAmount((file.size() > 0) ? static_cast<int>(file.size() / 250) : 0);
	int totalAmount(0);

	if (model->getRootEntry()->rowCount() > 0)
	{
		FeedsModel::Entry *importFolderEntry(m_optionsWidget->getTargetFolder());

		FeedsManager::getModel()->beginImport(importFolderEntry, estimatedAmount);

		importFolder(model->getRootEntry(), importFolderEntry, (m_optionsWidget ? m_optionsWidget->areDuplicatesAllowed() : true));

		FeedsManager::getModel()->endImport();
	}

	model->deleteLater();

	emit exchangeFinished(FeedsExchange, SuccessfullOperation, totalAmount);

	file.close();

	return true;
}

}
