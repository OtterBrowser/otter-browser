/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "QtWebKitWebPage.h"
#include "QtWebKitWebWidget.h"
#include "../../../../core/Console.h"
#include "../../../../core/ContentBlockingManager.h"
#include "../../../../core/NetworkManagerFactory.h"
#include "../../../../core/SettingsManager.h"
#include "../../../../core/Utils.h"
#include "../../../../ui/ContentsDialog.h"

#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtGui/QDesktopServices>
#include <QtNetwork/QNetworkReply>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWebKit/QWebElement>
#include <QtWebKit/QWebHistory>
#include <QtWebKitWidgets/QWebFrame>

namespace Otter
{

QtWebKitWebPage::QtWebKitWebPage(QtWebKitWebWidget *parent) : QWebPage(parent),
	m_webWidget(parent),
	m_ignoreJavaScriptPopups(false),
	m_isGlobalUserAgent(true)
{
	optionChanged(QLatin1String("Network/UserAgent"), SettingsManager::getValue(QLatin1String("Network/UserAgent")));
	optionChanged(QLatin1String("Content/ZoomTextOnly"), SettingsManager::getValue(QLatin1String("Content/ZoomTextOnly")));
	optionChanged(QLatin1String("Content/BackgroundColor"), QVariant());

	connect(this, SIGNAL(loadFinished(bool)), this, SLOT(pageLoadFinished()));
	connect(ContentBlockingManager::getInstance(), SIGNAL(styleSheetsUpdated()), this, SLOT(updatePageStyleSheets()));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
}

void QtWebKitWebPage::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Network/UserAgent"))
	{
		if (m_isGlobalUserAgent)
		{
			setUserAgent(value.toString(), NetworkManagerFactory::getUserAgent(value.toString()).value, false);
		}
	}
	else if (option.startsWith(QLatin1String("Content/")))
	{
		updatePageStyleSheets();
	}
}

void QtWebKitWebPage::pageLoadFinished()
{
	clearIgnoreJavaScriptPopups();

	if (ContentBlockingManager::isContentBlockingEnabled() && mainFrame()->url().isValid())
	{
		const QStringList domainList = ContentBlockingManager::createSubdomainList(mainFrame()->url().host());

		updateBlockedPageElements(domainList, false);
		updateBlockedPageElements(domainList, true);
	}
}

void QtWebKitWebPage::clearIgnoreJavaScriptPopups()
{
	m_ignoreJavaScriptPopups = false;
}

void QtWebKitWebPage::updatePageStyleSheets()
{
	const QString userStyleSheets = QString(QStringLiteral("html {color: %1;} a {color: %2;} a:visited {color: %3;}")).arg(SettingsManager::getValue(QLatin1String("Content/TextColor")).toString()).arg(SettingsManager::getValue(QLatin1String("Content/LinkColor")).toString()).arg(SettingsManager::getValue(QLatin1String("Content/VisitedLinkColor")).toString()).toUtf8() + ContentBlockingManager::getStyleSheetHidingRules();

	settings()->setUserStyleSheetUrl(QUrl(QLatin1String("data:text/css;charset=utf-8;base64,") + userStyleSheets.toUtf8().toBase64()));
}

void QtWebKitWebPage::updateBlockedPageElements(const QStringList domainList, const bool isException)
{
	for (int i = 0; i < domainList.count(); ++i)
	{
		const QList<QString> exceptionList = isException ? ContentBlockingManager::getHidingRulesExceptions().values(domainList.at(i)) : ContentBlockingManager::getSpecificDomainHidingRules().values(domainList.at(i));

		for (int j = 0; j < exceptionList.count(); ++j)
		{
			const QWebElementCollection elements = mainFrame()->documentElement().findAll(exceptionList.at(j));

			for (int k = 0; k < elements.count(); ++k)
			{
				QWebElement element = elements.at(k);

				if (element.isNull())
				{
					continue;
				}

				if (isException)
				{
					element.setStyleProperty(QLatin1String("display"), QLatin1String("block"));
				}
				else
				{
					element.removeFromDocument();
				}
			}
		}
	}
}

