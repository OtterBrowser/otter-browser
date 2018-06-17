/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2016 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../../../../core/Application.h"
#include "../../../../core/SessionsManager.h"
#include "../../../../core/ThemesManager.h"
#include "../../../../core/Utils.h"

#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QMimeDatabase>
#include <QtCore/QRegularExpression>
#include <QtCore/QtMath>
#include <QtWidgets/QFileIconProvider>

namespace Otter
{

QtWebKitFtpListingNetworkReply::QtWebKitFtpListingNetworkReply(const QNetworkRequest &request, QObject *parent) : QNetworkReply(parent),
	m_ftp(new QFtp(this)),
	m_offset(0)
{
	setRequest(request);

	connect(m_ftp, &QFtp::listInfo, this, &QtWebKitFtpListingNetworkReply::addEntry);
	connect(m_ftp, &QFtp::readyRead, this, &QtWebKitFtpListingNetworkReply::processData);
	connect(m_ftp, &QFtp::commandFinished, this, &QtWebKitFtpListingNetworkReply::processCommand);
	connect(m_ftp, &QFtp::dataTransferProgress, this, &QtWebKitFtpListingNetworkReply::downloadProgress);

	m_ftp->connectToHost(request.url().host());
}

void QtWebKitFtpListingNetworkReply::processCommand(int command, bool isError)
{
	Q_UNUSED(command)

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

		emit error(ContentNotFoundError);

		return;
	}

	switch (m_ftp->currentCommand())
	{
		case QFtp::ConnectToHost:
			m_ftp->login();

			break;
		case QFtp::Login:
			m_ftp->list(Utils::normalizeUrl(request().url()).path());

			break;
		case QFtp::List:
			if (m_files.count() == 1 && m_directories.isEmpty() && request().url().path().endsWith(m_files.first().name()))
			{
				m_ftp->get(Utils::normalizeUrl(request().url()).path());
			}
			else
			{
				open(ReadOnly | Unbuffered);

				QString entriesHtml;
				const QMimeDatabase mimeDatabase;
				const QRegularExpression entryExpression(QLatin1String("<!--entry:begin-->(.*)<!--entry:end-->"), (QRegularExpression::DotMatchesEverythingOption | QRegularExpression::MultilineOption));
				QFile file(SessionsManager::getReadableDataPath(QLatin1String("files/listing.html")));
				file.open(QIODevice::ReadOnly | QIODevice::Text);

				QTextStream stream(&file);
				stream.setCodec("UTF-8");

				QString mainTemplate(stream.readAll());
				const QString entryTemplate(entryExpression.match(mainTemplate).captured(1));
				QUrl url(request().url());
				QStringList navigation;

				if (url.path().isEmpty())
				{
					url.setPath(QLatin1String("/"));
				}

				while (true)
				{
					const bool isRoot(url.path() == QLatin1String("/"));

					url = url.adjusted(QUrl::StripTrailingSlash);

					navigation.prepend(QStringLiteral("<a href=\"%1\">%2</a>").arg(url.url()).arg(isRoot ? url.toString() : url.fileName() + QLatin1Char('/')));

					if (isRoot)
					{
						break;
					}

					url = url.adjusted(QUrl::RemoveFilename);
				}

				QHash<QString, QString> icons;
				QHash<QString, QString> variables;
				variables[QLatin1String("title")] = request().url().toString() + (request().url().path().endsWith(QLatin1Char('/')) ? QString() : QLatin1String("/"));
				variables[QLatin1String("description")] = tr("Directory Contents");
				variables[QLatin1String("dir")] = (QGuiApplication::isLeftToRight() ? QLatin1String("ltr") : QLatin1String("rtl"));
				variables[QLatin1String("navigation")] = navigation.join(QLatin1String("&shy;"));
				variables[QLatin1String("headerName")] = tr("Name");
				variables[QLatin1String("headerType")] = tr("Type");
				variables[QLatin1String("headerSize")] = tr("Size");
				variables[QLatin1String("headerDate")] = tr("Date");

				const int iconSize(16 * qCeil(Application::getInstance()->devicePixelRatio()));
				const QFileIconProvider iconProvider;
				const QVector<QUrlInfo> entries(m_directories + m_files);

				for (int i = 0; i < entries.count(); ++i)
				{
					const QMimeType mimeType(entries.at(i).isDir() ? mimeDatabase.mimeTypeForName(QLatin1String("inode-directory")) : mimeDatabase.mimeTypeForUrl(request().url().url() + entries.at(i).name()));
					QString entryHtml(entryTemplate);

					if (!icons.contains(mimeType.name()))
					{
						QByteArray byteArray;
						QBuffer buffer(&byteArray);
						QIcon icon(QIcon::fromTheme(mimeType.iconName(), iconProvider.icon(entries.at(i).isDir() ? QFileIconProvider::Folder : QFileIconProvider::File)));

						if (icon.isNull())
						{
							icon = ThemesManager::createIcon((entries.at(i).isDir() ? QLatin1String("inode-directory") : QLatin1String("unknown")), false);
						}

						icon.pixmap(iconSize, iconSize).save(&buffer, "PNG");

						icons[mimeType.name()] = QString(byteArray.toBase64());
					}

					QHash<QString, QString> entryVariables;
					entryVariables[QLatin1String("url")] = Utils::normalizeUrl(request().url()).url() + QLatin1Char('/') + entries.at(i).name();
					entryVariables[QLatin1String("icon")] = QStringLiteral("data:image/png;base64,%1").arg(icons[mimeType.name()]);
					entryVariables[QLatin1String("mimeType")] = mimeType.name();
					entryVariables[QLatin1String("name")] = entries.at(i).name();
					entryVariables[QLatin1String("comment")] = mimeType.comment();
					entryVariables[QLatin1String("size")] = (entries.at(i).isDir() ? QString() : Utils::formatUnit(entries.at(i).size(), false, 2));
					entryVariables[QLatin1String("lastModified")] = Utils::formatDateTime(entries.at(i).lastModified());

					QHash<QString, QString>::iterator iterator;

					for (iterator = entryVariables.begin(); iterator != entryVariables.end(); ++iterator)
					{
						entryHtml.replace(QLatin1Char('{') + iterator.key() + QLatin1Char('}'), iterator.value());
					}

					entriesHtml.append(entryHtml);
				}

				mainTemplate.replace(entryExpression, entriesHtml);

				QHash<QString, QString>::iterator iterator;

				for (iterator = variables.begin(); iterator != variables.end(); ++iterator)
				{
					mainTemplate.replace(QLatin1Char('{') + iterator.key() + QLatin1Char('}'), iterator.value());
				}

				m_content = mainTemplate.toUtf8();

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

void QtWebKitFtpListingNetworkReply::addEntry(const QUrlInfo &entry)
{
	if (entry.isDir())
	{
		m_directories.append(entry);
	}
	else
	{
		m_files.append(entry);
	}
}

void QtWebKitFtpListingNetworkReply::processData()
{
	m_content += m_ftp->readAll();

	emit readyRead();
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
