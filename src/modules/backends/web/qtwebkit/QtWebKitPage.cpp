/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "QtWebKitPage.h"
#include "QtWebKitNetworkManager.h"
#include "QtWebKitPluginFactory.h"
#include "QtWebKitWebWidget.h"
#include "../../../../core/ActionsManager.h"
#include "../../../../core/Console.h"
#include "../../../../core/ContentFiltersManager.h"
#include "../../../../core/HandlersManager.h"
#include "../../../../core/SettingsManager.h"
#include "../../../../core/ThemesManager.h"
#include "../../../../core/UserScript.h"
#include "../../../../core/Utils.h"
#include "../../../../ui/ContentsDialog.h"
#include "../../../../ui/LineEditWidget.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtGui/QGuiApplication>
#include <QtGui/QWheelEvent>
#include <QtWebKit/QWebHistory>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWidgets/QMessageBox>

namespace Otter
{

QtWebKitFrame::QtWebKitFrame(QWebFrame *frame, QtWebKitWebWidget *parent) : QObject(parent),
	m_frame(frame),
	m_widget(parent),
	m_isDisplayingErrorPage(false)
{
	connect(frame, &QWebFrame::destroyed, this, &QtWebKitFrame::deleteLater);
	connect(frame, &QWebFrame::loadFinished, this, &QtWebKitFrame::handleLoadFinished);
}

void QtWebKitFrame::runUserScripts(const QUrl &url) const
{
	const QVector<UserScript*> scripts(UserScript::getUserScriptsForUrl(url, UserScript::AnyTime, (m_frame->parentFrame() != nullptr)));

	for (int i = 0; i < scripts.count(); ++i)
	{
		m_frame->documentElement().evaluateJavaScript(scripts.at(i)->getSource());
	}
}

void QtWebKitFrame::applyContentBlockingRules(const QStringList &rules, bool isHiding)
{
	const QString value(isHiding ? QLatin1String("none !important") : QString());
	const QWebElementCollection elements(m_frame->documentElement().findAll(rules.join(QLatin1Char(','))));

	for (int i = 0; i < elements.count(); ++i)
	{
		QWebElement element(elements.at(i));

		if (!element.isNull())
		{
			element.setStyleProperty(QLatin1String("display"), value);
		}
	}
}

void QtWebKitFrame::handleIsDisplayingErrorPageChanged(QWebFrame *frame, bool isDisplayingErrorPage)
{
	if (frame == m_frame)
	{
		m_isDisplayingErrorPage = isDisplayingErrorPage;
	}
}

void QtWebKitFrame::handleLoadFinished()
{
	if (!m_widget)
	{
		return;
	}

	if (m_isDisplayingErrorPage)
	{
		const QVector<WebWidget::SslInformation::SslError> sslErrors(m_widget->getSslInformation().errors);
		QFile file(QLatin1String(":/modules/backends/web/qtwebkit/resources/errorPage.js"));

		if (file.open(QIODevice::ReadOnly))
		{
			m_frame->documentElement().evaluateJavaScript(QString(file.readAll()).arg(m_widget->getMessageToken(), (sslErrors.isEmpty() ? QByteArray() : sslErrors.first().error.certificate().digest().toBase64()), ((m_frame->page()->history()->currentItemIndex() > 0) ? QLatin1String("true") : QLatin1String("false"))));

			file.close();
		}
	}

	runUserScripts(m_widget->getUrl());

	if (!m_widget->isPrivate() && m_widget->getOption(SettingsManager::Browser_RememberPasswordsOption).toBool())
	{
		QFile file(QLatin1String(":/modules/backends/web/qtwebkit/resources/formExtractor.js"));

		if (file.open(QIODevice::ReadOnly))
		{
			m_frame->documentElement().evaluateJavaScript(QString(file.readAll()).arg(m_widget->getMessageToken()));

			file.close();
		}
	}

	if (!m_widget->getOption(SettingsManager::ContentBlocking_EnableContentBlockingOption).toBool())
	{
		return;
	}

	const ContentFiltersManager::CosmeticFiltersResult cosmeticFilters(ContentFiltersManager::getCosmeticFilters(ContentFiltersManager::getProfileIdentifiers(m_widget->getOption(SettingsManager::ContentBlocking_ProfilesOption).toStringList()), m_widget->getUrl()));

	applyContentBlockingRules(cosmeticFilters.rules, true);
	applyContentBlockingRules(cosmeticFilters.exceptions, false);

	const QStringList blockedRequests(m_widget->getBlockedElements());

	if (blockedRequests.count() > 0)
	{
		const QWebElementCollection elements(m_frame->documentElement().findAll(QLatin1String("[src]")));

		for (int i = 0; i < elements.count(); ++i)
		{
			QWebElement element(elements.at(i));
			const QUrl url(element.attribute(QLatin1String("src")));

			for (int j = 0; j < blockedRequests.count(); ++j)
			{
				if (url.matches(QUrl(blockedRequests.at(j)), QUrl::None) || blockedRequests.at(j).endsWith(url.url()))
				{
					element.setStyleProperty(QLatin1String("display"), QLatin1String("none !important"));

					break;
				}
			}
		}
	}
}

bool QtWebKitFrame::isDisplayingErrorPage() const
{
	return m_isDisplayingErrorPage;
}

QtWebKitPage::QtWebKitPage(QtWebKitNetworkManager *networkManager, QtWebKitWebWidget *parent) : QWebPage(parent),
	m_widget(parent),
	m_networkManager(networkManager),
	m_isIgnoringJavaScriptPopups(false),
	m_isPopup(false),
	m_isViewingMedia(false)
{
	setNetworkAccessManager(m_networkManager);
	setPluginFactory(new QtWebKitPluginFactory(parent));
	setForwardUnsupportedContent(true);
	handleFrameCreation(mainFrame());

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &QtWebKitPage::handleOptionChanged);
	connect(this, &QtWebKitPage::frameCreated, this, &QtWebKitPage::handleFrameCreation);
	connect(this, &QtWebKitPage::consoleMessageReceived, this, &QtWebKitPage::handleConsoleMessage);
	connect(mainFrame(), &QWebFrame::loadStarted, this, [&]()
	{
		updateStyleSheets();
	});
	connect(mainFrame(), &QWebFrame::loadFinished, this, [&]()
	{
		m_isIgnoringJavaScriptPopups = false;

		updateStyleSheets();
	});
}