void QtWebKitWebPage::javaScriptAlert(QWebFrame *frame, const QString &message)
{
	if (m_ignoreJavaScriptPopups)
	{
		return;
	}

	if (!m_webWidget || !m_webWidget->parentWidget())
	{
		QWebPage::javaScriptAlert(frame, message);

		return;
	}

	ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-information")), tr("JavaScript"), message, QString(), QDialogButtonBox::Ok, NULL, m_webWidget);
	dialog.setCheckBox(tr("Disable JavaScript popups"), false);

	QEventLoop eventLoop;

	m_webWidget->showDialog(&dialog);

	connect(&dialog, SIGNAL(closed(bool,QDialogButtonBox::StandardButton)), &eventLoop, SLOT(quit()));
	connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

	eventLoop.exec();

	m_webWidget->hideDialog(&dialog);

	if (dialog.getCheckBoxState())
	{
		m_ignoreJavaScriptPopups = true;
	}
}

void QtWebKitWebPage::javaScriptConsoleMessage(const QString &note, int line, const QString &source)
{
	Console::addMessage(note, JavaScriptMessageCategory, ErrorMessageLevel, source, line);
}

void QtWebKitWebPage::triggerAction(QWebPage::WebAction action, bool checked)
{
	if (action == InspectElement && m_webWidget)
	{
		m_webWidget->triggerAction(InspectPageAction, true);
	}

	QWebPage::triggerAction(action, checked);
}

void QtWebKitWebPage::setParent(QtWebKitWebWidget *parent)
{
	m_webWidget = parent;

	QWebPage::setParent(parent);
}

void QtWebKitWebPage::setUserAgent(const QString &identifier, const QString &value, bool manual)
{
	m_userAgentIdentifier = ((identifier == QLatin1String("default")) ? QString() : identifier);
	m_userAgentParsed = QString(value).replace(QLatin1String("{platform}"), QLatin1String("%Platform%%Security%%Subplatform%")).replace(QLatin1String("{engineVersion}"), QLatin1String("%WebKitVersion%")).replace(QLatin1String("{engineName}"), QLatin1String("AppleWebKit")).replace(QLatin1String("{applicationVersion}"), QCoreApplication::applicationVersion());
	m_userAgentValue = value;
	m_isGlobalUserAgent = !manual;
}

QWebPage* QtWebKitWebPage::createWindow(QWebPage::WebWindowType type)
{
	if (type == QWebPage::WebBrowserWindow)
	{
		QtWebKitWebWidget *widget = NULL;

		if (m_webWidget)
		{
			widget = qobject_cast<QtWebKitWebWidget*>(m_webWidget->clone(false));
		}
		else
		{
			widget = new QtWebKitWebWidget(settings()->testAttribute(QWebSettings::PrivateBrowsingEnabled), NULL, NULL);
		}

		widget->setRequestedUrl(m_webWidget->getRequestedUrl(), false, true);

		emit requestedNewWindow(widget, DefaultOpen);

		return widget->getPage();
	}

	return QWebPage::createWindow(type);
}

QString QtWebKitWebPage::userAgentForUrl(const QUrl &url) const
{
	if (!m_userAgentIdentifier.isEmpty())
	{
		return m_userAgentParsed;
	}

	return QWebPage::userAgentForUrl(url);
}

QPair<QString, QString> QtWebKitWebPage::getUserAgent() const
{
	return qMakePair(m_userAgentIdentifier, m_userAgentValue);
}

