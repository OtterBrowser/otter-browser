/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 - 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#include "LocalListingNetworkReply.h"
#include "SessionsManager.h"
#include "ThemesManager.h"
#include "Utils.h"

#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeDatabase>
#include <QtCore/QRegularExpression>
#include <QtCore/QTimer>
#include <QtGui/QGuiApplication>
#include <QtGui/QIcon>
#include <QtWidgets/QFileIconProvider>

namespace Otter
{

LocalListingNetworkReply::LocalListingNetworkReply(QObject *parent, const QNetworkRequest &request) : QNetworkReply(parent),
	m_offset(0)
{
	setRequest(request);
	open(QIODevice::ReadOnly | QIODevice::Unbuffered);

	QString entriesHtml;
	QRegularExpression entryExpression(QLatin1String("<!--entry:begin-->(.*)<!--entry:end-->"), (QRegularExpression::DotMatchesEverythingOption | QRegularExpression::MultilineOption));
	QFile file(SessionsManager::getReadableDataPath(QLatin1String("files/listing.html")));
	file.open(QIODevice::ReadOnly | QIODevice::Text);

	QTextStream stream(&file);
	stream.setCodec("UTF-8");

	QString mainTemplate(stream.readAll());
	const QString entryTemplate(entryExpression.match(mainTemplate).captured(1));
	QDir directory(request.url().toLocalFile());
	const QFileInfoList entries(directory.entryInfoList((QDir::AllEntries | QDir::Hidden), (QDir::Name | QDir::DirsFirst)));
	QStringList navigation;

	do
	{
		navigation.prepend(QStringLiteral("<a href=\"%1\">%2</a>").arg(QUrl::fromUserInput(directory.canonicalPath()).toString()).arg((directory.isRoot() ? QLatin1String("file://") : QString()) + directory.dirName() + QString('/')));
	}
	while (directory.cdUp());

	QHash<QString, QString> icons;
	QHash<QString, QString> variables;
	variables[QLatin1String("title")] = QFileInfo(request.url().toLocalFile()).canonicalFilePath();
	variables[QLatin1String("description")] = tr("Directory Contents");
	variables[QLatin1String("dir")] = (QGuiApplication::isLeftToRight() ? QLatin1String("ltr") : QLatin1String("rtl"));
	variables[QLatin1String("navigation")] = navigation.join(QLatin1String("&shy;"));
	variables[QLatin1String("headerName")] = tr("Name");
	variables[QLatin1String("headerType")] = tr("Type");
	variables[QLatin1String("headerSize")] = tr("Size");
	variables[QLatin1String("headerDate")] = tr("Date");

	const QFileIconProvider iconProvider;

	for (int i = 0; i < entries.count(); ++i)
	{
		const QMimeType mimeType(QMimeDatabase().mimeTypeForFile(entries.at(i).canonicalFilePath()));
		QString entryHtml(entryTemplate);

		if (!icons.contains(mimeType.name()))
		{
			QByteArray byteArray;
			QBuffer buffer(&byteArray);
			QPixmap pixmap(QIcon::fromTheme(mimeType.iconName(), iconProvider.icon(entries.at(i))).pixmap(16, 16));

			if (pixmap.isNull())
			{
				pixmap = ThemesManager::getIcon((entries.at(i).isDir() ? QLatin1String("inode-directory") : QLatin1String("unknown")), false).pixmap(16, 16);
			}

			pixmap.save(&buffer, "PNG");

			icons[mimeType.name()] = QString(byteArray.toBase64());
		}

		QHash<QString, QString> entryVariables;
		entryVariables[QLatin1String("url")] = QUrl::fromUserInput(entries.at(i).filePath()).toString();
		entryVariables[QLatin1String("icon")] = QStringLiteral("data:image/png;base64,%1").arg(icons[mimeType.name()]);
		entryVariables[QLatin1String("mimeType")] = mimeType.name();
		entryVariables[QLatin1String("name")] = entries.at(i).fileName();
		entryVariables[QLatin1String("comment")] = mimeType.comment();
		entryVariables[QLatin1String("size")] = (entries.at(i).isDir() ? QString() : Utils::formatUnit(entries.at(i).size(), false, 2));
		entryVariables[QLatin1String("lastModified")] = Utils::formatDateTime(entries.at(i).lastModified());

		QHash<QString, QString>::iterator iterator;

		for (iterator = entryVariables.begin(); iterator != entryVariables.end(); ++iterator)
		{
			entryHtml.replace(QStringLiteral("{%1}").arg(iterator.key()), iterator.value());
		}

		entriesHtml.append(entryHtml);
	}

	mainTemplate.replace(entryExpression, entriesHtml);

	QHash<QString, QString>::iterator iterator;

	for (iterator = variables.begin(); iterator != variables.end(); ++iterator)
	{
		mainTemplate.replace(QStringLiteral("{%1}").arg(iterator.key()), iterator.value());
	}

	m_content = mainTemplate.toUtf8();

	setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QLatin1String("text/html; charset=UTF-8")));
	setHeader(QNetworkRequest::ContentLengthHeader, QVariant(m_content.size()));

	QTimer::singleShot(0, this, SIGNAL(readyRead()));
	QTimer::singleShot(0, this, SIGNAL(finished()));
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
		qint64 number(qMin(maxSize, m_content.size() - m_offset));

		memcpy(data, (m_content.constData() + m_offset), number);

		m_offset += number;

		return number;
	}

	return -1;
}

bool LocalListingNetworkReply::isSequential() const
{
	return true;
}

}