QtWebKitPage::QtWebKitPage() : QWebPage(),
	m_widget(nullptr),
	m_networkManager(nullptr),
	m_mainFrame(nullptr),
	m_isIgnoringJavaScriptPopups(false),
	m_isPopup(false),
	m_isViewingMedia(false)
{
}

QtWebKitPage::QtWebKitPage(const QUrl &url) : QWebPage(),
	m_widget(nullptr),
	m_networkManager(new QtWebKitNetworkManager(true, nullptr, nullptr)),
	m_mainFrame(nullptr),
	m_isIgnoringJavaScriptPopups(true),
	m_isPopup(false),
	m_isViewingMedia(false)
{
	setNetworkAccessManager(m_networkManager);

	m_networkManager->setParent(this);
	m_networkManager->updateOptions(url);

	settings()->setAttribute(QWebSettings::JavaEnabled, false);
	settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
	settings()->setAttribute(QWebSettings::PluginsEnabled, false);
	mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
	mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
	mainFrame()->setUrl(url);
}

QtWebKitPage::~QtWebKitPage()
{
	qDeleteAll(m_popups);

	m_popups.clear();
}

void QtWebKitPage::validatePopup(const QUrl &url)
{
	QtWebKitPage *page(qobject_cast<QtWebKitPage*>(sender()));

	if (page)
	{
		m_popups.removeAll(page);

		page->deleteLater();
	}

	const QVector<int> profiles(ContentFiltersManager::getProfileIdentifiers(getOption(SettingsManager::ContentBlocking_ProfilesOption).toStringList()));

	if (!profiles.isEmpty())
	{
		const ContentFiltersManager::CheckResult result(ContentFiltersManager::checkUrl(profiles, mainFrame()->url(), url, NetworkManager::PopupType));

		if (result.isBlocked)
		{
			Console::addMessage(QCoreApplication::translate("main", "Request blocked by rule from profile %1:\n%2").arg(ContentFiltersManager::getProfile(result.profile)->getTitle()).arg(result.rule), Console::NetworkCategory, Console::LogLevel, url.url(), -1, (m_widget ? m_widget->getWindowIdentifier() : 0));

			return;
		}
	}

	const QString popupsPolicy(getOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption).toString());

	if (popupsPolicy == QLatin1String("ask"))
	{
		emit requestedPopupWindow(mainFrame()->url(), url);
	}
	else
	{
		QtWebKitWebWidget *widget(createWidget((popupsPolicy == QLatin1String("openAllInBackground")) ? (SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen) : SessionsManager::NewTabOpen));
		widget->setUrl(url);
	}
}

