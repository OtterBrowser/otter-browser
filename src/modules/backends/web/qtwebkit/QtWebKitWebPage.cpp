#include "QtWebKitWebPage.h"
#include "QtWebKitWebWidget.h"
#include "../../../../core/SettingsManager.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtNetwork/QNetworkReply>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QMessageBox>
#include <QtWebKit/QWebHistory>
#include <QtWebKitWidgets/QWebFrame>

namespace Otter
{

QtWebKitWebPage::QtWebKitWebPage(WebWidget *parent) : QWebPage(parent),
	m_webWidget(parent)
{
}

void QtWebKitWebPage::triggerAction(QWebPage::WebAction action, bool checked)
{
	if (action == InspectElement && m_webWidget)
	{
		m_webWidget->triggerAction(InspectPageAction, true);
	}

	QWebPage::triggerAction(action, checked);
}

void QtWebKitWebPage::setParent(WebWidget *parent)
{
	m_webWidget = parent;

	QWebPage::setParent(parent);
}

QWebPage *QtWebKitWebPage::createWindow(QWebPage::WebWindowType type)
{
	if (type == QWebPage::WebBrowserWindow)
	{
		QtWebKitWebPage *page = new QtWebKitWebPage(NULL);

		emit requestedNewWindow(new QtWebKitWebWidget(settings()->testAttribute(QWebSettings::PrivateBrowsingEnabled), NULL, page));

		return page;
	}

	return QWebPage::createWindow(type);
}

bool QtWebKitWebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, QWebPage::NavigationType type)
{
	if (request.url().scheme() == "javascript" && frame)
	{
		frame->evaluateJavaScript(request.url().path());

		return true;
	}

	if (type == QWebPage::NavigationTypeFormResubmitted && SettingsManager::getValue("Choices/WarnFormResend", true).toBool())
	{
		QMessageBox messageBox;
		messageBox.setWindowTitle(tr("Question"));
		messageBox.setText(tr("Are you sure that you want to send form data again?"));
		messageBox.setInformativeText("Do you want to resend data?");
		messageBox.setIcon(QMessageBox::Question);
		messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
		messageBox.setDefaultButton(QMessageBox::Cancel);
		messageBox.setCheckBox(new QCheckBox(tr("Do not show this message again")));

		const bool cancel = (messageBox.exec() == QMessageBox::Cancel);

		SettingsManager::setValue("Choices/WarnFormResend", !messageBox.checkBox()->isChecked());

		if (cancel)
		{
			return false;
		}
	}

	return QWebPage::acceptNavigationRequest(frame, request, type);
}

bool QtWebKitWebPage::extension(QWebPage::Extension extension, const QWebPage::ExtensionOption *option, QWebPage::ExtensionReturn *output)
{
	if (extension == QWebPage::ErrorPageExtension)
	{
		const QWebPage::ErrorPageExtensionOption *errorOption = static_cast<const QWebPage::ErrorPageExtensionOption*>(option);
		QWebPage::ErrorPageExtensionReturn *errorOutput = static_cast<QWebPage::ErrorPageExtensionReturn*>(output);

		if (!errorOption || !errorOutput)
		{
			return false;
		}

		QFile file(":/files/error.html");
		file.open(QIODevice::ReadOnly | QIODevice::Text);

		QTextStream stream(&file);
		stream.setCodec("UTF-8");

		QHash<QString, QString> variables;
		variables["title"] = tr("Error %1").arg(errorOption->error);
		variables["description"] = errorOption->errorString;
		variables["dir"] = (QGuiApplication::isLeftToRight() ? "ltr" : "rtl");

		QString html = stream.readAll();
		QHash<QString, QString>::iterator iterator;

		for (iterator = variables.begin(); iterator != variables.end(); ++iterator)
		{
			html.replace(QString("{%1}").arg(iterator.key()), iterator.value());
		}

		errorOutput->content = html.toUtf8();

		return true;
	}

	return false;
}

bool QtWebKitWebPage::supportsExtension(QWebPage::Extension extension) const
{
	return (extension == QWebPage::ErrorPageExtension);
}

}
