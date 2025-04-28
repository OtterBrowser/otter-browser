/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2016 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "QtWebKitFtpListingNetworkReply.h"
#include "../../../../core/Utils.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QMimeDatabase>

namespace Otter
{

QtWebKitFtpListingNetworkReply::QtWebKitFtpListingNetworkReply(const QNetworkRequest &request, QObject *parent) : ListingNetworkReply(request, parent),
	m_ftp(new QFtp(this)),
	m_offset(0)
{
	connect(m_ftp, &QFtp::listInfo, this, [&](const QUrlInfo &entry)
	{
		if (entry.isSymLink())
		{
			m_symlinks.append(entry);
		}
		else if (entry.isDir())
		{
			m_directories.append(entry);
		}
		else
		{
			m_files.append(entry);
		}
	});
	connect(m_ftp, &QFtp::readyRead, this, [&]()
	{
		m_content += m_ftp->readAll();

		emit readyRead();
	});
	connect(m_ftp, &QFtp::commandFinished, this, &QtWebKitFtpListingNetworkReply::processCommand);
	connect(m_ftp, &QFtp::dataTransferProgress, this, &QtWebKitFtpListingNetworkReply::downloadProgress);

	m_ftp->connectToHost(request.url().host());
}

void QtWebKitFtpListingNetworkReply::processCommand(int command, bool isError)
{
	if (isError)
	{
		open(ReadOnly | Unbuffered);

		ErrorPageInformation::PageAction reloadAction;
		reloadAction.name = QLatin1String("reloadPage");
		reloadAction.title = QCoreApplication::translate("utils", "Try Again");
		reloadAction.type = ErrorPageInformation::MainAction;

		ErrorPageInformation information;
		information.url = request().url();
		information.description = QStringList(m_ftp->errorString());
		information.actions.append(reloadAction);

		if (m_ftp->error() == QFtp::HostNotFound)
		{
			information.type = ErrorPageInformation::ServerNotFoundError;
		}
		else if (m_ftp->error() == QFtp::ConnectionRefused)
		{
			information.type = ErrorPageInformation::ConnectionRefusedError;
		}
		else if (m_ftp->replyCode() > 0)
		{
			information.title = tr("Network error %1").arg(m_ftp->replyCode());
		}

		m_content = Utils::createErrorPage(information).toUtf8();

		setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QLatin1String("text/html; charset=UTF-8")));
		setHeader(QNetworkRequest::ContentLengthHeader, QVariant(m_content.size()));

		emit listingError();
		emit readyRead();
		emit finished();

		if (m_ftp->error() != QFtp::NotConnected)
		{
			m_ftp->close();
		}

		setError(ContentNotFoundError, tr("Unknown command"));

		emit errorOccurred(ContentNotFoundError);

		return;
	}

	const QUrl normalizedUrl(Utils::normalizeUrl(request().url()));

	switch (command)
	{
		case QFtp::ConnectToHost:
			m_ftp->login();

			break;
		case QFtp::Login:
			m_ftp->list(normalizedUrl.path());

			break;
		case QFtp::List:
			if (m_directories.isEmpty() && ((m_files.count() == 1 && m_symlinks.isEmpty() && request().url().path().endsWith(m_files.value(0).name())) || (m_symlinks.count() == 1 && m_files.isEmpty() && request().url().path().endsWith(m_symlinks.value(0).name()))))
			{
				m_ftp->get(normalizedUrl.path());
			}
			else
			{
				open(ReadOnly | Unbuffered);

				QUrl url(request().url());
				QMimeDatabase mimeDatabase;
				QVector<NavigationEntry> navigation;
				const QVector<QUrlInfo> rawEntries(m_symlinks + m_directories + m_files);
				QVector<ListingEntry> entries;
				entries.reserve(rawEntries.count());

				if (url.path().isEmpty())
				{
					url.setPath(QLatin1String("/"));
				}

				while (true)
				{
					const bool isRoot(url.path() == QLatin1String("/"));

					url = url.adjusted(QUrl::StripTrailingSlash);

					NavigationEntry entry;
					entry.name = (isRoot ? url.toString() : url.fileName() + QLatin1Char('/'));
					entry.url = url.url();

					navigation.prepend(entry);

					if (isRoot)
					{
						break;
					}

					url = url.adjusted(QUrl::RemoveFilename);
				}

				for (int i = 0; i < rawEntries.count(); ++i)
				{
					const QUrlInfo rawEntry(rawEntries.at(i));
					ListingEntry entry;
					entry.name = rawEntry.name();
					entry.url = normalizedUrl.url() + QLatin1Char('/') + rawEntry.name();
					entry.timeModified = rawEntry.lastModified();
					entry.type = (rawEntry.isSymLink() ? ListingEntry::UnknownType : (rawEntry.isDir() ? ListingEntry::DirectoryType : ListingEntry::FileType));
					entry.size = rawEntry.size();
					entry.isSymlink = rawEntry.isSymLink();

					if (rawEntry.isSymLink())
					{
						entry.mimeType = mimeDatabase.mimeTypeForName(QLatin1String("text/uri-list"));
					}
					else if (rawEntry.isDir())
					{
						entry.mimeType = mimeDatabase.mimeTypeForName(QLatin1String("inode/directory"));
					}
					else
					{
						entry.mimeType = mimeDatabase.mimeTypeForUrl(normalizedUrl.url() + rawEntry.name());
					}

					entries.append(entry);
				}

				m_content = createListing(normalizedUrl.toString() + (normalizedUrl.path().endsWith(QLatin1Char('/')) ? QChar() : QLatin1Char('/')), navigation, entries);

				setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QLatin1String("text/html; charset=UTF-8")));
				setHeader(QNetworkRequest::ContentLengthHeader, QVariant(m_content.size()));

				emit readyRead();
				emit finished();

				m_ftp->close();
			}

			break;
		case QFtp::Get:
			open(QIODevice::ReadOnly | QIODevice::Unbuffered);
			setHeader(QNetworkRequest::ContentLengthHeader, QVariant(m_content.size()));

			emit readyRead();
			emit finished();

			m_ftp->close();

			break;
		default:
			break;
	}
}

void QtWebKitFtpListingNetworkReply::abort()
{
	m_ftp->close();

	emit finished();
}

qint64 QtWebKitFtpListingNetworkReply::bytesAvailable() const
{
	return (m_content.size() - m_offset);
}

qint64 QtWebKitFtpListingNetworkReply::readData(char *data, qint64 maxSize)
{
	if (m_offset < m_content.size())
	{
		const qint64 number(qMin(maxSize, (m_content.size() - m_offset)));

		memcpy(data, (m_content.constData() + m_offset), static_cast<size_t>(number));

		m_offset += number;

		return number;
	}

	return -1;
}

bool QtWebKitFtpListingNetworkReply::isSequential() const
{
	return true;
}

}