void QtWebKitPage::markAsDisplayingErrorPage()
{
	emit isDisplayingErrorPageChanged(mainFrame(), false);
}

void QtWebKitPage::markAsPopup()
{
	m_isPopup = true;
}

void QtWebKitPage::handleOptionChanged(int identifier)
{
	if (SettingsManager::getOptionName(identifier).startsWith(QLatin1String("Content/")) || identifier == SettingsManager::Interface_ShowScrollBarsOption)
	{
		updateStyleSheets();
	}
}

void QtWebKitPage::handleFrameCreation(QWebFrame *frame)
{
	QtWebKitFrame *frameWrapper(new QtWebKitFrame(frame, m_widget));

	if (frame == mainFrame())
	{
		m_mainFrame = frameWrapper;
	}

	connect(this, &QtWebKitPage::isDisplayingErrorPageChanged, frameWrapper, &QtWebKitFrame::handleIsDisplayingErrorPageChanged);
}

void QtWebKitPage::handleConsoleMessage(MessageSource category, MessageLevel level, const QString &message, int line, const QString &source)
{
	Console::MessageLevel mappedLevel(Console::UnknownLevel);

	switch (level)
	{
		case LogMessageLevel:
			mappedLevel = Console::LogLevel;

			break;
		case WarningMessageLevel:
			mappedLevel = Console::WarningLevel;

			break;
		case ErrorMessageLevel:
			mappedLevel = Console::ErrorLevel;

			break;
		default:
			mappedLevel = Console::DebugLevel;

			break;
	}

	Console::MessageCategory mappedCategory(Console::OtherCategory);

	switch (category)
	{
		case NetworkMessageSource:
			mappedCategory = Console::NetworkCategory;

			break;
		case ContentBlockerMessageSource:
			mappedCategory = Console::ContentFiltersCategory;

			break;
		case SecurityMessageSource:
			mappedCategory = Console::SecurityCategory;

			break;
		case CSSMessageSource:
			mappedCategory = Console::CssCategory;

			break;
		case JSMessageSource:
			mappedCategory = Console::JavaScriptCategory;

			break;
		default:
			mappedCategory = Console::OtherCategory;

			break;
	}

	Console::addMessage(message, mappedCategory, mappedLevel, source, line, (m_widget ? m_widget->getWindowIdentifier() : 0));
}

void QtWebKitPage::updateStyleSheets(const QUrl &url)
{
	QString styleSheet((QStringLiteral("html {color: %1;} a {color: %2;} a:visited {color: %3;}")).arg(getOption(SettingsManager::Content_TextColorOption).toString()).arg(getOption(SettingsManager::Content_LinkColorOption).toString()).arg(getOption(SettingsManager::Content_VisitedLinkColorOption).toString()).toUtf8());
	const QWebElement mediaElement(mainFrame()->findFirstElement(QLatin1String("img, audio source, video source")));
	const bool isViewingMedia(!mediaElement.isNull() && QUrl(mediaElement.attribute(QLatin1String("src"))) == (url.isEmpty() ? mainFrame()->url() : url));

	if (isViewingMedia && mediaElement.tagName().toLower() == QLatin1String("img"))
	{
		styleSheet += QLatin1String("html {width:100%;height:100%;} body {display:-webkit-flex;margin:0;padding:0;-webkit-align-items:center;text-align:center;} img {max-width:100%;max-height:100%;margin:auto;-webkit-user-select:none;} .zoomedIn {display:table;} .zoomedIn body {display:table-cell;vertical-align:middle;} .zoomedIn img {max-width:none;max-height:none;cursor:-webkit-zoom-out;} .zoomedIn .drag {cursor:move;} .zoomedOut img {cursor:-webkit-zoom-in;}");

		settings()->setAttribute(QWebSettings::AutoLoadImages, true);
		settings()->setAttribute(QWebSettings::JavascriptEnabled, true);

		runScript(QLatin1String("imageViewer"));
	}

	if (isViewingMedia != m_isViewingMedia)
	{
		m_isViewingMedia = isViewingMedia;

		if (m_widget)
		{
			emit m_widget->categorizedActionsStateChanged({ActionsManager::ActionDefinition::NavigationCategory});
		}

		emit viewingMediaChanged(m_isViewingMedia);
	}

	if (!getOption(SettingsManager::Interface_ShowScrollBarsOption).toBool())
	{
		styleSheet.append(QLatin1String("body::-webkit-scrollbar {display:none;}"));
	}

	const QString userSyleSheetPath(getOption(SettingsManager::Content_UserStyleSheetOption).toString());

	if (!userSyleSheetPath.isEmpty())
	{
		QFile file(Utils::normalizePath(userSyleSheetPath));

		if (file.open(QIODevice::ReadOnly))
		{
			styleSheet.append(file.readAll());

			file.close();
		}
	}

	settings()->setUserStyleSheetUrl(QUrl(QLatin1String("data:text/css;charset=utf-8;base64,") + styleSheet.toUtf8().toBase64()));
}

