/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "OpmlImporter.h"
#include "OpmlImporterWidget.h"
#include "../../../core/FeedsManager.h"

#include <QtCore/QDir>
#include <QtCore/QFile>

namespace Otter
{

OpmlImporter::OpmlImporter(QObject *parent) : Importer(parent),
	m_optionsWidget(nullptr)
{
}

void OpmlImporter::importFolder(FeedsModel::Entry *source, FeedsModel::Entry *target, bool areDuplicatesAllowed)
{
	for (int i = 0; i < source->rowCount(); ++i)
	{
		FeedsModel::Entry *sourceEntry(static_cast<FeedsModel::Entry*>(source->child(i)));

		if (!sourceEntry)
		{
			continue;
		}

		const FeedsModel::EntryType type(static_cast<FeedsModel::EntryType>(sourceEntry->data(FeedsModel::TypeRole).toInt()));

		if (type != FeedsModel::FeedEntry && type != FeedsModel::FolderEntry)
		{
			continue;
		}

		switch (type)
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

QWidget* OpmlImporter::createOptionsWidget(QWidget *parent)
{
	if (!m_optionsWidget)
	{
		m_optionsWidget = new OpmlImporterWidget(parent);
	}

	return m_optionsWidget;
}

QString OpmlImporter::getTitle() const
{
	return tr("OPML Feeds");
}

QString OpmlImporter::getDescription() const
{
	return tr("Imports feeds from OPML file");
}

QString OpmlImporter::getVersion() const
{
	return QLatin1String("1.0");
}

QString OpmlImporter::getSuggestedPath(const QString &path) const
{
	if (!path.isEmpty() && QFileInfo(path).isDir())
	{
		return QDir(path).filePath(QLatin1String("feeds.opml"));
	}

	return path;
}

QString OpmlImporter::getBrowser() const
{
	return QLatin1String("other");
}

QUrl OpmlImporter::getHomePage() const
{
	return QUrl(QLatin1String("https://otter-browser.org/"));
}

QStringList OpmlImporter::getFileFilters() const
{
	return {tr("OPML files (*.opml)")};
}

Importer::ImportType OpmlImporter::getImportType() const
{
	return FeedsImport;
}

bool OpmlImporter::import(const QString &path)
{
	QFile file(getSuggestedPath(path));

	if (!file.open(QIODevice::ReadOnly))
	{
		emit importFinished(FeedsImport, FailedImport, 0);

		return false;
	}

	emit importStarted(FeedsImport, -1);

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

	emit importFinished(FeedsImport, SuccessfullImport, totalAmount);

	file.close();

	return true;
}

}
