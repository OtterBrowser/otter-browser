/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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
#include "../../../../core/Utils.h"
#include "../../../../ui/ContentsDialog.h"

#include <QtCore/QFile>
#include <QtGui/QDesktopServices>
#include <QtGui/QGuiApplication>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QFileDialog>
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
	optionChanged(QLatin1String("Interface/ShowScrollBars"), SettingsManager::getValue(QLatin1String("Interface/ShowScrollBars")));

	connect(this, SIGNAL(loadFinished(bool)), this, SLOT(pageLoadFinished()));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
}

QtWebKitPage::QtWebKitPage() : QWebPage(),
	m_widget(NULL),
	m_networkManager(NULL),
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

void QtWebKitPage::optionChanged(const QString &option, const QVariant &value)
{
	Q_UNUSED(value)

	if (option.startsWith(QLatin1String("Content/")) || option == QLatin1String("Interface/ShowScrollBars"))
	{
		updateStyleSheets();
	}
}

void QtWebKitPage::pageLoadFinished()
{
	m_ignoreJavaScriptPopups = false;

	updateStyleSheets();

	if (m_widget)
	{
		const QStringList domainList = ContentBlockingManager::createSubdomainList(m_widget->getUrl().host());

		for (int i = 0; i < domainList.count(); ++i)
		{
			applyContentBlockingRules(ContentBlockingManager::getStyleSheetBlackList(m_widget->getContentBlockingProfiles()).values(domainList.at(i)), true);
			applyContentBlockingRules(ContentBlockingManager::getStyleSheetWhiteList(m_widget->getContentBlockingProfiles()).values(domainList.at(i)), false);
		}
	}
}

void QtWebKitPage::removePopup(const QUrl &url)
{
	QtWebKitPage *page = qobject_cast<QtWebKitPage*>(sender());

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
	for (int i = 0; i < rules.count(); ++i)
	{
		const QWebElementCollection elements = mainFrame()->documentElement().findAll(rules.at(i));

		for (int j = 0; j < elements.count(); ++j)
		{
			QWebElement element = elements.at(j);

			if (element.isNull())
			{
				continue;
			}

			if (remove)
			{
				element.removeFromDocument();
			}
			else
			{
				element.setStyleProperty(QLatin1String("display"), QLatin1String("block"));
			}
		}
	}
}

void QtWebKitPage::updateStyleSheets(const QUrl &url)
{
	const QUrl currentUrl = (url.isEmpty() ? mainFrame()->url() : url);
	QString styleSheet = QString(QStringLiteral("html {color: %1;} a {color: %2;} a:visited {color: %3;}")).arg(SettingsManager::getValue(QLatin1String("Content/TextColor")).toString()).arg(SettingsManager::getValue(QLatin1String("Content/LinkColor")).toString()).arg(SettingsManager::getValue(QLatin1String("Content/VisitedLinkColor")).toString()).toUtf8() + (m_widget ? ContentBlockingManager::getStyleSheet(m_widget->getContentBlockingProfiles()) : QByteArray());
	QWebElement media = mainFrame()->findFirstElement(QLatin1String("img, audio source, video source"));
	const bool isViewingMedia = (!media.isNull() && QUrl(media.attribute(QLatin1String("src"))) == currentUrl);

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

	if (!SettingsManager::getValue(QLatin1String("Interface/ShowScrollBars")).toBool())
	{
		styleSheet.append(QLatin1String("body::-webkit-scrollbar {display:none;}"));
	}

	const QString userSyleSheet = (m_widget ? m_widget->getOption(QLatin1String("Content/UserStyleSheet"), currentUrl).toString() : QString());

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

	ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-information")), tr("JavaScript"), message, QString(), QDialogButtonBox::Ok, NULL, m_widget);
	dialog.setCheckBox(tr("Disable JavaScript popups"), false);

	connect(m_widget, SIGNAL(aboutToReload()), &dialog, SLOT(close()));

	m_widget->showDialog(&dialog);

	if (dialog.getCheckBoxState())
	{
		m_ignoreJavaScriptPopups = true;
	}
}

void QtWebKitPage::javaScriptConsoleMessage(const QString &note, int line, const QString &source)
{
	Console::addMessage(note, JavaScriptMessageCategory, ErrorMessageLevel, source, line);
}

void QtWebKitPage::triggerAction(QWebPage::WebAction action, bool checked)
{
	if (action == InspectElement && m_widget)
	{
		QVariantMap parameters;
		parameters[QLatin1String("isChecked")] = true;

		m_widget->triggerAction(ActionsManager::InspectPageAction, parameters);
	}

	QWebPage::triggerAction(action, checked);
}

QVariant QtWebKitPage::runScript(const QString &path, QWebElement element)
{
	QFile file(QString(":/modules/backends/web/qtwebkit/resources/%1.js").arg(path));
	file.open(QIODevice::ReadOnly);

	const QVariant result = (element.isNull() ? mainFrame()->evaluateJavaScript(file.readAll()) : element.evaluateJavaScript(file.readAll()));

	file.close();

	return result;
}

