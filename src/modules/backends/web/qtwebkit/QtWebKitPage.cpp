/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "QtWebKitPage.h"
#include "QtWebKitNetworkManager.h"
#include "QtWebKitWebWidget.h"
#include "../../../../core/Console.h"
#include "../../../../core/ContentBlockingManager.h"
#include "../../../../core/NetworkManagerFactory.h"
#include "../../../../core/SettingsManager.h"
#include "../../../../core/ThemesManager.h"
#include "../../../../core/Utils.h"
#include "../../../../ui/ContentsDialog.h"

#include <QtCore/QFile>
#include <QtGui/QDesktopServices>
#include <QtGui/QGuiApplication>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWebKit/QWebElement>
#include <QtWebKitWidgets/QWebFrame>

namespace Otter
{

QtWebKitPage::QtWebKitPage(QtWebKitNetworkManager *networkManager, QtWebKitWebWidget *parent) : QWebPage(parent),
	m_widget(parent),
	m_networkManager(networkManager),
	m_ignoreJavaScriptPopups(false),
	m_isPopup(false),
	m_isViewingMedia(false)
{
	setNetworkAccessManager(m_networkManager);
	setForwardUnsupportedContent(true);
	updateStyleSheets();
	optionChanged(SettingsManager::Interface_ShowScrollBarsOption);

	connect(this, SIGNAL(loadFinished(bool)), this, SLOT(pageLoadFinished()));
#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
	connect(this, SIGNAL(consoleMessageReceived(MessageSource,MessageLevel,QString,int,QString)), this, SLOT(handleConsoleMessage(MessageSource,MessageLevel,QString,int,QString)));
#endif
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(int,QVariant)), this, SLOT(optionChanged(int)));
}

QtWebKitPage::QtWebKitPage() : QWebPage(),
	m_widget(nullptr),
	m_networkManager(nullptr),
	m_ignoreJavaScriptPopups(false),
	m_isPopup(false),
	m_isViewingMedia(false)
{
}

QtWebKitPage::~QtWebKitPage()
{
	qDeleteAll(m_popups);

	m_popups.clear();
}

void QtWebKitPage::optionChanged(int identifier)
{
	if (SettingsManager::getOptionName(identifier).startsWith(QLatin1String("Content/")) || identifier == SettingsManager::Interface_ShowScrollBarsOption)
	{
		updateStyleSheets();
	}
}

void QtWebKitPage::pageLoadFinished()
{
	m_ignoreJavaScriptPopups = false;

	updateStyleSheets();

	if (!m_widget || !m_widget->getOption(SettingsManager::ContentBlocking_EnableContentBlockingOption, m_widget->getUrl()).toBool())
	{
		return;
	}

	const QVector<int> profiles(ContentBlockingManager::getProfileList(m_widget->getOption(SettingsManager::ContentBlocking_ProfilesOption, m_widget->getUrl()).toStringList()));

	applyContentBlockingRules(ContentBlockingManager::getStyleSheet(profiles), true);

	const QStringList domainList(ContentBlockingManager::createSubdomainList(m_widget->getUrl().host()));

	for (int i = 0; i < domainList.count(); ++i)
	{
		applyContentBlockingRules(ContentBlockingManager::getStyleSheetBlackList(domainList.at(i), profiles), true);
		applyContentBlockingRules(ContentBlockingManager::getStyleSheetWhiteList(domainList.at(i), profiles), false);
	}

	const QStringList blockedRequests(m_widget->getBlockedElements());

	if (blockedRequests.count() > 0)
	{
		const QWebElementCollection elements(mainFrame()->documentElement().findAll(QLatin1String("[src]")));

		for (int i = 0; i < blockedRequests.count(); ++i)
		{
			for (int j = 0; j < elements.count(); ++j)
			{
				QWebElement element(elements.at(j));

				if (element.attribute(QLatin1String("src")) == blockedRequests[i]) ///NOTE: Consider comparing them as URLs
				{
					element.setStyleProperty(QLatin1String("display"), QLatin1String("none !important"));

					break;
				}
			}
		}
	}
}

