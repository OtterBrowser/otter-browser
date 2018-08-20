/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "QtWebEnginePage.h"
#include "QtWebEngineWebBackend.h"
#include "QtWebEngineWebWidget.h"
#include "../../../../core/Console.h"
#include "../../../../core/ContentFiltersManager.h"
#include "../../../../core/HandlersManager.h"
#include "../../../../core/HistoryManager.h"
#include "../../../../core/ThemesManager.h"
#include "../../../../core/UserScript.h"
#include "../../../../core/Utils.h"
#include "../../../../ui/ContentsDialog.h"
#include "../../../../ui/LineEditWidget.h"

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtWebEngineWidgets/QWebEngineHistory>
#include <QtWebEngineWidgets/QWebEngineProfile>
#include <QtWebEngineWidgets/QWebEngineScript>
#include <QtWebEngineWidgets/QWebEngineScriptCollection>
#include <QtWebEngineWidgets/QWebEngineSettings>
#include <QtWidgets/QMessageBox>

namespace Otter
{

QtWebEnginePage::QtWebEnginePage(bool isPrivate, QtWebEngineWebWidget *parent) : QWebEnginePage((isPrivate ? new QWebEngineProfile(parent) : QWebEngineProfile::defaultProfile()), parent),
	m_widget(parent),
	m_previousNavigationType(QtWebEnginePage::NavigationTypeOther),
	m_isIgnoringJavaScriptPopups(false),
	m_isViewingMedia(false),
	m_isPopup(false)
{
	if (isPrivate && m_widget)
	{
		connect(profile(), &QWebEngineProfile::downloadRequested, qobject_cast<QtWebEngineWebBackend*>(m_widget->getBackend()), &QtWebEngineWebBackend::handleDownloadRequested);
	}

	connect(this, &QtWebEnginePage::loadFinished, this, &QtWebEnginePage::handleLoadFinished);
}

void QtWebEnginePage::validatePopup(const QUrl &url)
{
	QtWebEnginePage *page(qobject_cast<QtWebEnginePage*>(sender()));

	if (page)
	{
		m_popups.removeAll(page);

		page->deleteLater();
	}

	const QVector<int> profiles(ContentFiltersManager::getProfileIdentifiers(m_widget->getOption(SettingsManager::ContentBlocking_ProfilesOption, m_widget->getUrl()).toStringList()));

	if (!profiles.isEmpty())
	{
		const ContentFiltersManager::CheckResult result(ContentFiltersManager::checkUrl(profiles, m_widget->getUrl(), url, NetworkManager::PopupType));

		if (result.isBlocked)
		{
			Console::addMessage(QCoreApplication::translate("main", "Request blocked by rule from profile %1:\n%2").arg(ContentFiltersManager::getProfile(result.profile)->getTitle()).arg(result.rule), Console::NetworkCategory, Console::LogLevel, url.url(), -1, (m_widget ? m_widget->getWindowIdentifier() : 0));

			return;
		}
	}

	const QString popupsPolicy(SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption, Utils::extractHost(m_widget ? m_widget->getRequestedUrl() : QUrl())).toString());

	if (popupsPolicy == QLatin1String("ask"))
	{
		emit requestedPopupWindow(m_widget->getUrl(), url);
	}
	else
	{
		QtWebEngineWebWidget *widget(createWidget((popupsPolicy == QLatin1String("openAllInBackground")) ? (SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen) : SessionsManager::NewTabOpen));
		widget->setUrl(url);
	}
}

void QtWebEnginePage::markAsPopup()
{
	m_isPopup = true;
}

void QtWebEnginePage::javaScriptAlert(const QUrl &url, const QString &message)
{
	if (m_isIgnoringJavaScriptPopups)
	{
		return;
	}

	if (!m_widget || !m_widget->parentWidget())
	{
		QWebEnginePage::javaScriptAlert(url, message);

		return;
	}

	emit m_widget->needsAttention();

	ContentsDialog dialog(ThemesManager::createIcon(QLatin1String("dialog-information")), tr("JavaScript"), message, {}, QDialogButtonBox::Ok, nullptr, m_widget);
	dialog.setCheckBox(tr("Disable JavaScript popups"), false);

	connect(m_widget, &QtWebEngineWebWidget::aboutToReload, &dialog, &ContentsDialog::close);

	m_widget->showDialog(&dialog);

	if (dialog.getCheckBoxState())
	{
		m_isIgnoringJavaScriptPopups = true;
	}
}

