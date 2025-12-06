/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

QtWebEnginePage::QtWebEnginePage(bool isPrivate, QtWebEngineWebWidget *parent) : QWebEnginePage((isPrivate ? new QWebEngineProfile(parent) : qobject_cast<QtWebEngineWebBackend*>(parent->getBackend())->getDefaultProfile()), parent),
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
					runJavaScript(QString::fromLatin1(file.readAll()).arg(createJavaScriptList(cosmeticFilters.exceptions), createJavaScriptList(cosmeticFilters.rules)));

					file.close();
				}
			}

			const QStringList blockedRequests(m_widget->getBlockedElements());

			if (!blockedRequests.isEmpty())
			{
				QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/hideBlockedRequests.js"));

				if (file.open(QIODevice::ReadOnly))
				{
					runJavaScript(QString::fromLatin1(file.readAll()).arg(createJavaScriptList(blockedRequests)));

					file.close();
				}
			}
		}

		QString string(url().toString());
		string.truncate(1000);

		const QRegularExpressionMatch match(QRegularExpression(QLatin1String(R"(>(<img style="user-select: none;(?: cursor: zoom-in;)?"|<video controls="" autoplay="" name="media"><source) src=")") + QRegularExpression::escape(string)).match(result));
		const bool isViewingMedia(match.hasMatch());

		if (isViewingMedia && match.captured().startsWith(QLatin1String("><img")))
		{
			settings()->setAttribute(QWebEngineSettings::AutoLoadImages, true);
			settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);

			QFile file(SessionsManager::getReadableDataPath(QLatin1String("imageViwer.js")));

			if (file.open(QIODevice::ReadOnly))
			{
				runJavaScript(QString::fromLatin1(file.readAll()));

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

void QtWebEnginePage::setHistory(const Session::Window::History &history)
{
	m_history.clear();

	if (history.entries.isEmpty())
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
		const Session::Window::History::Entry entry(history.entries.at(i));

		stream << QUrl(entry.url) << entry.title << QByteArray() << static_cast<qint32>(0) << false << QUrl() << static_cast<qint32>(0) << QUrl(entry.url) << false << QDateTime::currentSecsSinceEpoch() << static_cast<int>(200);

		HistoryEntryInformation entryInformation;
		entryInformation.timeVisited = entry.time;
		entryInformation.position = entry.position;
		entryInformation.zoom = entry.zoom;
		entryInformation.isValid = true;

		m_history.append(entryInformation);
	}

	stream.device()->reset();
	stream >> *(this->history());

	if (this->history()->count() == history.entries.count() && !Utils::isUrlEmpty(this->history()->currentItem().url()))
	{
		this->history()->goToItem(this->history()->itemAt(history.index));
	}
	else
	{
		setUrl(history.entries.value(history.index).url);
	}
}

QtWebEngineWebWidget* QtWebEnginePage::getWebWidget() const
{
	return m_widget;
}

QWebEnginePage* QtWebEnginePage::createWindow(WebWindowType type)
{
	if (type == WebDialog)
	{
		return QWebEnginePage::createWindow(type);
	}

	const QString popupsPolicy((m_widget ? m_widget->getOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption) : SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption)).toString());

	if (!m_widget || m_widget->getLastUrlClickTime().isNull() || m_widget->getLastUrlClickTime().secsTo(QDateTime::currentDateTimeUtc()) > 1)
	{
		if (popupsPolicy == QLatin1String("blockAll"))
		{
			return nullptr;
		}

		if (popupsPolicy == QLatin1String("ask") || !m_widget->getOption(SettingsManager::ContentBlocking_ProfilesOption, m_widget->getUrl()).isNull())
		{
			QtWebEnginePage *page(new QtWebEnginePage(false, nullptr));
			page->markAsPopup();

			connect(page, &QtWebEnginePage::aboutToNavigate, this, [=](const QUrl &url)
			{
				m_popups.removeAll(page);

				page->deleteLater();

				const QVector<int> profiles(ContentFiltersManager::getProfileIdentifiers(m_widget->getOption(SettingsManager::ContentBlocking_ProfilesOption, m_widget->getUrl()).toStringList()));

				if (!profiles.isEmpty())
				{
					const ContentFiltersManager::CheckResult result(ContentFiltersManager::checkUrl(profiles, m_widget->getUrl(), url, NetworkManager::PopupType));

					if (result.isBlocked)
					{
						Console::addMessage(QCoreApplication::translate("main", "Request blocked by rule from profile %1:\n%2").arg(ContentFiltersManager::getProfile(result.profile)->getTitle(), result.rule), Console::NetworkCategory, Console::LogLevel, url.url(), -1, m_widget->getWindowIdentifier());

						return;
					}
				}

				const QString popupsPolicy(SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption, Utils::extractHost(m_widget->getRequestedUrl())).toString());

				if (popupsPolicy == QLatin1String("ask"))
				{
					emit requestedPopupWindow(m_widget->getUrl(), url);
				}
				else
				{
					SessionsManager::OpenHints hints(SessionsManager::NewTabOpen);

					if (popupsPolicy == QLatin1String("openAllInBackground"))
					{
						hints |= SessionsManager::BackgroundOpen;
					}

					QtWebEngineWebWidget *widget(createWidget(hints));
					widget->setUrl(url);
				}
			});

			return page;
		}
	}

	return createWidget(SessionsManager::calculateOpenHints((popupsPolicy == QLatin1String("openAllInBackground")) ? (SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen) : SessionsManager::NewTabOpen))->getPage();
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