void QtWebKitPage::removePopup(const QUrl &url)
{
	QtWebKitPage *page(qobject_cast<QtWebKitPage*>(sender()));

	if (page)
	{
		m_popups.removeAll(page);

		page->deleteLater();
	}

	emit requestedPopupWindow(mainFrame()->url(), url);
}

void QtWebKitPage::markAsPopup()
{
	m_isPopup = true;
}

void QtWebKitPage::applyContentBlockingRules(const QStringList &rules, bool remove)
{
	const QWebElement document(mainFrame()->documentElement());
	const QString value(remove ? QLatin1String("none !important") : QString());

	for (int i = 0; i < rules.count(); ++i)
	{
		const QWebElementCollection elements(document.findAll(rules.at(i)));

		for (int j = 0; j < elements.count(); ++j)
		{
			QWebElement element(elements.at(j));

			if (!element.isNull())
			{
				element.setStyleProperty(QLatin1String("display"), value);
			}
		}
	}
}

#ifndef OTTER_ENABLE_QTWEBKIT_LEGACY
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
			mappedCategory = Console::ContentBlockingCategory;

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
#endif

void QtWebKitPage::updateStyleSheets(const QUrl &url)
{
	const QUrl currentUrl(url.isEmpty() ? mainFrame()->url() : url);
	QString styleSheet((QStringLiteral("html {color: %1;} a {color: %2;} a:visited {color: %3;}")).arg(SettingsManager::getValue(SettingsManager::Content_TextColorOption).toString()).arg(SettingsManager::getValue(SettingsManager::Content_LinkColorOption).toString()).arg(SettingsManager::getValue(SettingsManager::Content_VisitedLinkColorOption).toString()).toUtf8());
	QWebElement media(mainFrame()->findFirstElement(QLatin1String("img, audio source, video source")));
	const bool isViewingMedia(!media.isNull() && QUrl(media.attribute(QLatin1String("src"))) == currentUrl);

	if (isViewingMedia && media.tagName().toLower() == QLatin1String("img"))
	{
		styleSheet += QLatin1String("html {width:100%;height:100%;} body {display:-webkit-flex;margin:0;padding:0;-webkit-align-items:center;text-align:center;} img {max-width:100%;max-height:100%;margin:auto;-webkit-user-select:none;} .zoomedIn {display:table;} .zoomedIn body {display:table-cell;vertical-align:middle;} .zoomedIn img {max-width:none;max-height:none;cursor:-webkit-zoom-out;} .zoomedIn .drag {cursor:move;} .zoomedOut img {cursor:-webkit-zoom-in;}");

		settings()->setAttribute(QWebSettings::AutoLoadImages, true);
		settings()->setAttribute(QWebSettings::JavascriptEnabled, true);

		runScript(QLatin1String("imageViewer"));
	}

	if (isViewingMedia != m_isViewingMedia)
	{
		m_isViewingMedia = isViewingMedia;

		emit viewingMediaChanged(m_isViewingMedia);
	}

	if (!SettingsManager::getValue(SettingsManager::Interface_ShowScrollBarsOption).toBool())
	{
		styleSheet.append(QLatin1String("body::-webkit-scrollbar {display:none;}"));
	}

	const QString userSyleSheet(m_widget ? m_widget->getOption(SettingsManager::Content_UserStyleSheetOption, currentUrl).toString() : QString());

	if (!userSyleSheet.isEmpty())
	{
		QFile file(userSyleSheet);
		file.open(QIODevice::ReadOnly);

		styleSheet.append(file.readAll());
	}

	settings()->setUserStyleSheetUrl(QUrl(QLatin1String("data:text/css;charset=utf-8;base64,") + styleSheet.toUtf8().toBase64()));
}