void QtWebEnginePage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &note, int line, const QString &source)
{
	Console::MessageLevel mappedLevel(Console::LogLevel);

	if (level == WarningMessageLevel)
	{
		mappedLevel = Console::WarningLevel;
	}
	else if (level == ErrorMessageLevel)
	{
		mappedLevel = Console::ErrorLevel;
	}

	Console::addMessage(note, Console::JavaScriptCategory, mappedLevel, source, line, (m_widget ? m_widget->getWindowIdentifier() : 0));
}

void QtWebEnginePage::handleLoadFinished()
{
	m_isIgnoringJavaScriptPopups = false;

	const int historyIndex(history()->currentItemIndex());
	HistoryEntryInformation entry(m_history.value(historyIndex));

	if (entry.isValid)
	{
		entry.icon = icon();

		if (entry.identifier > 0 && !profile()->isOffTheRecord())
		{
			HistoryManager::updateEntry(entry.identifier, url(), (m_widget ? m_widget->getTitle() : title()), icon());
		}

		m_history[historyIndex] = entry;
	}

	toHtml([&](const QString &result)
	{
		if (m_widget)
		{
			const QUrl url(m_widget->getUrl());
			const ContentFiltersManager::CosmeticFiltersResult cosmeticFilters(ContentFiltersManager::getCosmeticFilters(ContentFiltersManager::getProfileIdentifiers(m_widget->getOption(SettingsManager::ContentBlocking_ProfilesOption).toStringList()), url));

			if (!cosmeticFilters.rules.isEmpty() || !cosmeticFilters.exceptions.isEmpty())
			{
				QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/hideElements.js"));

				if (file.open(QIODevice::ReadOnly))
				{
					runJavaScript(QString(file.readAll()).arg(createJavaScriptList(cosmeticFilters.exceptions)).arg(createJavaScriptList(cosmeticFilters.rules)));

					file.close();
				}
			}

			const QStringList blockedRequests(qobject_cast<QtWebEngineWebBackend*>(m_widget->getBackend())->getBlockedElements(url.host()));

			if (!blockedRequests.isEmpty())
			{
				QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/hideBlockedRequests.js"));

				if (file.open(QIODevice::ReadOnly))
				{
					runJavaScript(QString(file.readAll()).arg(createJavaScriptList(blockedRequests)));

					file.close();
				}
			}
		}

		QString string(url().toString());
		string.truncate(1000);

		const QRegularExpressionMatch match(QRegularExpression(QLatin1String(">(<img style=\"-webkit-user-select: none;(?: cursor: zoom-in;)?\"|<video controls=\"\" autoplay=\"\" name=\"media\"><source) src=\"") + QRegularExpression::escape(string)).match(result));
		const bool isViewingMedia(match.hasMatch());

		if (isViewingMedia && match.captured().startsWith(QLatin1String("><img")))
		{
			settings()->setAttribute(QWebEngineSettings::AutoLoadImages, true);
			settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);

			QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/imageViewer.js"));

			if (file.open(QIODevice::ReadOnly))
			{
				runJavaScript(file.readAll());

				file.close();
			}
		}

		if (isViewingMedia != m_isViewingMedia)
		{
			m_isViewingMedia = isViewingMedia;

			emit viewingMediaChanged(m_isViewingMedia);
		}
	});
}