bool QtWebKitWebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, QWebPage::NavigationType type)
{
	if (request.url().scheme() == QLatin1String("javascript") && frame)
	{
		frame->evaluateJavaScript(request.url().path());

		return false;
	}

	if (request.url().scheme() == QLatin1String("mailto"))
	{
		QDesktopServices::openUrl(request.url());

		return false;
	}

	if (type == QWebPage::NavigationTypeFormResubmitted && SettingsManager::getValue(QLatin1String("Choices/WarnFormResend")).toBool())
	{
		bool cancel = false;
		bool warn = true;

		if (m_webWidget)
		{
			ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-warning")), tr("Question"), tr("Are you sure that you want to send form data again?"), tr("Do you want to resend data?"), (QDialogButtonBox::Yes | QDialogButtonBox::Cancel), NULL, m_webWidget);
			dialog.setCheckBox(tr("Do not show this message again"), false);

			QEventLoop eventLoop;

			m_webWidget->showDialog(&dialog);

			connect(&dialog, SIGNAL(closed(bool,QDialogButtonBox::StandardButton)), &eventLoop, SLOT(quit()));
			connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

			eventLoop.exec();

			m_webWidget->hideDialog(&dialog);

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

	if (type == QWebPage::NavigationTypeReload && m_webWidget)
	{
		m_webWidget->markPageRealoded();
	}

	return QWebPage::acceptNavigationRequest(frame, request, type);
}

bool QtWebKitWebPage::javaScriptConfirm(QWebFrame *frame, const QString &message)
{
	if (m_ignoreJavaScriptPopups)
	{
		return false;
	}

	if (!m_webWidget || !m_webWidget->parentWidget())
	{
		return QWebPage::javaScriptConfirm(frame, message);
	}

	ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-information")), tr("JavaScript"), message, QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), NULL, m_webWidget);
	dialog.setCheckBox(tr("Disable JavaScript popups"), false);

	QEventLoop eventLoop;

	m_webWidget->showDialog(&dialog);

	connect(&dialog, SIGNAL(closed(bool,QDialogButtonBox::StandardButton)), &eventLoop, SLOT(quit()));
	connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

	eventLoop.exec();

	m_webWidget->hideDialog(&dialog);

	if (dialog.getCheckBoxState())
	{
		m_ignoreJavaScriptPopups = true;
	}

	return dialog.isAccepted();
}

bool QtWebKitWebPage::javaScriptPrompt(QWebFrame *frame, const QString &message, const QString &defaultValue, QString *result)
{
	if (m_ignoreJavaScriptPopups)
	{
		return false;
	}

	if (!m_webWidget || !m_webWidget->parentWidget())
	{
		return QWebPage::javaScriptPrompt(frame, message, defaultValue, result);
	}

	QWidget *widget = new QWidget(m_webWidget);
	QLineEdit *lineEdit = new QLineEdit(defaultValue, widget);
	QLabel *label = new QLabel(message, widget);
	label->setTextFormat(Qt::PlainText);

	QHBoxLayout *layout = new QHBoxLayout(widget);
	layout->addWidget(label);
	layout->addWidget(lineEdit);

	ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-information")), tr("JavaScript"), QString(), QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), widget, m_webWidget);
	dialog.setCheckBox(tr("Disable JavaScript popups"), false);

	QEventLoop eventLoop;

	m_webWidget->showDialog(&dialog);

	connect(&dialog, SIGNAL(closed(bool,QDialogButtonBox::StandardButton)), &eventLoop, SLOT(quit()));
	connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

	eventLoop.exec();

	m_webWidget->hideDialog(&dialog);

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

bool QtWebKitWebPage::extension(QWebPage::Extension extension, const QWebPage::ExtensionOption *option, QWebPage::ExtensionReturn *output)
{
	if (extension != QWebPage::ErrorPageExtension)
	{
		return false;
	}

	const QWebPage::ErrorPageExtensionOption *errorOption = static_cast<const QWebPage::ErrorPageExtensionOption*>(option);
	QWebPage::ErrorPageExtensionReturn *errorOutput = static_cast<QWebPage::ErrorPageExtensionReturn*>(output);

	if (!errorOption || !errorOutput)
	{
		return false;
	}

	QFile file(QLatin1String(":/files/error.html"));
	file.open(QIODevice::ReadOnly | QIODevice::Text);

	QTextStream stream(&file);
	stream.setCodec("UTF-8");

	QHash<QString, QString> variables;
	variables[QLatin1String("title")] = tr("Error %1").arg(errorOption->error);
	variables[QLatin1String("description")] = errorOption->errorString;
	variables[QLatin1String("dir")] = (QGuiApplication::isLeftToRight() ? QLatin1String("ltr") : QLatin1String("rtl"));

	QString html = stream.readAll();
	QHash<QString, QString>::iterator iterator;

	for (iterator = variables.begin(); iterator != variables.end(); ++iterator)
	{
		html.replace(QStringLiteral("{%1}").arg(iterator.key()), iterator.value());
	}

	errorOutput->baseUrl = errorOption->url;
	errorOutput->content = html.toUtf8();

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

bool QtWebKitWebPage::shouldInterruptJavaScript()
{
	if (m_webWidget)
	{
		ContentsDialog dialog(Utils::getIcon(QLatin1String("dialog-warning")), tr("Question"), tr("The script on this page appears to have a problem."), tr("Do you want to stop the script?"), (QDialogButtonBox::Yes | QDialogButtonBox::No), NULL, m_webWidget);
		QEventLoop eventLoop;

		m_webWidget->showDialog(&dialog);

		connect(&dialog, SIGNAL(finished(int)), &eventLoop, SLOT(quit()));
		connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

		eventLoop.exec();

		m_webWidget->hideDialog(&dialog);

		return dialog.isAccepted();
	}

	return QWebPage::shouldInterruptJavaScript();
}

bool QtWebKitWebPage::supportsExtension(QWebPage::Extension extension) const
{
	return (extension == QWebPage::ErrorPageExtension);
}

}