QWebPage* QtWebKitPage::createWindow(QWebPage::WebWindowType type)
{
	if (type == QWebPage::WebBrowserWindow)
	{
		QtWebKitWebWidget *widget = NULL;
		QString popupsPolicy = SettingsManager::getValue(QLatin1String("Content/PopupsPolicy")).toString();
		bool isPopup = true;

		if (m_widget)
		{
			popupsPolicy = m_widget->getOption(QLatin1String("Content/PopupsPolicy"), (m_widget ? m_widget->getRequestedUrl() : QUrl())).toString();
			isPopup = currentFrame()->hitTestContent(m_widget->getClickPosition()).linkUrl().isEmpty();
		}

		if (isPopup)
		{
			if (popupsPolicy == QLatin1String("blockAll"))
			{
				return NULL;
			}

			if (popupsPolicy == QLatin1String("ask"))
			{
				QtWebKitPage *page = new QtWebKitPage();
				page->markAsPopup();

				connect(page, SIGNAL(aboutToNavigate(QUrl,QWebFrame*,QWebPage::NavigationType)), this, SLOT(removePopup(QUrl)));

				return page;
			}
		}

		if (m_widget)
		{
			widget = qobject_cast<QtWebKitWebWidget*>(m_widget->clone(false));
			widget->setRequestedUrl(m_widget->getRequestedUrl(), false, true);
		}
		else
		{
			widget = new QtWebKitWebWidget(settings()->testAttribute(QWebSettings::PrivateBrowsingEnabled), NULL, NULL);
		}

		emit requestedNewWindow(widget, (popupsPolicy == QLatin1String("openAllInBackground") ? (WindowsManager::NewTabOpen | WindowsManager::BackgroundOpen) : WindowsManager::DefaultOpen));

		return widget->getPage();
	}

	return QWebPage::createWindow(type);
}

QString QtWebKitPage::getDefaultUserAgent() const
{
	return userAgentForUrl(QUrl());
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
		frame->evaluateJavaScript(request.url().path());

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

	if (type == QWebPage::NavigationTypeFormResubmitted && SettingsManager::getValue(QLatin1String("Choices/WarnFormResend")).toBool())
	{
		bool cancel = false;
		bool warn = true;

		if (m_widget)
		{
			ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-warning")), tr("Question"), tr("Are you sure that you want to send form data again?"), tr("Do you want to resend data?"), (QDialogButtonBox::Yes | QDialogButtonBox::Cancel), NULL, m_widget);
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

		SettingsManager::setValue(QLatin1String("Choices/WarnFormResend"), warn);

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

	ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-information")), tr("JavaScript"), message, QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), NULL, m_widget);
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

	QWidget *widget = new QWidget(m_widget);
	QLineEdit *lineEdit = new QLineEdit(defaultValue, widget);
	QLabel *label = new QLabel(message, widget);
	label->setBuddy(lineEdit);
	label->setTextFormat(Qt::PlainText);

	QVBoxLayout *layout = new QVBoxLayout(widget);
	layout->addWidget(label);
	layout->addWidget(lineEdit);

	ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-information")), tr("JavaScript"), QString(), QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), widget, m_widget);
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
		QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);

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
		const QWebPage::ChooseMultipleFilesExtensionOption *filesOption = static_cast<const QWebPage::ChooseMultipleFilesExtensionOption*>(option);
		QWebPage::ChooseMultipleFilesExtensionReturn *filesOutput = static_cast<QWebPage::ChooseMultipleFilesExtensionReturn*>(output);

		filesOutput->fileNames = QFileDialog::getOpenFileNames(m_widget, tr("Open File"), QString(), filesOption->suggestedFileNames.join(QLatin1Char(';')));

		return true;
	}

	if (extension == QWebPage::ErrorPageExtension)
	{
		const QWebPage::ErrorPageExtensionOption *errorOption = static_cast<const QWebPage::ErrorPageExtensionOption*>(option);
		QWebPage::ErrorPageExtensionReturn *errorOutput = static_cast<QWebPage::ErrorPageExtensionReturn*>(output);

		if (!errorOption || !errorOutput || (errorOption->error == 203 && errorOption->domain == QWebPage::WebKit))
		{
			return false;
		}

		errorOutput->baseUrl = errorOption->url;
		errorOutput->content = Utils::createErrorPage(errorOption->url, QString::number(errorOption->error), errorOption->errorString).toUtf8();

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

		Console::addMessage(tr("%1 error #%2: %3").arg(domain).arg(errorOption->error).arg(errorOption->errorString), NetworkMessageCategory, ErrorMessageLevel, errorOption->url.toString());

		return true;
	}

	return false;
}

bool QtWebKitPage::shouldInterruptJavaScript()
{
	if (m_widget)
	{
		ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-warning")), tr("Question"), tr("The script on this page appears to have a problem."), tr("Do you want to stop the script?"), (QDialogButtonBox::Yes | QDialogButtonBox::No), NULL, m_widget);

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