void QtWebEnginePage::setHistory(const WindowHistoryInformation &history)
{
	m_history.clear();

	if (history.entries.count() == 0)
	{
		this->history()->clear();

		const QVector<UserScript*> scripts(UserScript::getUserScriptsForUrl(QUrl(QLatin1String("about:blank"))));

		for (int i = 0; i < scripts.count(); ++i)
		{
			runJavaScript(scripts.at(i)->getSource(), QWebEngineScript::UserWorld);
		}

		return;
	}

	m_history.reserve(history.entries.count());

	QByteArray byteArray;
	QDataStream stream(&byteArray, QIODevice::ReadWrite);
	stream << static_cast<int>(3) << history.entries.count() << history.index;

	for (int i = 0; i < history.entries.count(); ++i)
	{
		const WindowHistoryEntry entry(history.entries.at(i));

		stream << QUrl(entry.url) << entry.title << QByteArray() << static_cast<qint32>(0) << false << QUrl() << static_cast<qint32>(0) << QUrl(entry.url) << false << QDateTime::currentDateTime().toSecsSinceEpoch() << static_cast<int>(200);

		HistoryEntryInformation entryInformation;
		entryInformation.timeVisited = entry.time;
		entryInformation.position = entry.position;
		entryInformation.zoom = entry.zoom;
		entryInformation.isValid = true;

		m_history.append(entryInformation);
	}

	stream.device()->reset();
	stream >> *(this->history());

	this->history()->goToItem(this->history()->itemAt(history.index));
}

QWebEnginePage* QtWebEnginePage::createWindow(WebWindowType type)
{
	if (type != WebDialog)
	{
		const QString popupsPolicy((m_widget ? m_widget->getOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption) : SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption)).toString());

		if (!m_widget || m_widget->getLastUrlClickTime().isNull() || m_widget->getLastUrlClickTime().secsTo(QDateTime::currentDateTime()) > 1)
		{
			if (popupsPolicy == QLatin1String("blockAll"))
			{
				return nullptr;
			}

			if (popupsPolicy == QLatin1String("ask") || !m_widget->getOption(SettingsManager::ContentBlocking_ProfilesOption, m_widget->getUrl()).isNull())
			{
				QtWebEnginePage *page(new QtWebEnginePage(false, nullptr));
				page->markAsPopup();

				connect(page, &QtWebEnginePage::aboutToNavigate, this, &QtWebEnginePage::validatePopup);

				return page;
			}
		}

		return createWidget(SessionsManager::calculateOpenHints((popupsPolicy == QLatin1String("openAllInBackground")) ? (SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen) : SessionsManager::NewTabOpen))->getPage();
	}

	return QWebEnginePage::createWindow(type);
}

QtWebEngineWebWidget* QtWebEnginePage::createWidget(SessionsManager::OpenHints hints)
{
	QtWebEngineWebWidget *widget(nullptr);

	if (m_widget)
	{
		widget = qobject_cast<QtWebEngineWebWidget*>(m_widget->clone(false, m_widget->isPrivate(), SettingsManager::getOption(SettingsManager::Sessions_OptionsExludedFromInheritingOption).toStringList()));
	}
	else if (profile()->isOffTheRecord())
	{
		widget = new QtWebEngineWebWidget({{QLatin1String("hints"), SessionsManager::PrivateOpen}}, nullptr, nullptr);
	}
	else
	{
		widget = new QtWebEngineWebWidget({}, nullptr, nullptr);
	}

	widget->handleLoadStarted();

	emit requestedNewWindow(widget, hints, {});

	return widget;
}

QString QtWebEnginePage::createJavaScriptList(QStringList rules) const
{
	if (rules.isEmpty())
	{
		return {};
	}

	for (int i = 0; i < rules.count(); ++i)
	{
		rules[i] = rules[i].replace(QLatin1Char('\''), QLatin1String("\\'"));
	}

	return QLatin1Char('\'') + rules.join(QLatin1String("','")) + QLatin1Char('\'');
}

QString QtWebEnginePage::createScriptSource(const QString &path, const QStringList &parameters) const
{
	QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/") + path + QLatin1String(".js"));

	if (!file.open(QIODevice::ReadOnly))
	{
		return {};
	}

	QString script(file.readAll());

	file.close();

	for (int i = 0; i < parameters.count(); ++i)
	{
		script = script.arg(parameters.at(i));
	}

	return script;
}

QVariant QtWebEnginePage::runScriptSource(const QString &script)
{
	QVariant result;
	QEventLoop eventLoop;

	runJavaScript(script, [&](const QVariant &runResult)
	{
		result = runResult;

		eventLoop.quit();
	});

	if (m_widget)
	{
		connect(m_widget, &QtWebEngineWebWidget::aboutToReload, &eventLoop, &QEventLoop::quit);
	}

	connect(this, &QtWebEnginePage::destroyed, &eventLoop, &QEventLoop::quit);

	eventLoop.exec();

	return result;
}

