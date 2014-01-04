/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "BookmarksImportDialog.h"
#include "ui_BookmarksImportDialog.h"
#include "../core/Utils.h"

namespace Otter {

BookmarksImportDialog::BookmarksImportDialog(QWidget *parent) :
	QDialog(parent),
	m_filename(""),
	m_model(new QStandardItemModel(this)),
	m_ui(new Ui::BookmarksImportDialog)
{
	m_ui->setupUi(this);

	m_ui->fileLabel->setText(tr("File"));
	m_ui->fileButton->setText(tr("Choose"));
	m_ui->newFolderButton->setText(tr("New"));

	QTreeView *view = new QTreeView(m_ui->folderComboBox);
	m_ui->folderComboBox->setView(view);
	m_ui->folderComboBox->setModel(m_model);

	view->setHeaderHidden(true);
	view->setItemsExpandable(false);
	view->setRootIsDecorated(false);
	view->setStyleSheet(QLatin1String("QTreeView::branch {border-image: url(invalid.png);}"));

	bookmark = new BookmarkInformation();
	m_folder = 0;

	reloadFolders();

	connect(BookmarksManager::getInstance(), SIGNAL(folderModified(int)), this, SLOT(reloadFolders()));
	connect(view->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(folderChanged(QModelIndex)));
	connect(m_ui->newFolderButton, SIGNAL(clicked()), this, SLOT(createFolder()));

	connect(m_ui->fileButton, SIGNAL(clicked()), this, SLOT(chooseFile()));
	connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(ready()));
}

BookmarksImportDialog::~BookmarksImportDialog()
{
	delete m_ui;
}

void BookmarksImportDialog::ready()
{
	if (!m_filename.isEmpty())
	{
		QFile file(m_filename);
		if (!file.open(QIODevice::ReadOnly))
		{
			QMessageBox::critical(this, tr("Error"), tr("Could not open file!"));
			return;
		}
		QTextStream in(&file);
		importBookmarksFromAdr(in);

		file.close();
	}

	accept();
}

void BookmarksImportDialog::reloadFolders()
{
	m_model->clear();

	QStandardItem *item = new QStandardItem(Utils::getIcon(QLatin1String("inode-directory")), tr("Bookmarks"));
	item->setData(0, Qt::UserRole);
	item->setToolTip(tr("Bookmarks"));

	m_model->invisibleRootItem()->appendRow(item);

	m_index = item->index();

	populateFolder(BookmarksManager::getFolder(0), item);

	QTreeView *view = qobject_cast<QTreeView*>(m_ui->folderComboBox->view());

	if (view)
	{
		view->setCurrentIndex(m_index);
		view->expandAll();

		m_ui->folderComboBox->setRootModelIndex(m_index.parent());
		m_ui->folderComboBox->setModelColumn(0);
		m_ui->folderComboBox->setCurrentIndex(m_index.row());
		m_ui->folderComboBox->setRootModelIndex(QModelIndex());
	}
}

void BookmarksImportDialog::folderChanged(const QModelIndex &index)
{
	m_folder = index.data(Qt::UserRole).toInt();
}

void BookmarksImportDialog::populateFolder(const QList<BookmarkInformation*> bookmarks, QStandardItem *parent)
{
	for (int i = 0; i < bookmarks.count(); ++i)
	{
		if (bookmarks.at(i)->type == FolderBookmark)
		{
			const QString title = (bookmarks.at(i)->title.isEmpty() ? tr("(Untitled)") : bookmarks.at(i)->title);
			QStandardItem *item = new QStandardItem(Utils::getIcon(QLatin1String("inode-directory")), title);
			item->setData(bookmarks.at(i)->identifier, Qt::UserRole);
			item->setToolTip(title);

			parent->appendRow(item);

			populateFolder(BookmarksManager::getFolder(bookmarks.at(i)->identifier), item);

			if (bookmarks.at(i)->identifier == m_folder)
			{
				m_index = item->index();
			}
		}
	}
}

void BookmarksImportDialog::createFolder()
{
	const QString title = QInputDialog::getText(this, tr("Folder Name"), tr("Select name of new folder:"));

	if (title.isEmpty())
	{
		return;
	}

	BookmarkInformation *bookmark = new BookmarkInformation();
	bookmark->title = title;

	disconnect(BookmarksManager::getInstance(), SIGNAL(folderModified(int)), this, SLOT(reloadFolders()));

	if (BookmarksManager::addBookmark(bookmark, m_ui->folderComboBox->view()->currentIndex().data(Qt::UserRole).toInt()))
	{
		m_folder = bookmark->identifier;

		reloadFolders();
	}
	else
	{
		delete bookmark;
	}

	connect(BookmarksManager::getInstance(), SIGNAL(folderModified(int)), this, SLOT(reloadFolders()));
}

void BookmarksImportDialog::chooseFile()
{
	m_filename = QFileDialog::getOpenFileName(this, tr("Open file"), QString(), tr("Adr bookmarks file (bookmarks.adr);;All files (*.*)"));
}




