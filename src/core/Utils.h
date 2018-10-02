/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include <QtCore/QMimeData>
#include <QtCore/QMimeType>
#include <QtCore/QUrl>
#include <QtGui/QIcon>

#define SECONDS_IN_DAY 86400

namespace Otter
{

enum TrileanValue
{
	UnknownValue = 0,
	FalseValue,
	TrueValue
};

struct ApplicationInformation final
{
	QString command;
	QString name;
	QIcon icon;
};

struct ErrorPageInformation final
{
	enum ActionType
	{
		OtherAction = 0,
		MainAction,
		AdvancedAction
	};

	enum ErrorType
	{
		OtherError = 0,
		BlockedContentError,
		ConnectionInsecureError,
		ConnectionRefusedError,
		FileNotFoundError,
		FraudAttemptError,
		ServerNotFoundError,
		UnsupportedAddressTypeError
	};

	struct PageAction final
	{
		QString name;
		QString title;
		ActionType type;
	};

	QString title;
	QUrl url;
	QStringList description;
	QVector<PageAction> actions;
	ErrorType type = OtherError;
};

struct SaveInformation final
{
	QString path;
	QString filter;
	bool canSave = false;
};

namespace Utils
{

void runApplication(const QString &command, const QUrl &url = {});
void startLinkDrag(const QUrl &url, const QString &title, const QPixmap &pixmap, QObject *parent = nullptr);
QString matchUrl(const QUrl &url, const QString &prefix);
QString createIdentifier(const QString &source, const QStringList &exclude, bool toLowerCase = true);
QString createErrorPage(const ErrorPageInformation &information);
QString elideText(const QString &text, const QFontMetrics &fontMetrics, QWidget *widget = nullptr, int maximumWidth = -1, int minimumWidth = 100);
QString savePixmapAsDataUri(const QPixmap &pixmap);
QString extractHost(const QUrl &url);
QString formatElapsedTime(int value);
QString formatDateTime(const QDateTime &dateTime, QString format = {}, bool allowFancy = true);
QString formatUnit(qint64 value, bool isSpeed = false, int precision = 1, bool appendRaw = false);
QString formatFileTypes(const QStringList &filters = {});
QString normalizeObjectName(QString name, const QString &suffix = {});
QString normalizePath(const QString &path);
QUrl expandUrl(const QUrl &url);
QUrl normalizeUrl(QUrl url);
QLocale createLocale(const QString &name);
QPixmap loadPixmapFromDataUri(const QString &data);
SaveInformation getSavePath(const QString &fileName, const QString &directory = {}, QStringList filters = {}, bool forceAsk = false);
QStringList getOpenPaths(const QStringList &fileNames = {}, QStringList filters = {}, bool selectMultiple = false);
QVector<QUrl> extractUrls(const QMimeData *mimeData);
QVector<ApplicationInformation> getApplicationsForMimeType(const QMimeType &mimeType);
qreal calculatePercent(qint64 amount, qint64 total, int multiplier = 100);
bool isUrl(const QString &text);
bool isUrlEmpty(const QUrl &url);

}

}

#endif