QVariant QtWebEnginePage::runScriptFile(const QString &path, const QStringList &parameters)
{
	return runScriptSource(createScriptSource(path, parameters));
}

WindowHistoryInformation QtWebEnginePage::getHistory() const
{
	const int historyCount(history()->count());
	WindowHistoryInformation information;
	information.entries.reserve(historyCount);
	information.index = history()->currentItemIndex();

	const QString url(requestedUrl().toString());

	for (int i = 0; i < historyCount; ++i)
	{
		const QWebEngineHistoryItem item(history()->itemAt(i));
		const HistoryEntryInformation entryInformation(m_history.value(i));
		WindowHistoryEntry entry;
		entry.url = item.url().toString();
		entry.title = item.title();

		if (entryInformation.isValid)
		{
			entry.icon = entryInformation.icon;
			entry.position = entryInformation.position;
			entry.time = entryInformation.timeVisited;
			entry.zoom = entryInformation.zoom;
		}

		information.entries.append(entry);
	}

	if (m_widget && m_widget->getLoadingState() == WebWidget::OngoingLoadingState && url != history()->itemAt(history()->currentItemIndex()).url().toString())
	{
		WindowHistoryEntry entry;
		entry.url = url;
		entry.title = m_widget->getTitle();
		entry.icon = icon();
		entry.time = QDateTime::currentDateTimeUtc();
		entry.position = scrollPosition().toPoint();
		entry.zoom = static_cast<int>(zoomFactor() * 100);

		information.index = historyCount;
		information.entries.append(entry);
	}

	return information;
}

QStringList QtWebEnginePage::chooseFiles(FileSelectionMode mode, const QStringList &oldFiles, const QStringList &acceptedMimeTypes)
{
	Q_UNUSED(acceptedMimeTypes)

	return Utils::getOpenPaths(oldFiles, {}, (mode == FileSelectOpenMultiple));
}

bool QtWebEnginePage::acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame)
{
	if (m_isPopup)
	{
		emit aboutToNavigate(url, type);

		return false;
	}

	if (HandlersManager::handleUrl(url))
	{
		return false;
	}

	if (!isMainFrame)
	{
		return true;
	}

	if (url.scheme() == QLatin1String("javascript"))
	{
		runJavaScript(url.path());

		return false;
	}

	if (type == NavigationTypeReload && m_previousNavigationType == NavigationTypeFormSubmitted && SettingsManager::getOption(SettingsManager::Choices_WarnFormResendOption).toBool())
	{
		bool shouldCancelRequest(false);
		bool shouldWarnNextTime(true);

		if (m_widget)
		{
			ContentsDialog dialog(ThemesManager::createIcon(QLatin1String("dialog-warning")), tr("Question"), tr("Are you sure that you want to send form data again?"), tr("Do you want to resend data?"), (QDialogButtonBox::Yes | QDialogButtonBox::Cancel), nullptr, m_widget);
			dialog.setCheckBox(tr("Do not show this message again"), false);

			connect(m_widget, &QtWebEngineWebWidget::aboutToReload, &dialog, &ContentsDialog::close);

			m_widget->showDialog(&dialog);

			shouldCancelRequest = !dialog.isAccepted();
			shouldWarnNextTime = !dialog.getCheckBoxState();
		}
		else
		{
			QMessageBox dialog;
			dialog.setWindowTitle(tr("Question"));
			dialog.setText(tr("Are you sure that you want to send form data again?"));
			dialog.setInformativeText(tr("Do you want to resend data?"));
			dialog.setIcon(QMessageBox::Question);
			dialog.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
			dialog.setDefaultButton(QMessageBox::Cancel);
			dialog.setCheckBox(new QCheckBox(tr("Do not show this message again")));

			shouldCancelRequest = (dialog.exec() == QMessageBox::Cancel);
			shouldWarnNextTime = !dialog.checkBox()->isChecked();
		}

		SettingsManager::setOption(SettingsManager::Choices_WarnFormResendOption, shouldWarnNextTime);

		if (shouldCancelRequest)
		{
			return false;
		}
	}

	if (type != NavigationTypeReload)
	{
		m_previousNavigationType = type;

		if (type != NavigationTypeBackForward)
		{
			m_history.resize(history()->currentItemIndex());

			HistoryEntryInformation entry;
			entry.icon = icon();
			entry.timeVisited = QDateTime::currentDateTimeUtc();
			entry.position = scrollPosition().toPoint();
			entry.zoom = static_cast<int>(zoomFactor() * 100);
			entry.isValid = true;

			if (!profile()->isOffTheRecord())
			{
				entry.identifier = HistoryManager::addEntry(url, {}, {});
			}

			m_history.append(entry);
		}
	}

	scripts().clear();

	const QVector<UserScript*> userScripts(UserScript::getUserScriptsForUrl(url));

	for (int i = 0; i < userScripts.count(); ++i)
	{
		QWebEngineScript::InjectionPoint injectionPoint(QWebEngineScript::DocumentReady);

		switch (userScripts.at(i)->getInjectionTime())
		{
			case UserScript::DeferredTime:
				injectionPoint = QWebEngineScript::Deferred;

				break;
			case UserScript::DocumentCreationTime:
				injectionPoint = QWebEngineScript::DocumentCreation;

				break;
			default:
				break;
		}

		QWebEngineScript script;
		script.setSourceCode(userScripts.at(i)->getSource());
		script.setRunsOnSubFrames(userScripts.at(i)->shouldRunOnSubFrames());
		script.setInjectionPoint(injectionPoint);

		scripts().insert(script);
	}

	emit aboutToNavigate(url, type);

	return true;
}