void QtWebKitPage::javaScriptAlert(QWebFrame *frame, const QString &message)
{
	if (m_ignoreJavaScriptPopups)
	{
		return;
	}

	if (!m_widget || !m_widget->parentWidget())
	{
		QWebPage::javaScriptAlert(frame, message);

		return;
	}

	emit m_widget->needsAttention();

	ContentsDialog dialog(ThemesManager::getIcon(QLatin1String("dialog-information")), tr("JavaScript"), message, QString(), QDialogButtonBox::Ok, nullptr, m_widget);
	dialog.setCheckBox(tr("Disable JavaScript popups"), false);

	connect(m_widget, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

	m_widget->showDialog(&dialog);

	if (dialog.getCheckBoxState())
	{
		m_ignoreJavaScriptPopups = true;
	}
}

#ifdef OTTER_ENABLE_QTWEBKIT_LEGACY
void QtWebKitPage::javaScriptConsoleMessage(const QString &note, int line, const QString &source)
{
	Console::addMessage(note, Console::JavaScriptCategory, Console::ErrorLevel, source, line, (m_widget ? m_widget->getWindowIdentifier() : 0));
}
#endif

void QtWebKitPage::triggerAction(QWebPage::WebAction action, bool checked)
{
	if (action == InspectElement && m_widget && !m_widget->isInspecting())
	{
		QVariantMap parameters;
		parameters[QLatin1String("isChecked")] = true;

		settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);

		m_widget->triggerAction(ActionsManager::InspectPageAction, parameters);
	}

	QWebPage::triggerAction(action, checked);
}

QVariant QtWebKitPage::runScript(const QString &path, QWebElement element)
{
	if (element.isNull())
	{
		element = mainFrame()->documentElement();
	}

	QFile file(QString(":/modules/backends/web/qtwebkit/resources/%1.js").arg(path));
	file.open(QIODevice::ReadOnly);

	const QVariant result(element.evaluateJavaScript(file.readAll()));

	file.close();

	return result;
}

QWebPage* QtWebKitPage::createWindow(QWebPage::WebWindowType type)
{
	if (type == QWebPage::WebBrowserWindow)
	{
		QtWebKitWebWidget *widget(nullptr);
		QString popupsPolicy(SettingsManager::getValue(SettingsManager::Content_PopupsPolicyOption).toString());
		bool isPopup(true);

		if (m_widget)
		{
			popupsPolicy = m_widget->getOption(SettingsManager::Content_PopupsPolicyOption, (m_widget ? m_widget->getRequestedUrl() : QUrl())).toString();
			isPopup = currentFrame()->hitTestContent(m_widget->getClickPosition()).linkUrl().isEmpty();
		}

		if (isPopup)
		{
			if (popupsPolicy == QLatin1String("blockAll"))
			{
				return nullptr;
			}

			if (popupsPolicy == QLatin1String("ask"))
			{
				QtWebKitPage *page(new QtWebKitPage());
				page->markAsPopup();

				connect(page, SIGNAL(aboutToNavigate(QUrl,QWebFrame*,QWebPage::NavigationType)), this, SLOT(removePopup(QUrl)));

				return page;
			}
		}

		if (m_widget)
		{
			widget = qobject_cast<QtWebKitWebWidget*>(m_widget->clone(false));
		}
		else
		{
			widget = new QtWebKitWebWidget(settings()->testAttribute(QWebSettings::PrivateBrowsingEnabled), nullptr, nullptr);
		}

		widget->pageLoadStarted();

		emit requestedNewWindow(widget, WindowsManager::calculateOpenHints(popupsPolicy == QLatin1String("openAllInBackground") ? (WindowsManager::NewTabOpen | WindowsManager::BackgroundOpen) : WindowsManager::NewTabOpen));

		return widget->getPage();
	}

	return QWebPage::createWindow(type);
}

QString QtWebKitPage::chooseFile(QWebFrame *frame, const QString &suggestedFile)
{
	Q_UNUSED(frame)

	return Utils::getOpenPaths(QStringList(suggestedFile)).value(0);
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
	return QWebPage::userAgentForUrl(QUrl());
}

