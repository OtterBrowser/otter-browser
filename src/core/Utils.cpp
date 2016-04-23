/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "Utils.h"
#include "Application.h"
#include "PlatformIntegration.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QTextStream>
#include <QtCore/QTime>
#include <QtCore/QtMath>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>

namespace Otter
{

namespace Utils
{

void runApplication(const QString &command, const QUrl &url)
{
	if (command.isEmpty() && !url.isValid())
	{
		return;
	}

	PlatformIntegration *integration = Application::getInstance()->getPlatformIntegration();

	if (integration)
	{
		integration->runApplication(command, url);
	}
	else
	{
		QDesktopServices::openUrl(QUrl(url));
	}
}

QString matchUrl(const QUrl &url, const QString &prefix)
{
	QString match = url.toString();

	if (match.startsWith(prefix, Qt::CaseInsensitive))
	{
		return match;
	}

	match = url.toString(QUrl::RemoveScheme).mid(2);

	if (match.startsWith(prefix, Qt::CaseInsensitive))
	{
		return match;
	}

	if (match.startsWith(QLatin1String("www.")) && url.host().count(QLatin1Char('.')) > 1)
	{
		match = match.mid(4);

		if (match.startsWith(prefix, Qt::CaseInsensitive))
		{
			return match;
		}
	}

	return QString();
}

QString createIdentifier(const QString &base, const QStringList &exclude, bool toLowerCase)
{
	QString identifier;

	if (!base.isEmpty())
	{
		identifier = base;

		if (toLowerCase)
		{
			identifier = base.toLower();
		}

		identifier.remove(QRegularExpression(QLatin1String("[^A-Za-z0-9\\-_]")));
	}

	if (identifier.isEmpty())
	{
		identifier = QLatin1String("custom");
	}

	if (!exclude.contains(identifier))
	{
		return identifier;
	}

	int number = 2;

	const QRegularExpression expression(QLatin1String("_([0-9]+)$"));
	const QRegularExpressionMatch match = expression.match(identifier);

	if (match.hasMatch())
	{
		identifier.remove(expression);

		number = match.captured(1).toInt();
	}

	while (exclude.contains(identifier + QLatin1Char('_') + QString::number(number)))
	{
		++number;
	}

	return identifier + QLatin1Char('_') + QString::number(number);
}

QString createErrorPage(const QUrl &url, const QString &errorNumber, const QString &errorString)
{
	QFile file(SessionsManager::getReadableDataPath(QLatin1String("files/error.html")));
	file.open(QIODevice::ReadOnly | QIODevice::Text);

	QTextStream stream(&file);
	stream.setCodec("UTF-8");

	QHash<QString, QString> variables;
	variables[QLatin1String("dir")] = (QGuiApplication::isLeftToRight() ? QLatin1String("ltr") : QLatin1String("rtl"));
	variables[QLatin1String("title")] = QCoreApplication::translate("utils", "Error %1").arg(errorNumber);
	variables[QLatin1String("description")] = errorString;
	variables[QLatin1String("introduction")] = QCoreApplication::translate("utils", "You tried to access the address <a href=\"%1\">%1</a>, which is currently unavailable. Please make sure that the web address (URL) is correctly spelled and punctuated, then try reloading the page.").arg(url.toDisplayString());

	QString mainTemplate = stream.readAll();
	QRegularExpression hintExpression(QLatin1String("<!--hint:begin-->(.*)<!--hint:end-->"), (QRegularExpression::DotMatchesEverythingOption | QRegularExpression::MultilineOption));
	const QString hintTemplate = hintExpression.match(mainTemplate).captured(1);
	QString hintsHtml;
	QStringList hints;
	hints << QCoreApplication::translate("utils", "Make sure your internet connection is active and check whether other applications that rely on the same connection are working.")
	<< QCoreApplication::translate("utils", "Check that the setup of any internet security software is correct and does not interfere with ordinary web browsing.")
	<< QCoreApplication::translate("utils", "Try pressing the F12 key on your keyboard and disabling proxy servers, unless you know that you are required to use a proxy to connect to the internet, and then reload the page.");

	for (int i = 0; i < hints.count(); ++i)
	{
		hintsHtml.append(QString(hintTemplate).replace(QLatin1String("{hint}"), hints.at(i)));
	}

	QHash<QString, QString>::iterator iterator;

	for (iterator = variables.begin(); iterator != variables.end(); ++iterator)
	{
		mainTemplate.replace(QStringLiteral("{%1}").arg(iterator.key()), iterator.value());
	}

	mainTemplate.replace(hintExpression, hintsHtml);

	return mainTemplate;
}

QString elideText(const QString &text, QWidget *widget, int width)
{
	if (width < 0)
	{
		width = (QApplication::desktop()->screenGeometry(widget).width() / 4);
	}

	return (widget ? widget->fontMetrics() : QApplication::fontMetrics()).elidedText(text, Qt::ElideRight, qMax(100, width));
}

QString formatConfigurationEntry(const QLatin1String &key, const QString &value, bool quote)
{
	QString escapedValue(value);
	escapedValue.replace(QLatin1Char('\n'), QLatin1String("\\n"));

	if (quote)
	{
		return QStringLiteral("%1=\"%2\"\n").arg(key).arg(escapedValue.replace(QLatin1Char('\"'), QLatin1String("\\\"")));
	}

	return QStringLiteral("%1=%2\n").arg(key).arg(escapedValue);
}

QString formatTime(int value)
{
	QTime time(0, 0);
	time = time.addSecs(value);

	if (value > 3600)
	{
		QString string = time.toString(QLatin1String("hh:mm:ss"));

		if (value > SECONDS_IN_DAY)
		{
			string = QCoreApplication::translate("utils", "%n days %1", "", (qFloor(static_cast<qreal>(value) / SECONDS_IN_DAY))).arg(string);
		}

		return string;
	}

	return time.toString(QLatin1String("mm:ss"));
}

QString formatDateTime(const QDateTime &dateTime, const QString &format)
{
	return (format.isEmpty() ? QLocale().toString(dateTime, QLocale::ShortFormat) : QLocale().toString(dateTime, format));
}

QString formatUnit(qint64 value, bool isSpeed, int precision, bool appendRaw)
{
	if (value < 0)
	{
		return QString('?');
	}

	if (value > 1024)
	{
		if (value > 1048576)
		{
			if (value > 1073741824)
			{
				return QCoreApplication::translate("utils", (isSpeed ? "%1 GB/s" : "%1 GB")).arg((value / 1073741824.0), 0, 'f', precision) + (appendRaw ? (isSpeed ? QString(" (%1 B/s)") : QString(" (%1 B)")).arg(value) : QString());
			}

			return QCoreApplication::translate("utils", (isSpeed ? "%1 MB/s" : "%1 MB")).arg((value / 1048576.0), 0, 'f', precision) + (appendRaw ? (isSpeed ? QString(" (%1 B/s)") : QString(" (%1 B)")).arg(value) : QString());
		}

		return QCoreApplication::translate("utils", (isSpeed ? "%1 KB/s" : "%1 KB")).arg((value / 1024.0), 0, 'f', precision) + (appendRaw ? (isSpeed ? QString(" (%1 B/s)") : QString(" (%1 B)")).arg(value) : QString());
	}

	return QCoreApplication::translate("utils", (isSpeed ? "%1 B/s" : "%1 B")).arg(value);
}

QString normalizePath(const QString &path)
{
	if (path == QString(QLatin1Char('~')) || path.startsWith(QLatin1Char('~') + QDir::separator()))
	{
		const QStringList locations(QStandardPaths::standardLocations(QStandardPaths::HomeLocation));

		if (!locations.isEmpty())
		{
			return locations.first() + path.mid(1);
		}
	}

	return path;
}

QUrl normalizeUrl(QUrl url)
{
	url = url.adjusted(QUrl::RemoveFragment | QUrl::NormalizePathSegments | QUrl::StripTrailingSlash);

	if (url.path() == QLatin1String("/"))
	{
		url.setPath(QString());
	}

	return url;
}

QList<ApplicationInformation> getApplicationsForMimeType(const QMimeType &mimeType)
{
	PlatformIntegration *integration = Application::getInstance()->getPlatformIntegration();

	if (integration)
	{
		return integration->getApplicationsForMimeType(mimeType);
	}

	return QList<ApplicationInformation>();
}

bool isUrlEmpty(const QUrl &url)
{
	return (url.isEmpty() || (url.scheme() == QLatin1String("about") && (url.path().isEmpty() || url.path() == QLatin1String("blank") || url.path() == QLatin1String("start"))));
}

}

}