bool QtWebEnginePage::javaScriptConfirm(const QUrl &url, const QString &message)
{
	if (m_isIgnoringJavaScriptPopups)
	{
		return false;
	}

	if (!m_widget || !m_widget->parentWidget())
	{
		return QWebEnginePage::javaScriptConfirm(url, message);
	}

	emit m_widget->needsAttention();

	ContentsDialog dialog(ThemesManager::createIcon(QLatin1String("dialog-information")), tr("JavaScript"), message, {}, (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), nullptr, m_widget);
	dialog.setCheckBox(tr("Disable JavaScript popups"), false);

	connect(m_widget, &QtWebEngineWebWidget::aboutToReload, &dialog, &ContentsDialog::close);

	m_widget->showDialog(&dialog);

	if (dialog.getCheckBoxState())
	{
		m_isIgnoringJavaScriptPopups = true;
	}

	return dialog.isAccepted();
}

bool QtWebEnginePage::javaScriptPrompt(const QUrl &url, const QString &message, const QString &defaultValue, QString *result)
{
	if (m_isIgnoringJavaScriptPopups)
	{
		return false;
	}

	if (!m_widget || !m_widget->parentWidget())
	{
		return QWebEnginePage::javaScriptPrompt(url, message, defaultValue, result);
	}

	emit m_widget->needsAttention();

	QWidget *widget(new QWidget(m_widget));
	LineEditWidget *lineEdit(new LineEditWidget(defaultValue, widget));
	QLabel *label(new QLabel(message, widget));
	label->setBuddy(lineEdit);
	label->setTextFormat(Qt::PlainText);

	QVBoxLayout *layout(new QVBoxLayout(widget));
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(label);
	layout->addWidget(lineEdit);

	ContentsDialog dialog(ThemesManager::createIcon(QLatin1String("dialog-information")), tr("JavaScript"), {}, {}, (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), widget, m_widget);
	dialog.setCheckBox(tr("Disable JavaScript popups"), false);

	connect(m_widget, &QtWebEngineWebWidget::aboutToReload, &dialog, &ContentsDialog::close);

	m_widget->showDialog(&dialog);

	if (dialog.isAccepted())
	{
		*result = lineEdit->text();
	}

	if (dialog.getCheckBoxState())
	{
		m_isIgnoringJavaScriptPopups = true;
	}

	return dialog.isAccepted();
}

bool QtWebEnginePage::isPopup() const
{
	return m_isPopup;
}

bool QtWebEnginePage::isViewingMedia() const
{
	return m_isViewingMedia;
}

}
