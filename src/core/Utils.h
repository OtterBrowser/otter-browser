/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_UTILS_H
#define OTTER_UTILS_H

#include <QtCore/QMimeType>
#include <QtCore/QUrl>
#include <QtGui/QIcon>

#define SECONDS_IN_DAY 86400

namespace Otter
{

struct ApplicationInformation
{
	QString command;
	QString name;
	QIcon icon;
};

struct SaveInformation
{
	QString path;
	QString filter;
	bool canOverwriteExisting = false;
};

namespace Utils
{

void runApplication(const QString &command, const QUrl &url = QUrl());
QString matchUrl(const QUrl &url, const QString &prefix);
QString createIdentifier(const QString &base, const QStringList &exclude, bool toLowerCase = true);
QString createErrorPage(const QUrl &url, const QString &title, const QString &description);
QString elideText(const QString &text, QWidget *widget = nullptr, int width = -1);
QString formatConfigurationEntry(const QLatin1String &key, const QString &value, bool quote = false);
QString formatTime(int value);
QString formatDateTime(const QDateTime &dateTime, QString format = QString());
QString formatUnit(qint64 value, bool isSpeed = false, int precision = 1, bool appendRaw = false);
QString formatFileTypes(const QStringList &filters = QStringList());
QString normalizePath(const QString &path);
QUrl normalizeUrl(QUrl url);
SaveInformation getSavePath(const QString &fileName, QString path = QString(), QStringList filters = QStringList(), bool forceAsk = false);
QStringList getOpenPaths(const QStringList &fileNames = QStringList(), QStringList filters = QStringList(), bool selectMultiple = false);
QList<ApplicationInformation> getApplicationsForMimeType(const QMimeType &mimeType);
bool isUrlEmpty(const QUrl &url);

}

}

#endif
