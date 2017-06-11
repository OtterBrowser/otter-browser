/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "Utils.h"
#include "Application.h"
#include "PlatformIntegration.h"
#include "TransfersManager.h"
#include "../ui/MainWindow.h"

#include <QtCore/QBuffer>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeDatabase>
#include <QtCore/QRegularExpression>
#include <QtCore/QTextStream>
#include <QtCore/QTime>
#include <QtCore/QtMath>
#include <QtGui/QDesktopServices>
#include <QtGui/QDrag>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

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

	PlatformIntegration *integration(Application::getPlatformIntegration());

	if (integration)
	{
		integration->runApplication(command, url);
	}
	else
	{
		QDesktopServices::openUrl(QUrl(url));
	}
}

void startLinkDrag(const QUrl &url, const QString &title, const QPixmap &pixmap, QObject *parent)
{
	PlatformIntegration *integration(Application::getPlatformIntegration());

	if (integration)
	{
		integration->startLinkDrag(url, title, pixmap, parent);

		return;
	}

	QDrag *drag(new QDrag(parent));
	QMimeData *mimeData(new QMimeData());
	mimeData->setText(url.toString());
	mimeData->setUrls({url});

	if (!title.isEmpty())
	{
		mimeData->setProperty("x-url-title", title);
	}

	mimeData->setProperty("x-url-string", url.toString());

	drag->setMimeData(mimeData);
	drag->setPixmap(pixmap);
	drag->exec(Qt::CopyAction);
}

