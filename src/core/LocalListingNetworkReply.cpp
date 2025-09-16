/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 - 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#include "LocalListingNetworkReply.h"
#include "Utils.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QMimeDatabase>
#include <QtCore/QTimer>

namespace Otter
{

LocalListingNetworkReply::LocalListingNetworkReply(const QNetworkRequest &request, QObject *parent) : ListingNetworkReply(request, parent),
	m_offset(0)
{
	setRequest(request);
	open(QIODevice::ReadOnly | QIODevice::Unbuffered);

	QDir directory(request.url().toLocalFile());

	if (!directory.exists() || !directory.isReadable())
	{
		ErrorPageInformation information;
		information.url = request.url();
		information.actions.append({QLatin1String("reloadPage"), QCoreApplication::translate("utils", "Try Again"), ErrorPageInformation::MainAction});

		if (directory.isReadable())
		{
			information.description = QStringList(tr("Directory does not exist"));
			information.type = ErrorPageInformation::FileNotFoundError;
		}
		else
		{
			information.title = tr("Directory is not readable");
			information.description = QStringList(tr("Cannot read directory listing"));
		}

		m_content = Utils::createErrorPage(information).toUtf8();

		setError(QNetworkReply::ContentAccessDenied, information.description.value(0));
		setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QLatin1String("text/html; charset=UTF-8")));
		setHeader(QNetworkRequest::ContentLengthHeader, QVariant(m_content.size()));

		QTimer::singleShot(0, this, [&]()
		{
			emit listingError();
			emit readyRead();
			emit finished();
		});

		return;
	}

	const QMimeDatabase mimeDatabase;
	QVector<NavigationEntry> navigation;
	navigation.reserve(request.url().path().count(QLatin1Char('/')) + 1);

#ifdef Q_OS_WIN32
	const bool isListingDevices(request.url().toLocalFile() == QLatin1String("/"));
	const QFileInfoList rawEntries(isListingDevices ? QDir::drives() : directory.entryInfoList((QDir::AllEntries | QDir::Hidden), (QDir::Name | QDir::DirsFirst)));
#else
	const QFileInfoList rawEntries(directory.entryInfoList((QDir::AllEntries | QDir::Hidden), (QDir::Name | QDir::DirsFirst)));
#endif
	QVector<ListingEntry> entries;
	entries.reserve(rawEntries.count());

	do
	{
		NavigationEntry entry;
#ifdef Q_OS_WIN32
		entry.name = (directory.isRoot() ? directory.canonicalPath() : directory.dirName() + QLatin1Char('/'));
#else
		entry.name = ((directory.isRoot() ? QLatin1String("file://") : QString()) + directory.dirName() + QLatin1Char('/'));
#endif
		entry.url = QUrl::fromUserInput(directory.canonicalPath()).toString();

		navigation.prepend(entry);
	}
	while (directory.cdUp());

#ifdef Q_OS_WIN32
	if (isListingDevices)
	{
		navigation.clear();
	}

	NavigationEntry rootEntry;
	rootEntry.name = QLatin1String("file:///");
	rootEntry.url = QUrl::fromUserInput(QLatin1String("/"));

	navigation.prepend(rootEntry);
#endif

	for (int i = 0; i < rawEntries.count(); ++i)
	{
		const QFileInfo rawEntry(rawEntries.at(i));

		if (rawEntry.fileName() == QLatin1String(".") || rawEntry.fileName() == QLatin1String(".."))
		{
			continue;
		}

		ListingEntry entry;
		entry.name = rawEntry.fileName();
		entry.url = QUrl::fromUserInput(rawEntry.filePath());
		entry.timeModified = rawEntry.lastModified();
		entry.mimeType = mimeDatabase.mimeTypeForFile(rawEntry.filePath());
		entry.size = rawEntry.size();
		entry.isSymlink = rawEntry.isSymLink();

		if (rawEntry.isRoot())
		{
			entry.type = ListingEntry::DriveType;
		}
		else if (rawEntry.isDir())
		{
			entry.type = ListingEntry::DirectoryType;
		}
		else
		{
			entry.type = ListingEntry::FileType;
		}

#ifdef Q_OS_WIN32
		if (isListingDevices)
		{
			entry.name = rawEntry.filePath().remove(QLatin1Char('/'));
		}
#endif

		entries.append(entry);
	}

	m_content = createListing(QFileInfo(request.url().toLocalFile()).canonicalFilePath(), navigation, entries);

	setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QLatin1String("text/html; charset=UTF-8")));
	setHeader(QNetworkRequest::ContentLengthHeader, QVariant(m_content.size()));

	QTimer::singleShot(0, this, [&]()
	{
		emit readyRead();
		emit finished();
	});
}

void LocalListingNetworkReply::abort()
{
}

qint64 LocalListingNetworkReply::bytesAvailable() const
{
	return (m_content.size() - m_offset);
}

qint64 LocalListingNetworkReply::readData(char *data, qint64 maxSize)
{
	if (m_offset < m_content.size())
	{
		const qint64 size(qMin(maxSize, (m_content.size() - m_offset)));

		memcpy(data, (m_content.constData() + m_offset), static_cast<size_t>(size));

		m_offset += size;

		return size;
	}

	return -1;
}

bool LocalListingNetworkReply::isSequential() const
{
	return true;
}

}