bool QtWebKitPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, QWebPage::NavigationType type)
{
	if (m_isPopup)
	{
		emit aboutToNavigate(request.url(), frame, type);

		return false;
	}

	if (frame && request.url().scheme() == QLatin1String("javascript"))
	{
		frame->documentElement().evaluateJavaScript(request.url().path());

		return false;
	}

	if (request.url().scheme() == QLatin1String("mailto"))
	{
		QDesktopServices::openUrl(request.url());

		return false;
	}

	if (type == QWebPage::NavigationTypeFormSubmitted && QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier))
	{
		m_networkManager->setFormRequest(request.url());
	}

	if (type == QWebPage::NavigationTypeFormResubmitted && SettingsManager::getValue(SettingsManager::Choices_WarnFormResendOption).toBool())
	{
		bool cancel(false);
		bool warn(true);

		if (m_widget)
		{
			ContentsDialog dialog(ThemesManager::getIcon(QLatin1String("dialog-warning")), tr("Question"), tr("Are you sure that you want to send form data again?"), tr("Do you want to resend data?"), (QDialogButtonBox::Yes | QDialogButtonBox::Cancel), nullptr, m_widget);
			dialog.setCheckBox(tr("Do not show this message again"), false);

			connect(m_widget, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

			m_widget->showDialog(&dialog);

			cancel = !dialog.isAccepted();
			warn = !dialog.getCheckBoxState();
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

			cancel = (dialog.exec() == QMessageBox::Cancel);
			warn = !dialog.checkBox()->isChecked();
		}

		SettingsManager::setValue(SettingsManager::Choices_WarnFormResendOption, warn);

		if (cancel)
		{
			return false;
		}
	}

	emit aboutToNavigate(request.url(), frame, type);

	return true;
}

bool QtWebKitPage::javaScriptConfirm(QWebFrame *frame, const QString &message)
{
	if (m_ignoreJavaScriptPopups)
	{
		return false;
	}

	if (!m_widget || !m_widget->parentWidget())
	{
		return QWebPage::javaScriptConfirm(frame, message);
	}

	emit m_widget->needsAttention();

	ContentsDialog dialog(ThemesManager::getIcon(QLatin1String("dialog-information")), tr("JavaScript"), message, QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), nullptr, m_widget);
	dialog.setCheckBox(tr("Disable JavaScript popups"), false);

	connect(m_widget, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

	m_widget->showDialog(&dialog);

	if (dialog.getCheckBoxState())
	{
		m_ignoreJavaScriptPopups = true;
	}

	return dialog.isAccepted();
}

bool QtWebKitPage::javaScriptPrompt(QWebFrame *frame, const QString &message, const QString &defaultValue, QString *result)
{
	if (m_ignoreJavaScriptPopups)
	{
		return false;
	}

	if (!m_widget || !m_widget->parentWidget())
	{
		return QWebPage::javaScriptPrompt(frame, message, defaultValue, result);
	}

	emit m_widget->needsAttention();

	QWidget *widget(new QWidget(m_widget));
	QLineEdit *lineEdit(new QLineEdit(defaultValue, widget));
	QLabel *label(new QLabel(message, widget));
	label->setBuddy(lineEdit);
	label->setTextFormat(Qt::PlainText);

	QVBoxLayout *layout(new QVBoxLayout(widget));
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(label);
	layout->addWidget(lineEdit);

	ContentsDialog dialog(ThemesManager::getIcon(QLatin1String("dialog-information")), tr("JavaScript"), QString(), QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), widget, m_widget);
	dialog.setCheckBox(tr("Disable JavaScript popups"), false);

	connect(m_widget, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

	m_widget->showDialog(&dialog);

	if (dialog.isAccepted())
	{
		*result = lineEdit->text();
	}

	if (dialog.getCheckBoxState())
	{
		m_ignoreJavaScriptPopups = true;
	}

	return dialog.isAccepted();
}