void QtWebKitPage::javaScriptAlert(QWebFrame *frame, const QString &message)
{
	if (m_isIgnoringJavaScriptPopups)
	{
		return;
	}

	if (!m_widget || !m_widget->parentWidget())
	{
		QWebPage::javaScriptAlert(frame, message);

		return;
	}

	emit m_widget->needsAttention();

	ContentsDialog dialog(ThemesManager::createIcon(QLatin1String("dialog-information")), tr("JavaScript"), message, {}, QDialogButtonBox::Ok, nullptr, m_widget);
	dialog.setCheckBox(tr("Disable JavaScript popups"), false);

	connect(m_widget, &QtWebKitWebWidget::aboutToReload, &dialog, &ContentsDialog::close);

	m_widget->showDialog(&dialog);

	if (dialog.getCheckBoxState())
	{
		m_isIgnoringJavaScriptPopups = true;
	}
}

void QtWebKitPage::triggerAction(WebAction action, bool isChecked)
{
	if (action == InspectElement && m_widget && !m_widget->isInspecting())
	{
		settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);

		m_widget->triggerAction(ActionsManager::InspectPageAction, {{QLatin1String("isChecked"), true}});
	}

	QWebPage::triggerAction(action, isChecked);
}

QWebPage* QtWebKitPage::createWindow(WebWindowType type)
{
	if (type != WebModalDialog)
	{
		const QString popupsPolicy(getOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption).toString());

		if (!m_widget || currentFrame()->hitTestContent(m_widget->getClickPosition()).linkUrl().isEmpty())
		{
			if (popupsPolicy == QLatin1String("blockAll"))
			{
				return nullptr;
			}

			if (popupsPolicy == QLatin1String("ask") || !getOption(SettingsManager::ContentBlocking_ProfilesOption).isNull())
			{
				QtWebKitPage *page(new QtWebKitPage());
				page->markAsPopup();

				connect(page, &QtWebKitPage::aboutToNavigate, this, &QtWebKitPage::validatePopup);

				return page;
			}
		}

		return createWidget(SessionsManager::calculateOpenHints((popupsPolicy == QLatin1String("openAllInBackground")) ? (SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen) : SessionsManager::NewTabOpen))->getPage();
	}

	return QWebPage::createWindow(type);
}

QtWebKitFrame* QtWebKitPage::getMainFrame() const
{
	return m_mainFrame;
}

QtWebKitWebWidget* QtWebKitPage::createWidget(SessionsManager::OpenHints hints)
{
	QtWebKitWebWidget *widget(nullptr);

	if (m_widget)
	{
		widget = qobject_cast<QtWebKitWebWidget*>(m_widget->clone(false, m_widget->isPrivate(), getOption(SettingsManager::Sessions_OptionsExludedFromInheritingOption).toStringList()));
	}
	else if (settings()->testAttribute(QWebSettings::PrivateBrowsingEnabled))
	{
		widget = new QtWebKitWebWidget({{QLatin1String("hints"), SessionsManager::PrivateOpen}}, nullptr, nullptr);
	}
	else
	{
		widget = new QtWebKitWebWidget({}, nullptr, nullptr);
	}

	widget->handleLoadStarted();

	emit requestedNewWindow(widget, hints, {});

	return widget;
}

QString QtWebKitPage::chooseFile(QWebFrame *frame, const QString &suggestedFile)
{
	Q_UNUSED(frame)

	return Utils::getOpenPaths({suggestedFile}).value(0);
}

QString QtWebKitPage::userAgentForUrl(const QUrl &url) const
{
	if (m_networkManager)
	{
		return m_networkManager->getUserAgent();
	}

	return QWebPage::userAgentForUrl(url);
}