QString QtWebEnginePage::createJavaScriptList(const QStringList &rules) const
{
	if (rules.isEmpty())
	{
		return {};
	}

	QStringList parsedRules(rules);

	for (int i = 0; i < rules.count(); ++i)
	{
		parsedRules[i].replace(QLatin1Char('\''), QLatin1String("\\'"));
	}

	return QLatin1Char('\'') + parsedRules.join(QLatin1String("','")) + QLatin1Char('\'');
}

QString QtWebEnginePage::createScriptSource(const QString &path, const QStringList &parameters) const
{
	QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/") + path + QLatin1String(".js"));

	if (!file.open(QIODevice::ReadOnly))
	{
		return {};
	}

	QString script(QString::fromLatin1(file.readAll()));

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

	runJavaScript(script, QWebEngineScript::UserWorld, [&](const QVariant &runResult)
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

WebWidget::SslInformation QtWebEnginePage::getSslInformation() const
{
	return m_sslInformation;
}

Session::Window::History QtWebEnginePage::getHistory() const
{
	QWebEngineHistory *pageHistory(history());
	const int historyCount(pageHistory->count());
	Session::Window::History history;
	history.entries.reserve(historyCount);
	history.index = pageHistory->currentItemIndex();

	const QString url(requestedUrl().toString());

	for (int i = 0; i < historyCount; ++i)
	{
		const QWebEngineHistoryItem item(pageHistory->itemAt(i));
		const HistoryEntryInformation entryInformation(m_history.value(i));
		Session::Window::History::Entry entry;
		entry.url = ((item.url().toString() == QLatin1String("about:blank")) ? item.originalUrl() : item.url()).toString();
		entry.title = item.title();

		if (entryInformation.isValid)
		{
			entry.icon = entryInformation.icon;
			entry.position = entryInformation.position;
			entry.time = entryInformation.timeVisited;
			entry.zoom = entryInformation.zoom;
		}

		history.entries.append(entry);
	}

	if (m_widget && m_widget->getLoadingState() == WebWidget::OngoingLoadingState && url != pageHistory->itemAt(pageHistory->currentItemIndex()).url().toString())
	{
		Session::Window::History::Entry entry;
		entry.url = url;
		entry.title = m_widget->getTitle();
		entry.icon = icon();
		entry.time = QDateTime::currentDateTimeUtc();
		entry.position = scrollPosition().toPoint();
		entry.zoom = static_cast<int>(zoomFactor() * 100);

		history.entries.append(entry);
		history.index = historyCount;
	}

	return history;
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

	if (m_widget && requestedUrl().isEmpty())
	{
		m_widget->setRequestedUrl(url, false, true);
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

	m_sslInformation = {};

	if (type != NavigationTypeReload)
	{
		m_previousNavigationType = type;

		if (type != NavigationTypeBackForward)
		{
			m_history.resize(qMax(0, history()->currentItemIndex()));

			HistoryEntryInformation entry;
			entry.icon = icon();
			entry.timeVisited = QDateTime::currentDateTimeUtc();
			entry.position = scrollPosition().toPoint();
			entry.zoom = static_cast<int>(zoomFactor() * 100);
			entry.isValid = true;

			if (!profile()->isOffTheRecord())
			{
				entry.identifier = HistoryManager::addEntry(url, {}, {}, (m_widget ? m_widget->isTypedIn() : false));
			}

			m_history.append(entry);
		}
	}

	scripts().clear();

	const QVector<UserScript*> userScripts(UserScript::getUserScriptsForUrl(url));

	for (int i = 0; i < userScripts.count(); ++i)
	{
		QWebEngineScript script;
		script.setSourceCode(userScripts.at(i)->getSource());
		script.setRunsOnSubFrames(userScripts.at(i)->shouldRunOnSubFrames());

		switch (userScripts.at(i)->getInjectionTime())
		{
			case UserScript::DeferredTime:
				script.setInjectionPoint(QWebEngineScript::Deferred);

				break;
			case UserScript::DocumentCreationTime:
				script.setInjectionPoint(QWebEngineScript::DocumentCreation);

				break;
			default:
				script.setInjectionPoint(QWebEngineScript::DocumentReady);

				break;
		}

		scripts().insert(script);
	}

	emit aboutToNavigate(url, type);

	return true;
}

bool QtWebEnginePage::certificateError(const QWebEngineCertificateError &error)
{
	if (!m_widget || error.certificateChain().isEmpty())
	{
		return false;
	}

	const QList<QSslError> errors(QSslCertificate::verify(error.certificateChain(), error.url().host()));

	m_sslInformation.errors.reserve(m_sslInformation.errors.count() + errors.count());

	for (int i = 0; i < errors.count(); ++i)
	{
		m_sslInformation.errors.append({errors.at(i), error.url()});
	}

	const QString firstPartyUrl(m_widget->getUrl().toString());
	const QString thirdPartyUrl(error.url().toString());

	if (m_widget->getOption(SettingsManager::Security_IgnoreSslErrorsOption, m_widget->getUrl()).toStringList().contains(QString::fromLatin1(error.certificateChain().constFirst().digest().toBase64())))
	{
		Console::addMessage(QStringLiteral("[accepted] The page at %1 was allowed to display insecure content from %2").arg(firstPartyUrl, thirdPartyUrl), Console::SecurityCategory, Console::WarningLevel, thirdPartyUrl, -1, m_widget->getWindowIdentifier());

		return true;
	}

	Console::addMessage(QStringLiteral("[blocked] The page at %1 was not allowed to display insecure content from %2").arg(firstPartyUrl, thirdPartyUrl), Console::SecurityCategory, Console::WarningLevel, thirdPartyUrl, -1, m_widget->getWindowIdentifier());

	return false;
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