bool QtWebKitPage::event(QEvent *event)
{
	if (event->type() == QEvent::Wheel)
	{
		QWheelEvent *wheelEvent(static_cast<QWheelEvent*>(event));

		if (wheelEvent->buttons() == Qt::RightButton)
		{
			return false;
		}
	}

	return QWebPage::event(event);
}

bool QtWebKitPage::extension(QWebPage::Extension extension, const QWebPage::ExtensionOption *option, QWebPage::ExtensionReturn *output)
{
	if (extension == QWebPage::ChooseMultipleFilesExtension && m_widget)
	{
		const QWebPage::ChooseMultipleFilesExtensionOption *filesOption(static_cast<const QWebPage::ChooseMultipleFilesExtensionOption*>(option));
		QWebPage::ChooseMultipleFilesExtensionReturn *filesOutput(static_cast<QWebPage::ChooseMultipleFilesExtensionReturn*>(output));

		filesOutput->fileNames = Utils::getOpenPaths(filesOption->suggestedFileNames, QStringList(), true);

		return true;
	}

	if (extension == QWebPage::ErrorPageExtension)
	{
		const QWebPage::ErrorPageExtensionOption *errorOption(static_cast<const QWebPage::ErrorPageExtensionOption*>(option));
		QWebPage::ErrorPageExtensionReturn *errorOutput(static_cast<QWebPage::ErrorPageExtensionReturn*>(output));

		if (!errorOption || !errorOutput)
		{
			return false;
		}

		QString domain;

		if (errorOption->domain == QWebPage::QtNetwork)
		{
			domain = QLatin1String("QtNetwork");
		}
		else if (errorOption->domain == QWebPage::WebKit)
		{
			domain = QLatin1String("WebKit");
		}
		else
		{
			domain = QLatin1String("HTTP");
		}

		Console::addMessage(tr("%1 error #%2: %3").arg(domain).arg(errorOption->error).arg(errorOption->errorString), Console::NetworkCategory, Console::ErrorLevel, errorOption->url.toString(), -1, (m_widget ? m_widget->getWindowIdentifier() : 0));

		if (errorOption->domain == QWebPage::WebKit && (errorOption->error == 102 || errorOption->error == 203))
		{
			return false;
		}

		QString title;

		if ((errorOption->domain == QWebPage::QtNetwork && (errorOption->error == QNetworkReply::HostNotFoundError || errorOption->error == QNetworkReply::ContentNotFoundError)) || (errorOption->domain == QWebPage::Http && errorOption->error == 404))
		{
			if (errorOption->url.isLocalFile())
			{
				title = tr("File not found");
			}
			else
			{
				title = tr("Server not found");
			}
		}
		else if (errorOption->domain == QWebPage::QtNetwork && errorOption->error == QNetworkReply::ConnectionRefusedError)
		{
			title = tr("Connection refused");
		}
		else if (errorOption->domain == QWebPage::WebKit)
		{
			title = tr("WebKit error %1").arg(errorOption->error);
		}
		else
		{
			title = tr("Network error %1").arg(errorOption->error);
		}

		errorOutput->baseUrl = errorOption->url;
		errorOutput->content = Utils::createErrorPage(errorOption->url, title, errorOption->errorString).toUtf8();

		return true;
	}

	return false;
}

bool QtWebKitPage::shouldInterruptJavaScript()
{
	if (m_widget)
	{
		ContentsDialog dialog(ThemesManager::getIcon(QLatin1String("dialog-warning")), tr("Question"), tr("The script on this page appears to have a problem."), tr("Do you want to stop the script?"), (QDialogButtonBox::Yes | QDialogButtonBox::No), nullptr, m_widget);

		connect(m_widget, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

		m_widget->showDialog(&dialog);

		return dialog.isAccepted();
	}

	return QWebPage::shouldInterruptJavaScript();
}

bool QtWebKitPage::supportsExtension(QWebPage::Extension extension) const
{
	return (extension == QWebPage::ChooseMultipleFilesExtension || extension == QWebPage::ErrorPageExtension);
}

bool QtWebKitPage::isViewingMedia() const
{
	return m_isViewingMedia;
}

}