QString QtWebKitPage::getDefaultUserAgent() const
{
	return QWebPage::userAgentForUrl({});
}

QVariant QtWebKitPage::runScript(const QString &path, QWebElement element)
{
	if (element.isNull())
	{
		element = mainFrame()->documentElement();
	}

	QFile file(QLatin1String(":/modules/backends/web/qtwebkit/resources/") + path + QLatin1String(".js"));

	if (file.open(QIODevice::ReadOnly))
	{
		const QVariant result(element.evaluateJavaScript(file.readAll()));

		file.close();

		return result;
	}

	return {};
}

QVariant QtWebKitPage::getOption(int identifier) const
{
	if (m_widget)
	{
		return m_widget->getOption(identifier);
	}

	const QUrl url(mainFrame()->url());

	return SettingsManager::getOption(identifier, Utils::extractHost(url.isEmpty() ? mainFrame()->requestedUrl() : url));
}

bool QtWebKitPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type)
{
	if (m_isPopup)
	{
		emit aboutToNavigate(request.url(), frame, type);

		return false;
	}

	if (HandlersManager::handleUrl(request.url()))
	{
		return false;
	}

	if (frame && request.url().scheme() == QLatin1String("javascript"))
	{
		frame->documentElement().evaluateJavaScript(request.url().path());

		return false;
	}

	const bool isAnchorNavigation(frame && (type == NavigationTypeLinkClicked || type == NavigationTypeOther) && frame->url().matches(request.url(), QUrl::RemoveFragment));

	if (mainFrame() == frame)
	{
		if (m_widget && !isAnchorNavigation)
		{
			m_widget->handleNavigationRequest(request.url(), type);
		}

		m_networkManager->setMainRequest(request.url());
	}

	if (type == NavigationTypeFormSubmitted && QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier))
	{
		m_networkManager->setFormRequest(request.url());
	}

	if (type == NavigationTypeFormResubmitted && SettingsManager::getOption(SettingsManager::Choices_WarnFormResendOption).toBool())
	{
		bool shouldCancelRequest(false);
		bool shouldWarnNextTime(true);

		if (m_widget)
		{
			ContentsDialog dialog(ThemesManager::createIcon(QLatin1String("dialog-warning")), tr("Question"), tr("Are you sure that you want to send form data again?"), tr("Do you want to resend data?"), (QDialogButtonBox::Yes | QDialogButtonBox::Cancel), nullptr, m_widget);
			dialog.setCheckBox(tr("Do not show this message again"), false);

			connect(m_widget, &QtWebKitWebWidget::aboutToReload, &dialog, &ContentsDialog::close);

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

	if (type != NavigationTypeOther)
	{
		emit isDisplayingErrorPageChanged(frame, false);
	}

	if (!isAnchorNavigation)
	{
		emit aboutToNavigate(request.url(), frame, type);
	}

	return true;
}

bool QtWebKitPage::javaScriptConfirm(QWebFrame *frame, const QString &message)
{
	if (m_isIgnoringJavaScriptPopups)
	{
		return false;
	}

	if (!m_widget || !m_widget->parentWidget())
	{
		return QWebPage::javaScriptConfirm(frame, message);
	}

	emit m_widget->needsAttention();

	ContentsDialog dialog(ThemesManager::createIcon(QLatin1String("dialog-information")), tr("JavaScript"), message, {}, (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), nullptr, m_widget);
	dialog.setCheckBox(tr("Disable JavaScript popups"), false);

	connect(m_widget, &QtWebKitWebWidget::aboutToReload, &dialog, &ContentsDialog::close);

	m_widget->showDialog(&dialog);

	if (dialog.getCheckBoxState())
	{
		m_isIgnoringJavaScriptPopups = true;
	}

	return dialog.isAccepted();
}

bool QtWebKitPage::javaScriptPrompt(QWebFrame *frame, const QString &message, const QString &defaultValue, QString *result)
{
	if (m_isIgnoringJavaScriptPopups)
	{
		return false;
	}

	if (!m_widget || !m_widget->parentWidget())
	{
		return QWebPage::javaScriptPrompt(frame, message, defaultValue, result);
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

	connect(m_widget, &QtWebKitWebWidget::aboutToReload, &dialog, &ContentsDialog::close);

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

bool QtWebKitPage::event(QEvent *event)
{
	if (event->type() == QEvent::Wheel && static_cast<QWheelEvent*>(event)->buttons() == Qt::RightButton)
	{
		return false;
	}

	return QWebPage::event(event);
}

bool QtWebKitPage::extension(Extension extension, const ExtensionOption *option, ExtensionReturn *output)
{
	if (!m_widget)
	{
		return false;
	}

	if (extension == ChooseMultipleFilesExtension)
	{
		const ChooseMultipleFilesExtensionOption *filesOption(static_cast<const ChooseMultipleFilesExtensionOption*>(option));
		ChooseMultipleFilesExtensionReturn *filesOutput(static_cast<ChooseMultipleFilesExtensionReturn*>(output));

		filesOutput->fileNames = Utils::getOpenPaths(filesOption->suggestedFileNames, {}, true);

		return true;
	}

	if (extension == ErrorPageExtension)
	{
		const ErrorPageExtensionOption *errorOption(static_cast<const ErrorPageExtensionOption*>(option));
		ErrorPageExtensionReturn *errorOutput(static_cast<ErrorPageExtensionReturn*>(output));

		if (!errorOption || !errorOutput)
		{
			return false;
		}

		const QUrl url((errorOption->url.isEmpty() && m_widget) ? m_widget->getRequestedUrl() : errorOption->url);
		QString domain;

		if (errorOption->domain == QtNetwork)
		{
			domain = QLatin1String("QtNetwork");
		}
		else if (errorOption->domain == WebKit)
		{
			domain = QLatin1String("WebKit");
		}
		else
		{
			domain = QLatin1String("HTTP");
		}

		const QString logMessage(tr("%1 error #%2: %3").arg(domain).arg(errorOption->error).arg(errorOption->errorString));
		const quint64 windowIdentifier(m_widget ? m_widget->getWindowIdentifier() : 0);

		if (errorOption->domain == WebKit && (errorOption->error == 102 || errorOption->error == 203))
		{
			Console::addMessage(logMessage, Console::NetworkCategory, Console::ErrorLevel, url.toString(), -1, windowIdentifier);

			return false;
		}

		errorOutput->baseUrl = url;

		settings()->setAttribute(QWebSettings::JavascriptEnabled, true);

		emit isDisplayingErrorPageChanged(errorOption->frame, true);

		if (errorOption->domain == QtNetwork && url.isLocalFile() && QFileInfo(url.toLocalFile()).isDir())
		{
			Console::addMessage(logMessage, Console::NetworkCategory, Console::ErrorLevel, url.toString(), -1, windowIdentifier);

			return false;
		}

		ErrorPageInformation information;
		information.url = url;
		information.description = QStringList(errorOption->errorString);

		if ((errorOption->domain == QtNetwork && (errorOption->error == QNetworkReply::HostNotFoundError || errorOption->error == QNetworkReply::ContentNotFoundError)) || (errorOption->domain == Http && errorOption->error == 404))
		{
			if (errorOption->url.isLocalFile())
			{
				information.type = ErrorPageInformation::FileNotFoundError;
			}
			else
			{
				information.type = ErrorPageInformation::ServerNotFoundError;
			}
		}
		else if (errorOption->domain == QtNetwork && errorOption->error == QNetworkReply::ConnectionRefusedError)
		{
			information.type = ErrorPageInformation::ConnectionRefusedError;
		}
		else if (errorOption->domain == QtNetwork && errorOption->error == QNetworkReply::SslHandshakeFailedError)
		{
			information.description.clear();
			information.type = ErrorPageInformation::ConnectionInsecureError;

			const QVector<WebWidget::SslInformation::SslError> sslErrors(m_widget->getSslInformation().errors);

			for (int i = 0; i < sslErrors.count(); ++i)
			{
				information.description.append(sslErrors.at(i).error.errorString());
			}
		}
		else if (errorOption->domain == QtNetwork && errorOption->error == QNetworkReply::QNetworkReply::ProtocolUnknownError)
		{
			const QUrl normalizedUrl(Utils::normalizeUrl(url));
			const QVector<NetworkManager::ResourceInformation> blockeckedRequests(m_networkManager->getBlockedRequests());
			bool isBlockedContent(false);

			for (int i = 0; i < blockeckedRequests.count(); ++i)
			{
				if (blockeckedRequests.at(i).resourceType == NetworkManager::MainFrameType && Utils::normalizeUrl(blockeckedRequests.at(i).url) == normalizedUrl)
				{
					isBlockedContent = true;

					information.description.clear();

					if (blockeckedRequests.at(i).metaData.contains(NetworkManager::ContentBlockingRuleMetaData))
					{
						const ContentFiltersProfile *profile(ContentFiltersManager::getProfile(blockeckedRequests.at(i).metaData.value(NetworkManager::ContentBlockingProfileMetaData).toInt()));

						information.description.append(tr("Request blocked by rule from profile %1:<br>\n%2").arg(profile ? profile->getTitle() : tr("(Unknown)")).arg(QStringLiteral("<span style=\"font-family:monospace;\">%1</span>").arg(blockeckedRequests.at(i).metaData.value(NetworkManager::ContentBlockingRuleMetaData).toString())));
					}

					break;
				}
			}

			if (isBlockedContent)
			{
				information.type = ErrorPageInformation::BlockedContentError;
			}
			else
			{
				information.type = ErrorPageInformation::UnsupportedAddressTypeError;
			}
		}
		else if (errorOption->domain == WebKit)
		{
			information.title = tr("WebKit error %1").arg(errorOption->error);
		}
		else
		{
			information.title = tr("Network error %1").arg(errorOption->error);
		}

		switch (information.type)
		{
			case ErrorPageInformation::BlockedContentError:
				{
					ErrorPageInformation::PageAction goBackAction;
					goBackAction.name = QLatin1String("goBack");
					goBackAction.title = QCoreApplication::translate("utils", "Go Back");
					goBackAction.type = ErrorPageInformation::MainAction;

					ErrorPageInformation::PageAction addExceptionAction;
					addExceptionAction.name = QLatin1String("addContentBlockingException");
					addExceptionAction.title = QCoreApplication::translate("utils", "Load Blocked Page");
					addExceptionAction.type = ErrorPageInformation::AdvancedAction;

					information.actions = {goBackAction, addExceptionAction};
				}

				break;
			case ErrorPageInformation::ConnectionInsecureError:
				{
					ErrorPageInformation::PageAction goBackAction;
					goBackAction.name = QLatin1String("goBack");
					goBackAction.title = QCoreApplication::translate("utils", "Go Back");
					goBackAction.type = ErrorPageInformation::MainAction;

					ErrorPageInformation::PageAction addExceptionAction;
					addExceptionAction.name = QLatin1String("addSslErrorException");
					addExceptionAction.title = QCoreApplication::translate("utils", "Load Insecure Page");
					addExceptionAction.type = ErrorPageInformation::AdvancedAction;

					information.actions = {goBackAction, addExceptionAction};
				}

				break;
			default:
				{
					ErrorPageInformation::PageAction reloadAction;
					reloadAction.name = QLatin1String("reloadPage");
					reloadAction.title = QCoreApplication::translate("utils", "Try Again");
					reloadAction.type = ErrorPageInformation::MainAction;

					information.actions = {reloadAction};
				}

				break;
		}

		errorOutput->content = Utils::createErrorPage(information).toUtf8();

		if (information.type != ErrorPageInformation::BlockedContentError)
		{
			Console::addMessage(logMessage, Console::NetworkCategory, Console::ErrorLevel, url.toString(), -1, windowIdentifier);
		}

		return true;
	}

	return false;
}

bool QtWebKitPage::shouldInterruptJavaScript()
{
	if (m_widget)
	{
		ContentsDialog dialog(ThemesManager::createIcon(QLatin1String("dialog-warning")), tr("Question"), tr("The script on this page appears to have a problem."), tr("Do you want to stop the script?"), (QDialogButtonBox::Yes | QDialogButtonBox::No), nullptr, m_widget);

		connect(m_widget, &QtWebKitWebWidget::aboutToReload, &dialog, &ContentsDialog::close);

		m_widget->showDialog(&dialog);

		return dialog.isAccepted();
	}

	return QWebPage::shouldInterruptJavaScript();
}

bool QtWebKitPage::supportsExtension(QWebPage::Extension extension) const
{
	return (extension == ChooseMultipleFilesExtension || extension == ErrorPageExtension);
}

bool QtWebKitPage::isDisplayingErrorPage() const
{
	return m_mainFrame->isDisplayingErrorPage();
}

bool QtWebKitPage::isPopup() const
{
	return m_isPopup;
}

bool QtWebKitPage::isViewingMedia() const
{
	return m_isViewingMedia;
}

}
