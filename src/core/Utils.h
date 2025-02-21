/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include <QtCore/QMetaEnum>
#include <QtCore/QMimeData>
#include <QtCore/QMimeType>
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>
#include <QtGui/QIcon>

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

struct DiagnosticReport final
{
	struct Section final
	{
		QString title;
		QVector<QStringList> entries;
		QVector<int> fieldWidths;
	};

	QVector<Section> sections;
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
		RedirectionLoopError,
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

class EnumeratorMapper
{
public:
	explicit EnumeratorMapper(const QMetaEnum &enumeration, const QString &suffix = {});

	QString mapToName(int value, bool lowercaseFirst = true) const;
	int mapToValue(const QString &name, bool checkSuffix = false) const;

private:
	QMetaEnum m_enumerator;
	QString m_suffix;
};

namespace Utils
{

void removeFiles(const QStringList &paths);
void runApplication(const QString &command, const QUrl &url = {});
void startLinkDrag(const QUrl &url, const QString &title, const QPixmap &pixmap, QObject *parent = nullptr);
void showToolTip(const QPoint &position, const QString &text, QWidget *widget, const QRect &rectangle);
QString matchUrl(const QUrl &url, const QString &prefix);
QString createIdentifier(const QString &source, const QStringList &exclude = {}, bool toLowerCase = true);
QString createErrorPage(const ErrorPageInformation &information);
QString appendShortcut(const QString &text, const QKeySequence &shortcut);
QString elideText(const QString &text, const QFontMetrics &fontMetrics, QWidget *widget = nullptr, int maximumWidth = -1, int minimumWidth = 100);
QString substitutePlaceholders(QString text, const QHash<QString, QString> &placeholders);
QString savePixmapAsDataUri(const QPixmap &pixmap);
QString extractHost(const QUrl &url);
QString formatElapsedTime(int value);
QString formatDateTime(const QDateTime &dateTime, QString format = {}, bool allowFancy = true);
QString formatUnit(qint64 value, bool isSpeed = false, int precision = 1, bool appendRaw = false);
QString formatFileTypes(const QStringList &filters = {});
QString normalizeObjectName(QString name, const QString &suffix = {});
QString normalizePath(const QString &path);
QString getTopLevelDomain(const QUrl &url);
QString getStandardLocation(QStandardPaths::StandardLocation type);
QUrl expandUrl(const QUrl &url);
QUrl normalizeUrl(QUrl url);
QColor createColor(const QUrl &url);
QLocale createLocale(const QString &name);
QPixmap loadPixmapFromDataUri(const QString &data);
QFont multiplyFontSize(QFont font, qreal multiplier);
SaveInformation getSavePath(const QString &fileName, const QString &directory = {}, QStringList filters = {}, bool forceAsk = false);
QStringList getCharacterEncodings();
QStringList getOpenPaths(const QStringList &fileNames = {}, QStringList filters = {}, bool selectMultiple = false);
QVector<QUrl> extractUrls(const QMimeData *mimeData);
QVector<ApplicationInformation> getApplicationsForMimeType(const QMimeType &mimeType);
qreal calculatePercent(qint64 amount, qint64 total, int multiplier = 100);
bool ensureDirectoryExists(const QString &path);
bool isDomainTheSame(const QUrl &firstUrl, const QUrl &secondUrl);
bool isUrl(const QString &text);
bool isUrlAmbiguous(const QUrl &url);
bool isUrlEmpty(const QUrl &url);

}

}

#endif
