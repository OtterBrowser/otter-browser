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

#include "Utils.h"
#include "Application.h"
#include "PlatformIntegration.h"
#include "TransfersManager.h"
#include "../ui/MainWindow.h"

#include <QtCore/QBuffer>
#include <QtCore/QCoreApplication>
#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeDatabase>
#include <QtCore/QRegularExpression>
#if QT_VERSION >= 0x060000
#include <QtCore5Compat/QTextCodec>
#else
#include <QtCore/QTextCodec>
#endif
#include <QtCore/QTextStream>
#include <QtCore/QTime>
#include <QtCore/QtMath>
#include <QtGui/QDesktopServices>
#include <QtGui/QDrag>
#include <QtGui/QScreen>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QToolTip>

namespace Otter
{

namespace Utils
{

void removeFiles(const QStringList &paths)
{
	for (int i = 0; i < paths.count(); ++i)
	{
		QFile::remove(paths.at(i));
	}
}

void runApplication(const QString &command, const QUrl &url)
{
	if (command.isEmpty() && !url.isValid())
	{
		return;
	}

	const PlatformIntegration *integration(Application::getPlatformIntegration());

	if (integration)
	{
		integration->runApplication(command, url);
	}
	else
	{
		QDesktopServices::openUrl(url);
	}
}

void startLinkDrag(const QUrl &url, const QString &title, const QPixmap &pixmap, QObject *parent)
{
	const PlatformIntegration *integration(Application::getPlatformIntegration());

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

void showToolTip(const QPoint &position, const QString &text, QWidget *widget, const QRect &rectangle)
{
	QToolTip::showText(position, QFontMetrics(QToolTip::font()).elidedText(text, Qt::ElideRight, (widget->screen()->geometry().width() / 2)), widget, rectangle);
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

	return {};
}

QString createIdentifier(const QString &source, const QStringList &exclude, bool toLowerCase)
{
	QString identifier;

	if (!source.isEmpty())
	{
		identifier = source;

		if (toLowerCase)
		{
			identifier = source.toLower();
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

	QString result;

	do
	{
		result = identifier + QLatin1Char('_') + QString::number(number);

		++number;
	}
	while (exclude.contains(result));

	return result;
}

QString createErrorPage(const ErrorPageInformation &information)
{
	QString title(information.title);
	QString introduction;
	QStringList hints;

	switch (information.type)
	{
		case ErrorPageInformation::BlockedContentError:
			introduction = QCoreApplication::translate("utils", "You tried to access the address <a href=\"%1\">%1</a>, which was blocked by content blocker.").arg(information.url.toDisplayString());

			break;
		case ErrorPageInformation::ConnectionInsecureError:
			introduction = QCoreApplication::translate("utils", "The owner of <strong>%1</strong> has configured their page improperly. To protect your information from being stolen, connection to this website was aborted.").arg(extractHost(information.url));

			break;
		case ErrorPageInformation::FraudAttemptError:
			introduction = QCoreApplication::translate("utils", "This web page at <strong>%1</strong> has been reported as a web forgery. To protect your information from being stolen, connection to this website was aborted.").arg(extractHost(information.url));

			break;
		default:
			introduction = QCoreApplication::translate("utils", "You tried to access the address <a href=\"%1\">%1</a>, which is currently unavailable. Please make sure that the web address (URL) is correctly spelled and punctuated, then try reloading the page.").arg(information.url.toDisplayString());

			if (information.url.isLocalFile())
			{
				hints = QStringList({QCoreApplication::translate("utils", "Check the file name for capitalization or other typing errors."), QCoreApplication::translate("utils", "Check to see if the file was moved, renamed or deleted.")});
			}
			else
			{
				hints = QStringList({QCoreApplication::translate("utils", "Check the address for typing errors."), QCoreApplication::translate("utils", "Make sure your internet connection is active and check whether other applications that rely on the same connection are working."), QCoreApplication::translate("utils", "Check that the setup of any internet security software is correct and does not interfere with ordinary web browsing."), QCoreApplication::translate("utils", "Try pressing the F12 key on your keyboard and disabling proxy servers, unless you know that you are required to use a proxy to connect to the internet, and then reload the page.")});
			}

			break;
	}

	if (title.isEmpty())
	{
		switch (information.type)
		{
			case ErrorPageInformation::BlockedContentError:
				title = QCoreApplication::translate("utils", "Address blocked");

				break;
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
			action.type = ErrorPageInformation::OtherAction;

			actions.append(action);
		}

		for (int i = 0; i < actions.count(); ++i)
		{
			const ErrorPageInformation::PageAction action(actions.at(i));
			QString actionHtml(actionTemplate);
			actionHtml.replace(QLatin1String("{action}"), action.name);
			actionHtml.replace(QLatin1String("{text}"), action.title);
			actionHtml.replace(QLatin1String("{attributes}"), ((action.type == ErrorPageInformation::MainAction) ? QLatin1String(" autofocus") : QString()));

			if (action.type != ErrorPageInformation::AdvancedAction)
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

	QHash<QString, QString> variables({{QLatin1String("dir"), (QGuiApplication::isLeftToRight() ? QLatin1String("ltr") : QLatin1String("rtl"))}, {QLatin1String("title"), QCoreApplication::translate("utils", "Error")}, {QLatin1String("header"), title}, {QLatin1String("introduction"), introduction}});

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
		mainTemplate.replace(QLatin1Char('{') + iterator.key() + QLatin1Char('}'), iterator.value());
	}

	return mainTemplate;
}

QString appendShortcut(const QString &text, const QKeySequence &shortcut)
{
	if (shortcut.isEmpty())
	{
		return text;
	}

	return text + QLatin1String(" (") + shortcut.toString(QKeySequence::NativeText) + QLatin1Char(')');
}

QString elideText(const QString &text, const QFontMetrics &fontMetrics, QWidget *widget, int maximumWidth, int minimumWidth)
{
	if (widget && maximumWidth < 0)
	{
		maximumWidth = (widget->screen()->geometry().width() / 4);
	}

	return fontMetrics.elidedText(text, Qt::ElideRight, qMax(minimumWidth, maximumWidth));
}

QString substitutePlaceholders(QString text, const QHash<QString, QString> &placeholders)
{
	QHash<QString, QString>::const_iterator iterator;

	for (iterator = placeholders.begin(); iterator != placeholders.end(); ++iterator)
	{
		text.replace(QLatin1Char('{') + iterator.key() + QLatin1Char('}'), iterator.value());
	}

	return text;
}

QString savePixmapAsDataUri(const QPixmap &pixmap)
{
	QByteArray data;
	QBuffer buffer(&data);
	buffer.open(QIODevice::WriteOnly);

	pixmap.save(&buffer, "PNG");

	return QLatin1String("data:image/png;base64,") + data.toBase64();
}

QString extractHost(const QUrl &url)
{
	return (url.isLocalFile() ? QLatin1String("localhost") : url.host());
}

QString formatElapsedTime(int value)
{
	if (value < 0)
	{
		return {};
	}

	QTime time(0, 0);
	time = time.addSecs(value);

	if (value > 3600)
	{
		QString string(time.toString(QLatin1String("hh:mm:ss")));

		if (value > 86400)
		{
			string = QCoreApplication::translate("utils", "%n days %1", "", (qFloor(static_cast<qreal>(value) / 86400))).arg(string);
		}

		return string;
	}

	return time.toString(QLatin1String("mm:ss"));
}

QString formatDateTime(const QDateTime &dateTime, QString format, bool allowFancy)
{
	const QDateTime localDateTime(dateTime.toLocalTime());

	if (allowFancy && SettingsManager::getOption(SettingsManager::Interface_UseFancyDateTimeFormatOption).toBool() && localDateTime.date() > QDate::currentDate().addDays(-7))
	{
		if (localDateTime.date() == QDate::currentDate())
		{
			return QCoreApplication::translate("utils", "Today at %1").arg(localDateTime.toString(QLatin1String("hh:mm")));
		}

		if (localDateTime.date() == QDate::currentDate().addDays(-1))
		{
			return QCoreApplication::translate("utils", "Yesterday at %1").arg(localDateTime.toString(QLatin1String("hh:mm")));
		}

		QString dayOfWeek(localDateTime.toString(QLatin1String("dddd")));
		dayOfWeek[0] = dayOfWeek.at(0).toUpper();

		return QCoreApplication::translate("utils", "%1 at %2").arg(dayOfWeek, dateTime.toString(QLatin1String("hh:mm")));
	}

	if (format.isEmpty())
	{
		format = SettingsManager::getOption(SettingsManager::Interface_DateTimeFormatOption).toString();
	}

	const QLocale locale;

	return (format.isEmpty() ? locale.toString(localDateTime, QLocale::ShortFormat) : locale.toString(localDateTime, format));
}

QString formatUnit(qint64 value, bool isSpeed, int precision, bool appendRaw)
{
	if (value < 0)
	{
		return {QLatin1Char('?')};
	}

	if (value > 1024)
	{
		if (value > 1048576)
		{
			if (value > 1073741824)
			{
				return QCoreApplication::translate("utils", (isSpeed ? "%1 GB/s" : "%1 GB")).arg((static_cast<qreal>(value) / static_cast<qreal>(1073741824)), 0, 'f', precision) + (appendRaw ? QString(isSpeed ? QLatin1String(" (%1 B/s)") : QLatin1String(" (%1 B)")).arg(value) : QString());
			}

			return QCoreApplication::translate("utils", (isSpeed ? "%1 MB/s" : "%1 MB")).arg((static_cast<qreal>(value) / static_cast<qreal>(1048576)), 0, 'f', precision) + (appendRaw ? QString(isSpeed ? QLatin1String(" (%1 B/s)") : QLatin1String(" (%1 B)")).arg(value) : QString());
		}

		return QCoreApplication::translate("utils", (isSpeed ? "%1 KB/s" : "%1 KB")).arg((static_cast<qreal>(value) / static_cast<qreal>(1024)), 0, 'f', precision) + (appendRaw ? QString(isSpeed ? QLatin1String(" (%1 B/s)") : QLatin1String(" (%1 B)")).arg(value) : QString());
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

QString normalizeObjectName(QString name, const QString &suffix)
{
	name.remove(QLatin1String("Otter__"));

	if (!suffix.isEmpty() && name.endsWith(suffix))
	{
		name.remove((name.length() - suffix.length()), suffix.length());
	}

	return name;
}

QString normalizePath(const QString &path)
{
	if (path == QString(QLatin1Char('~')) || QDir::toNativeSeparators(path).startsWith(QLatin1Char('~') + QDir::separator()))
	{
		return QDir::homePath() + path.mid(1);
	}

	return path;
}

QString getTopLevelDomain(const QUrl &url)
{
	return url.topLevelDomain();
}

QString getStandardLocation(QStandardPaths::StandardLocation type)
{
	const QStringList paths(QStandardPaths::standardLocations(type));

	return (paths.isEmpty() ? QString() : paths.value(0));
}

QUrl expandUrl(const QUrl &url)
{
	if (!url.isValid() || !url.scheme().isEmpty())
	{
		return url;
	}

	if (!url.path().startsWith(QLatin1Char('/')))
	{
		QUrl httpUrl(url);
		httpUrl.setScheme(QLatin1String("http"));

		return httpUrl;
	}

	QUrl localUrl(url);
	localUrl.setScheme(QLatin1String("file"));

	return localUrl;
}

QUrl normalizeUrl(QUrl url)
{
	url = url.adjusted(QUrl::RemoveFragment | QUrl::NormalizePathSegments | QUrl::StripTrailingSlash);

	if (url.path() == QLatin1String("/"))
	{
		url.setPath({});
	}

	return url;
}

QColor createColor(const QUrl &url)
{
	const QByteArray hash(QCryptographicHash::hash(url.host().toUtf8(), QCryptographicHash::Md5));

	return {hash.at(0), hash.at(1), hash.at(2)};
}

QLocale createLocale(const QString &name)
{
	if (name == QLatin1String("pt"))
	{
		return {QLocale::Portuguese, QLocale::Portugal};
	}

	return {name};
}

QPixmap loadPixmapFromDataUri(const QString &data)
{
	return QPixmap::fromImage(QImage::fromData(QByteArray::fromBase64(data.mid(data.indexOf(QLatin1String("base64,")) + 7).toUtf8())));
}

QFont multiplyFontSize(QFont font, qreal multiplier)
{
	if (font.pixelSize() > 0)
	{
		font.setPixelSize(font.pixelSize() * multiplier);
	}
	else
	{
		font.setPointSize(font.pointSize() * multiplier);
	}

	return font;
}

QStringList getCharacterEncodings()
{
	const QVector<int> textCodecs({106, 1015, 1017, 4, 5, 6, 7, 8, 82, 10, 85, 12, 13, 109, 110, 112, 2250, 2251, 2252, 2253, 2254, 2255, 2256, 2257, 2258, 18, 39, 17, 38, 2026});
	QStringList encodings;
	encodings.reserve(textCodecs.count());

	for (int i = 0; i < textCodecs.count(); ++i)
	{
		const QTextCodec *codec(QTextCodec::codecForMib(textCodecs.at(i)));

		if (codec)
		{
			encodings.append(QString::fromLatin1(codec->name()));
		}
	}

	return encodings;
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
		SettingsManager::setOption(SettingsManager::Paths_OpenFileOption, QFileInfo(paths.value(0)).dir().canonicalPath());
	}

	return paths;
}

QVector<QUrl> extractUrls(const QMimeData *mimeData)
{
	if (mimeData->property("x-url-string").isNull())
	{
		return mimeData->urls().toVector();
	}

	return {QUrl(mimeData->property("x-url-string").toString())};
}

QVector<ApplicationInformation> getApplicationsForMimeType(const QMimeType &mimeType)
{
	PlatformIntegration *integration(Application::getPlatformIntegration());

	if (integration)
	{
		return integration->getApplicationsForMimeType(mimeType);
	}

	return {};
}

SaveInformation getSavePath(const QString &fileName, const QString &directory, QStringList filters, bool forceAsk)
{
	SaveInformation information;
	QString path(directory.isEmpty() ? QString() : directory + QDir::separator() + fileName);

	if (filters.isEmpty())
	{
		QString suffix(QMimeDatabase().suffixForFileName(fileName));

		if (suffix.isEmpty())
		{
			suffix = QFileInfo(fileName).suffix();
		}

		if (!suffix.isEmpty())
		{
			filters.append(QCoreApplication::translate("utils", "%1 files (*.%2)").arg(suffix.toUpper(), suffix));
		}
	}

	filters.append(QCoreApplication::translate("utils", "All files (*)"));

	do
	{
		if (forceAsk || path.isEmpty())
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
			information.canSave = true;

			path = dialog.selectedFiles().value(0);
		}

		const bool fileExists(QFile::exists(path));

		if (TransfersManager::isDownloading({}, path))
		{
			path.clear();

			if (QMessageBox::warning(Application::getActiveWindow(), QCoreApplication::translate("utils", "Warning"), QCoreApplication::translate("utils", "This path is already used by different download, pick another one."), (QMessageBox::Ok | QMessageBox::Cancel)) == QMessageBox::Cancel)
			{
				break;
			}
		}
		else if ((fileExists && !QFileInfo(path).isWritable()) || (!fileExists && !QFileInfo(QFileInfo(path).dir().path()).isWritable()))
		{
			path.clear();

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

	if (path.isEmpty())
	{
		information.canSave = false;
	}
	else
	{
		SettingsManager::setOption(SettingsManager::Paths_SaveFileOption, QFileInfo(path).dir().canonicalPath());
	}

	information.path = path;

	return information;
}

qreal calculatePercent(qint64 amount, qint64 total, int multiplier)
{
	return ((static_cast<qreal>(amount) / static_cast<qreal>(total)) * multiplier);
}

bool ensureDirectoryExists(const QString &path)
{
	if (QFile::exists(path))
	{
		return true;
	}

	return QDir().mkpath(path);
}

bool isDomainTheSame(const QUrl &firstUrl, const QUrl &secondUrl)
{
	const QString firstTld(getTopLevelDomain(firstUrl));
	const QString secondTld(getTopLevelDomain(secondUrl));

	if (firstTld != secondTld)
	{
		return false;
	}

	QString firstDomain(QLatin1Char('.') + firstUrl.host().toLower());
	firstDomain.remove((firstDomain.length() - firstTld.length()), firstTld.length());

	QString secondDomain(QLatin1Char('.') + secondUrl.host().toLower());
	secondDomain.remove((secondDomain.length() - secondTld.length()), secondTld.length());

	return firstDomain.section(QLatin1Char('.'), -1) == secondDomain.section(QLatin1Char('.'), -1);
}

bool isUrl(const QString &text)
{
	return QRegularExpression(QLatin1String(R"(^[^\s]+\.[^\s]{2,}$)")).match(text).hasMatch();
}

bool isUrlAmbiguous(const QUrl &url)
{
	return (!url.isLocalFile() && url.host(QUrl::FullyEncoded) != url.host(QUrl::FullyDecoded));
}

bool isUrlEmpty(const QUrl &url)
{
	return (url.isEmpty() || (url.scheme() == QLatin1String("about") && (url.path().isEmpty() || url.path() == QLatin1String("blank") || url.path() == QLatin1String("start"))));
}

}

EnumeratorMapper::EnumeratorMapper(const QMetaEnum &enumeration, const QString &suffix) :
	m_enumerator(enumeration),
	m_suffix(suffix)
{
}

QString EnumeratorMapper::mapToName(int value, bool lowercaseFirst) const
{
	QString name(m_enumerator.valueToKey(value));

	if (name.isEmpty())
	{
		return {};
	}

	name.chop(m_suffix.length());

	if (lowercaseFirst)
	{
		name[0] = name.at(0).toLower();
	}

	return name;
}

int EnumeratorMapper::mapToValue(const QString &name, bool checkSuffix) const
{
	if (name.isEmpty())
	{
		return -1;
	}

	QString key(name);
	key[0] = key.at(0).toUpper();

	if (!checkSuffix || (!m_suffix.isEmpty() && !key.endsWith(m_suffix)))
	{
		key += m_suffix;
	}

	return m_enumerator.keyToValue(key.toLatin1());
}

}
