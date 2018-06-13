/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2014 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "OperaNotesImporter.h"
#include "../../../core/NotesManager.h"
#include "../../../ui/BookmarksComboBoxWidget.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QStandardPaths>
#include <QtCore/QTextStream>
#include <QtWidgets/QFormLayout>

namespace Otter
{

OperaNotesImporter::OperaNotesImporter(QObject *parent) : Importer(parent),
	m_folderComboBox(nullptr),
	m_currentFolder(NotesManager::getModel()->getRootItem()),
	m_importFolder(NotesManager::getModel()->getRootItem()),
	m_optionsWidget(nullptr)
{
}

QWidget* OperaNotesImporter::createOptionsWidget(QWidget *parent)
{
	if (!m_optionsWidget)
	{
		m_optionsWidget = new QWidget(parent);

		QFormLayout *layout(new QFormLayout(m_optionsWidget));
		layout->setMargin(0);

		m_optionsWidget->setLayout(layout);

		m_folderComboBox = new BookmarksComboBoxWidget(m_optionsWidget);
		m_folderComboBox->setMode(BookmarksModel::NotesMode);

		layout->addRow(tr("Import into folder:"), m_folderComboBox);
	}

	return m_optionsWidget;
}

QString OperaNotesImporter::getTitle() const
{
	return tr("Opera Notes");
}

QString OperaNotesImporter::getDescription() const
{
	return tr("Imports notes from Opera Browser version 12 or earlier");
}

QString OperaNotesImporter::getVersion() const
{
	return QLatin1String("0.8");
}

QString OperaNotesImporter::getSuggestedPath(const QString &path) const
{
	if (!path.isEmpty())
	{
		if (QFileInfo(path).isDir())
		{
			return QDir(path).filePath(QLatin1String("notes.adr"));
		}
		else
		{
			return path;
		}
	}
#if !defined(Q_OS_MAC) && defined(Q_OS_UNIX)
	const QString homePath(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).value(0));

	if (!homePath.isEmpty())
	{
		return QDir(homePath).filePath(QLatin1String(".opera/notes.adr"));
	}
#endif

	return path;
}

QString OperaNotesImporter::getBrowser() const
{
	return QLatin1String("opera");
}

QUrl OperaNotesImporter::getHomePage() const
{
	return QUrl(QLatin1String("https://otter-browser.org/"));
}

QStringList OperaNotesImporter::getFileFilters() const
{
	return {tr("Opera notes files (notes.adr)")};
}

Importer::ImportType OperaNotesImporter::getImportType() const
{
	return NotesImport;
}

bool OperaNotesImporter::import(const QString &path)
{
	QFile file(getSuggestedPath(path));

	if (!file.open(QIODevice::ReadOnly))
	{
		emit importFinished(NotesImport, FailedImport, 0);

		return false;
	}

	QTextStream stream(&file);
	stream.setCodec("UTF-8");

	QString line(stream.readLine());

	if (line != QLatin1String("Opera Hotlist version 2.0"))
	{
		emit importFinished(NotesImport, FailedImport, 0);

		return false;
	}

	emit importStarted(NotesImport, -1);

	if (m_optionsWidget)
	{
		m_importFolder = m_folderComboBox->getCurrentFolder();
		m_currentFolder = m_importFolder;
	}

	int totalAmount(0);

	NotesManager::getModel()->beginImport(m_importFolder, ((file.size() > 0) ? static_cast<int>(file.size() / 250) : 0));

	BookmarksModel::Bookmark *note(nullptr);
	OperaNoteEntry type(NoEntry);
	bool isHeader(true);

	while (!stream.atEnd())
	{
		line = stream.readLine();

		if (isHeader && (line.isEmpty() || line.at(0) != QLatin1Char('#')))
		{
			continue;
		}

		isHeader = false;

		if (line.startsWith(QLatin1String("#NOTE")))
		{
			note = NotesManager::addNote(BookmarksModel::UrlBookmark, {}, m_currentFolder);
			type = NoteEntry;

			++totalAmount;
		}
		else if (line.startsWith(QLatin1String("#FOLDER")))
		{
			note = NotesManager::addNote(BookmarksModel::FolderBookmark, {}, m_currentFolder);
			type = FolderStartEntry;

			++totalAmount;
		}
		else if (line.startsWith(QLatin1String("#SEPERATOR")))
		{
			note = NotesManager::addNote(BookmarksModel::SeparatorBookmark, {}, m_currentFolder);
			type = SeparatorEntry;

			++totalAmount;
		}
		else if (line == QLatin1String("-"))
		{
			type = FolderEndEntry;
		}
		else if (line.startsWith(QLatin1String("\tURL=")) && note)
		{
			const QUrl url(line.section(QLatin1Char('='), 1, -1));

			note->setData(url, BookmarksModel::UrlRole);
		}
		else if (line.startsWith(QLatin1String("\tNAME=")) && note)
		{
			note->setData(line.section(QLatin1Char('='), 1, -1).replace(QLatin1String("\x02\x02"), QLatin1String("\n")), BookmarksModel::DescriptionRole);
		}
		else if (line.startsWith(QLatin1String("\tCREATED=")) && note)
		{
			note->setData(QDateTime::fromTime_t(line.section(QLatin1Char('='), 1, -1).toUInt()), BookmarksModel::TimeAddedRole);
		}
		else if (line.isEmpty())
		{
			if (note)
			{
				if (type == FolderStartEntry)
				{
					m_currentFolder = note;
				}

				note = nullptr;
			}
			else if (type == FolderEndEntry && m_currentFolder != m_importFolder)
			{
				if (m_currentFolder)
				{
					m_currentFolder = static_cast<BookmarksModel::Bookmark*>(m_currentFolder->parent());
				}

				if (!m_currentFolder)
				{
					m_currentFolder = (m_importFolder ? m_importFolder : NotesManager::getModel()->getRootItem());
				}
			}

			type = NoEntry;
		}
	}

	NotesManager::getModel()->endImport();

	emit importFinished(NotesImport, SuccessfullImport, totalAmount);

	file.close();

	return true;
}

}