int BookmarksImportDialog::importBookmarksFromAdr(QTextStream &istream)
{
	QString line, firstline("Opera Hotlist version 2.0");
	BookmarkInformation readedBookmark;
	QString PREFIX_URL("\tURL="), PREFIX_NAME("\tNAME="), PREFIX_DESC("\tDESCRIPTION="), PREFIX_SNAME("\tSHORT NAME="), PREFIX_TRASH("\tTRASH FOLDER=");

	// check if bookmarks file
	line = istream.readLine();
	if (line != firstline)
	{
		return 1;
	}

	atReadingStart();

	// read options lines and blank line
	do
	{
		line = istream.readLine();
	} while (line.isEmpty());

	// here are two blank lines alongside, so here second is readedBookmark
	line = istream.readLine();

	// reading bookmarks
	while (!istream.atEnd())
	{
		line = istream.readLine();
		// here new bookmark starts
		if (line.startsWith("#URL"))
		{
			readedBookmark.url = readedBookmark.title = readedBookmark.description = "";
			currentEntryType = EntryBookmark;
		}
		// here new folder starts
		else if (line.startsWith("#FOLDER"))
		{
			readedBookmark.url = readedBookmark.title = readedBookmark.description = "";
			currentEntryType = EntryFolder;
		}
		// here separator starts. It's not misspelling
		else if (line.startsWith("#SEPERATOR"))
		{
			readedBookmark.url = readedBookmark.title = readedBookmark.description = "";
			currentEntryType = EntrySeparator;
		}
		// here folder ends
		else if (line == "-")
		{
			currentEntryType = EntryFolderEnd;
		}
		// used properties
		else if (line.startsWith(PREFIX_URL))
		{
			readedBookmark.url = QString(line.midRef(PREFIX_URL.size()).data());
		}
		else if (line.startsWith(PREFIX_NAME))
		{
			readedBookmark.title = QString(line.midRef(PREFIX_NAME.size()).data());
		}
		else if (line.startsWith(PREFIX_DESC))
		{
			readedBookmark.description = QString(line.midRef(PREFIX_DESC.size()).data()).replace(QString("\x02\x02"), QString("\n"));
		}
		// unused properties
		else if (line.startsWith(PREFIX_SNAME))
		{
			// TODO need short name support in ../core/BookmarksManager.h
		}
		else if (line.startsWith(PREFIX_TRASH))
		{
			// do nothing
		}
		else if (line.startsWith("\tID="))
		{
			//do nothing
		}
		else if (line.startsWith("\tCREATED="))
		{
			//do nothing
		}
		else if (line.startsWith("\tVISITED="))
		{
			//do nothing
		}
		else if (line.startsWith("\tUNIQUEID="))
		{
			//do nothing
		}
		else if (line.startsWith("\tPARTNERID="))
		{
			//do nothing
		}
		else if (line.startsWith("\tPARTNERID="))
		{
			//do nothing
		}
		else if (line.startsWith("\tEXPANDED"))
		{
			//do nothing
		}
		else if (line.startsWith("\tSMALL SCREEN="))
		{
			//do nothing
		}
		else if (line.startsWith("\tDELETABLE="))
		{
			//do nothing
		}
		else if (line.startsWith("\tACTIVE="))
		{
			//do nothing
		}
		else if (line.startsWith("\tTARGET="))
		{
			//do nothing
		}
		else if (line.startsWith("\tMOVE_IS_COPY"))
		{
			//do nothing
		}
		else if (line.startsWith("\tSEPARATOR_ALLOWED"))
		{
			//do nothing
		}
		else if (line.startsWith("#DELETED"))
		{
			//do nothing
		}
		// blank line ends bookmark properties
		else if (line.isEmpty())
		{
			switch (currentEntryType)
			{
				case EntryBookmark:
					atBookmark(readedBookmark);
					break;

				case EntryFolder:
					atFolderStart(readedBookmark);
					break;

				case EntryFolderEnd:
					atFolderEnd();
					break;

				case EntrySeparator:
					atSeparator(readedBookmark);
					break;
			}
		}
		// unknown line
		else
		{
			atUnknownLine(line);
		}
	}

	return 0;
}

void BookmarksImportDialog::atReadingStart()
{
	foldersPath.push(m_folder);
}

void BookmarksImportDialog::atFolderStart(BookmarkInformation bookmark)
{
	BookmarkInformation* addedBookmark = new BookmarkInformation();

	addedBookmark->url = bookmark.url;
	addedBookmark->title = bookmark.title;
	addedBookmark->description = bookmark.description;
	addedBookmark->type = FolderBookmark;

	BookmarksManager::addBookmark(addedBookmark, m_folder);
	m_folder = addedBookmark->identifier;
	foldersPath.push(m_folder);
}

void BookmarksImportDialog::atFolderEnd()
{
	foldersPath.pop();
	m_folder = foldersPath.top();
}

void BookmarksImportDialog::atBookmark(BookmarkInformation bookmark)
{
	BookmarkInformation* addedBookmark = new BookmarkInformation();

	addedBookmark->url = bookmark.url;
	addedBookmark->title = bookmark.title;
	addedBookmark->description = bookmark.description;
	addedBookmark->type = UrlBookmark;

	BookmarksManager::addBookmark(addedBookmark, m_folder);
}

void BookmarksImportDialog::atSeparator(BookmarkInformation bookmark)
{
	BookmarkInformation* addedBookmark = new BookmarkInformation();

	addedBookmark->url = bookmark.url;
	addedBookmark->title = bookmark.title;
	addedBookmark->description = bookmark.description;
	addedBookmark->type = SeparatorBookmark;

	BookmarksManager::addBookmark(addedBookmark, m_folder);
}

void BookmarksImportDialog::atUnknownLine(QString line)
{
	QDebug(QtDebugMsg) << "Nieznana linia: " << line;
}

}