QString matchUrl(const QUrl &url, const QString &prefix)
{
	QString match(url.toString());

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

	const QRegularExpression expression(QLatin1String("_([0-9]+)$"));
	const QRegularExpressionMatch match(expression.match(identifier));
	int number(2);

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

QString createErrorPage(const ErrorPageInformation &information)
{
	QString title(information.title);
	QString introduction;
	QStringList hints;

	if (information.type == ErrorPageInformation::ConnectionInsecureError)
	{
		introduction = QCoreApplication::translate("utils", "The owner of <strong>%1</strong> has configured their page improperly. To protect your information from being stolen, connection to this website was aborted.").arg(information.url.host().isEmpty() ? QLatin1String("localhost") : information.url.host());
	}
	else if (information.type == ErrorPageInformation::FraudAttemptError)
	{
		introduction = QCoreApplication::translate("utils", "This web page at <strong>%1</strong> has been reported as a web forgery. To protect your information from being stolen, connection to this website was aborted.").arg(information.url.host().isEmpty() ? QLatin1String("localhost") : information.url.host());
	}
	else
	{
		introduction = QCoreApplication::translate("utils", "You tried to access the address <a href=\"%1\">%1</a>, which is currently unavailable. Please make sure that the web address (URL) is correctly spelled and punctuated, then try reloading the page.").arg(information.url.toDisplayString());

		if (information.url.isLocalFile())
		{
			hints = QStringList({QCoreApplication::translate("utils", "Check the file name for capitalization or other typing errors."), QCoreApplication::translate("utils", "Check to see if the file was moved, renamed or deleted.")});
		}
		else
		{
			hints = QStringList({QCoreApplication::translate("utils", "Check the address for typing errors."), QCoreApplication::translate("utils", "Make sure your internet connection is active and check whether other applications that rely on the same connection are working."), QCoreApplication::translate("utils", "Check that the setup of any internet security software is correct and does not interfere with ordinary web browsing."), QCoreApplication::translate("utils", "Try pressing the F12 key on your keyboard and disabling proxy servers, unless you know that you are required to use a proxy to connect to the internet, and then reload the page.")});
		}
	}

	if (title.isEmpty())
	{
		switch (information.type)
		{
			case ErrorPageInformation::ConnectionInsecureError:
				title = QCoreApplication::translate("utils", "Connection is insecure");

				break;
			case ErrorPageInformation::ConnectionRefusedError:
				title = QCoreApplication::translate("utils", "Connection refused");

				break;
			case ErrorPageInformation::FileNotFoundError:
				title = QCoreApplication::translate("utils", "File not found");

				break;
			case ErrorPageInformation::FraudAttemptError:
				title = QCoreApplication::translate("utils", "Fraud attempt");

				break;
			case ErrorPageInformation::ServerNotFoundError:
				title = QCoreApplication::translate("utils", "Server not found");

				break;
			case ErrorPageInformation::UnsupportedAddressTypeError:
				title = QCoreApplication::translate("utils", "Unsupported address type");

				break;
			default:
				title = QCoreApplication::translate("utils", "Network error");

				break;
		}
	}

	QFile file(SessionsManager::getReadableDataPath(QLatin1String("files/error.html")));
	file.open(QIODevice::ReadOnly | QIODevice::Text);

	QTextStream stream(&file);
	stream.setCodec("UTF-8");

	QHash<QString, QString> variables;
	variables[QLatin1String("dir")] = (QGuiApplication::isLeftToRight() ? QLatin1String("ltr") : QLatin1String("rtl"));
	variables[QLatin1String("title")] = QCoreApplication::translate("utils", "Error");
	variables[QLatin1String("header")] = title;
	variables[QLatin1String("introduction")] = introduction;

	QString mainTemplate(stream.readAll());
	const QRegularExpression advancedActionsExpression(QLatin1String("<!--advancedActions:begin-->(.*)<!--advancedActions:end-->"), (QRegularExpression::DotMatchesEverythingOption | QRegularExpression::MultilineOption));
	const QRegularExpression basicActionsExpression(QLatin1String("<!--basicActions:begin-->(.*)<!--basicActions:end-->"), (QRegularExpression::DotMatchesEverythingOption | QRegularExpression::MultilineOption));
	const QRegularExpression descriptionExpression(QLatin1String("<!--description:begin-->(.*)<!--description:end-->"), (QRegularExpression::DotMatchesEverythingOption | QRegularExpression::MultilineOption));
	const QRegularExpression hintsExpression(QLatin1String("<!--hints:begin-->(.*)<!--hints:end-->"), (QRegularExpression::DotMatchesEverythingOption | QRegularExpression::MultilineOption));

	if (information.actions.isEmpty() && information.description.isEmpty())
	{
		mainTemplate.remove(advancedActionsExpression);
		mainTemplate.remove(basicActionsExpression);
	}
	else
	{
		const QRegularExpression actionExpression(QLatin1String("<!--action:begin-->(.*)<!--action:end-->"), (QRegularExpression::DotMatchesEverythingOption | QRegularExpression::MultilineOption));
		const QString actionTemplate(actionExpression.match(mainTemplate).captured(1));
		QString basicActionsHtml;
		QString advancedActionsHtml;
		QVector<ErrorPageInformation::PageAction> actions(information.actions);

		if (!information.description.isEmpty())
		{
			ErrorPageInformation::PageAction action;
			action.name = QLatin1String("advanced");
			action.title = QCoreApplication::translate("utils", "Advanced");

			actions.append(action);
		}

		for (int i = 0; i < actions.count(); ++i)
		{
			QString actionHtml(actionTemplate);
			actionHtml.replace(QLatin1String("{action}"), actions.at(i).name);
			actionHtml.replace(QLatin1String("{text}"), actions.at(i).title);
			actionHtml.replace(QLatin1String("{attributes}"), ((actions.at(i).type == ErrorPageInformation::MainAction) ? QLatin1String(" autofocus") : QString()));

			if (actions.at(i).type != ErrorPageInformation::AdvancedAction)
			{
				basicActionsHtml.append(actionHtml);
			}
			else
			{
				advancedActionsHtml.append(actionHtml);
			}
		}

		if (advancedActionsHtml.isEmpty())
		{
			mainTemplate.remove(advancedActionsExpression);
		}
		else
		{
			mainTemplate.replace(QLatin1String("{advancedActions}"), advancedActionsHtml);
			mainTemplate.replace(advancedActionsExpression, QLatin1String("\\1"));
		}

		if (basicActionsHtml.isEmpty())
		{
			mainTemplate.remove(basicActionsExpression);
		}
		else
		{
			mainTemplate.replace(actionExpression, basicActionsHtml);
			mainTemplate.replace(basicActionsExpression, QLatin1String("\\1"));
		}
	}

	if (information.description.isEmpty())
	{
		mainTemplate.remove(descriptionExpression);
	}
	else
	{
		variables[QLatin1String("description")] = information.description.join(QLatin1String("<br>\n"));

		mainTemplate.replace(descriptionExpression, QLatin1String("\\1"));
	}

	if (hints.isEmpty())
	{
		mainTemplate.remove(hintsExpression);
	}
	else
	{
		const QRegularExpression hintExpression(QLatin1String("<!--hint:begin-->(.*)<!--hint:end-->"), (QRegularExpression::DotMatchesEverythingOption | QRegularExpression::MultilineOption));
		const QString hintTemplate(hintExpression.match(mainTemplate).captured(1));
		QString hintsHtml;

		for (int i = 0; i < hints.count(); ++i)
		{
			hintsHtml.append(QString(hintTemplate).replace(QLatin1String("{hint}"), hints.at(i)));
		}

		mainTemplate.replace(hintExpression, hintsHtml);
		mainTemplate.replace(hintsExpression, QLatin1String("\\1"));
	}

	QHash<QString, QString>::iterator iterator;

	for (iterator = variables.begin(); iterator != variables.end(); ++iterator)
	{
		mainTemplate.replace(QStringLiteral("{%1}").arg(iterator.key()), iterator.value());
	}

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

QString savePixmapAsDataUri(const QPixmap &pixmap)
{
	QByteArray data;
	QBuffer buffer(&data);
	buffer.open(QIODevice::WriteOnly);

	pixmap.save(&buffer, "PNG");

	return QStringLiteral("data:image/png;base64,%1").arg(QString(data.toBase64()));
}

QString formatElapsedTime(int value)
{
	QTime time(0, 0);
	time = time.addSecs(value);

	if (value > 3600)
	{
		QString string(time.toString(QLatin1String("hh:mm:ss")));

		if (value > SECONDS_IN_DAY)
		{
			string = QCoreApplication::translate("utils", "%n days %1", "", (qFloor(static_cast<qreal>(value) / SECONDS_IN_DAY))).arg(string);
		}

		return string;
	}

	return time.toString(QLatin1String("mm:ss"));
}

QString formatDateTime(const QDateTime &dateTime, QString format, bool allowFancy)
{
	if (allowFancy && SettingsManager::getOption(SettingsManager::Interface_UseFancyDateTimeFormatOption).toBool() && dateTime.date() > QDate::currentDate().addDays(-7))
	{
		if (dateTime.date() == QDate::currentDate())
		{
			return QCoreApplication::translate("utils", "Today at %1").arg(dateTime.toString(QLatin1String("hh:mm")));
		}

		if (dateTime.date() == QDate::currentDate().addDays(-1))
		{
			return QCoreApplication::translate("utils", "Yesterday at %1").arg(dateTime.toString(QLatin1String("hh:mm")));
		}

		QString dayOfWeek(dateTime.toString(QLatin1String("dddd")));
		dayOfWeek[0] = dayOfWeek.at(0).toUpper();

		return QCoreApplication::translate("utils", "%1 at %2").arg(dayOfWeek, dateTime.toString(QLatin1String("hh:mm")));
	}

	if (format.isEmpty())
	{
		format = SettingsManager::getOption(SettingsManager::Interface_DateTimeFormatOption).toString();
	}

	return (format.isEmpty() ? QLocale().toString(dateTime, QLocale::ShortFormat) : QLocale().toString(dateTime, format));
}

QString formatUnit(qint64 value, bool isSpeed, int precision, bool appendRaw)
{
	if (value < 0)
	{
		return QString(QLatin1Char('?'));
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

QString formatFileTypes(const QStringList &filters)
{
	if (filters.isEmpty())
	{
		return QCoreApplication::translate("utils", "All files (*)");
	}

	QString result(filters.join(QLatin1String(";;")));

	if (!result.contains(QLatin1String("(*)")))
	{
		result.append(QLatin1String(";;") + QCoreApplication::translate("utils", "All files (*)"));
	}

	return result;
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

QPixmap loadPixmapFromDataUri(const QString &data)
{
	return QPixmap::fromImage(QImage::fromData(QByteArray::fromBase64(data.mid(data.indexOf(QLatin1String("base64,")) + 7).toUtf8())));
}

QStringList getOpenPaths(const QStringList &fileNames, QStringList filters, bool selectMultiple)
{
	Q_UNUSED(fileNames)

	QStringList paths;

	filters.append(QCoreApplication::translate("utils", "All files (*)"));

	if (selectMultiple)
	{
		paths = QFileDialog::getOpenFileNames(Application::getActiveWindow(), QCoreApplication::translate("utils", "Open Files"), SettingsManager::getOption(SettingsManager::Paths_OpenFileOption).toString(), filters.join(QLatin1String(";;")));
	}
	else
	{
		const QString path(QFileDialog::getOpenFileName(Application::getActiveWindow(), QCoreApplication::translate("utils", "Open File"), SettingsManager::getOption(SettingsManager::Paths_OpenFileOption).toString(), filters.join(QLatin1String(";;"))));

		if (!path.isEmpty())
		{
			paths = QStringList(path);
		}
	}

	if (!paths.isEmpty())
	{
		SettingsManager::setOption(SettingsManager::Paths_OpenFileOption, QFileInfo(paths.first()).dir().canonicalPath());
	}

	return paths;
}

QVector<QUrl> extractUrls(const QMimeData *mimeData)
{
	if (mimeData->property("x-url-string").isNull())
	{
		return mimeData->urls().toVector();
	}

	return QVector<QUrl>({QUrl(mimeData->property("x-url-string").toString())});
}

QVector<ApplicationInformation> getApplicationsForMimeType(const QMimeType &mimeType)
{
	PlatformIntegration *integration(Application::getPlatformIntegration());

	if (integration)
	{
		return integration->getApplicationsForMimeType(mimeType);
	}

	return QVector<ApplicationInformation>();
}

SaveInformation getSavePath(const QString &fileName, QString path, QStringList filters, bool forceAsk)
{
	SaveInformation information;

	if (!path.isEmpty())
	{
		path.append(QDir::separator() + fileName);
	}

	if (filters.isEmpty())
	{
		QString suffix(QMimeDatabase().suffixForFileName(fileName));

		if (suffix.isEmpty())
		{
			suffix = QFileInfo(fileName).suffix();
		}

		if (!suffix.isEmpty())
		{
			filters.append(QCoreApplication::translate("utils", "%1 files (*.%2)").arg(suffix.toUpper()).arg(suffix));
		}
	}

	filters.append(QCoreApplication::translate("utils", "All files (*)"));

	do
	{
		if (path.isEmpty() || forceAsk)
		{
			QFileDialog dialog(Application::getActiveWindow(), QCoreApplication::translate("utils", "Save File"), SettingsManager::getOption(SettingsManager::Paths_SaveFileOption).toString() + QDir::separator() + fileName);
			dialog.setNameFilters(filters);
			dialog.setFileMode(QFileDialog::AnyFile);
			dialog.setAcceptMode(QFileDialog::AcceptSave);

			if (dialog.exec() == QDialog::Rejected || dialog.selectedFiles().isEmpty())
			{
				break;
			}

			information.filter = dialog.selectedNameFilter();
			information.canOverwriteExisting = true;

			path = dialog.selectedFiles().value(0);
		}

		const bool isExisting(QFile::exists(path));

		if (TransfersManager::isDownloading(QString(), path))
		{
			path = QString();

			if (QMessageBox::warning(Application::getActiveWindow(), QCoreApplication::translate("utils", "Warning"), QCoreApplication::translate("utils", "This path is already used by different download, pick another one."), (QMessageBox::Ok | QMessageBox::Cancel)) == QMessageBox::Cancel)
			{
				break;
			}
		}
		else if ((isExisting && !QFileInfo(path).isWritable()) || (!isExisting && !QFileInfo(QFileInfo(path).dir().path()).isWritable()))
		{
			path = QString();

			if (QMessageBox::warning(Application::getActiveWindow(), QCoreApplication::translate("utils", "Warning"), QCoreApplication::translate("utils", "Target path is not writable.\nSelect another one."), (QMessageBox::Ok | QMessageBox::Cancel)) == QMessageBox::Cancel)
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
	while (true);

	if (!path.isEmpty())
	{
		SettingsManager::setOption(SettingsManager::Paths_SaveFileOption, QFileInfo(path).dir().canonicalPath());
	}

	information.path = path;

	return information;
}

bool isUrlEmpty(const QUrl &url)
{
	return (url.isEmpty() || (url.scheme() == QLatin1String("about") && (url.path().isEmpty() || url.path() == QLatin1String("blank") || url.path() == QLatin1String("start"))));
}

}

}
