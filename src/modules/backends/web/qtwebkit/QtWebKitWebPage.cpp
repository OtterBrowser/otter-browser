#include "QtWebKitWebPage.h"
#include "QtWebKitWebWidget.h"
#include "../../../../core/SettingsManager.h"

#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtNetwork/QNetworkReply>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLayout>
#include <QtWidgets/QMessageBox>
#include <QtWebKit/QWebHistory>
#include <QtWebKitWidgets/QWebFrame>

namespace Otter
{

QtWebKitWebPage::QtWebKitWebPage(QtWebKitWebWidget *parent) : QWebPage(parent),
	m_webWidget(parent),
	m_ignoreJavaScriptPopups(false)
{
	optionChanged("Browser/ZoomTextOnly", SettingsManager::getValue("Browser/ZoomTextOnly"));

	connect(this, SIGNAL(loadFinished(bool)), this, SLOT(clearIgnoreJavaScriptPopups()));
	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
}

void QtWebKitWebPage::clearIgnoreJavaScriptPopups()
{
	m_ignoreJavaScriptPopups = false;
}

void QtWebKitWebPage::optionChanged(const QString &option, const QVariant &value)
{
	if (option == "Browser/ZoomTextOnly")
	{
		settings()->setAttribute(QWebSettings::ZoomTextOnly, value.toBool());
	}
}

void QtWebKitWebPage::javaScriptAlert(QWebFrame *frame, const QString &message)
{
	if (m_ignoreJavaScriptPopups)
	{
		return;
	}

	if (m_webWidget)
	{
		QMessageBox dialog;
		dialog.setModal(false);
		dialog.setWindowTitle(tr("JavaScript"));
		dialog.setText(message.toHtmlEscaped());
		dialog.setStandardButtons(QMessageBox::Ok);
		dialog.setCheckBox(new QCheckBox(tr("Disable JavaScript popups")));

		QEventLoop eventLoop;

		m_webWidget->showDialog(&dialog);

		connect(&dialog, SIGNAL(finished(int)), &eventLoop, SLOT(quit()));
		connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

		eventLoop.exec();

		m_webWidget->hideDialog(&dialog);

		if (dialog.checkBox()->isChecked())
		{
			m_ignoreJavaScriptPopups = true;
		}

		return;
	}

	QWebPage::javaScriptAlert(frame, message);
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

QWebPage* QtWebKitWebPage::createWindow(QWebPage::WebWindowType type)
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
		QMessageBox dialog;
		dialog.setWindowTitle(tr("Question"));
		dialog.setText(tr("Are you sure that you want to send form data again?"));
		dialog.setInformativeText("Do you want to resend data?");
		dialog.setIcon(QMessageBox::Question);
		dialog.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
		dialog.setDefaultButton(QMessageBox::Cancel);
		dialog.setCheckBox(new QCheckBox(tr("Do not show this message again")));

		bool cancel = false;

		if (m_webWidget)
		{
			dialog.setModal(false);

			QEventLoop eventLoop;

			m_webWidget->showDialog(&dialog);

			connect(&dialog, SIGNAL(finished(int)), &eventLoop, SLOT(quit()));
			connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

			eventLoop.exec();

			m_webWidget->hideDialog(&dialog);

			cancel = (dialog.buttonRole(dialog.clickedButton()) == QMessageBox::RejectRole);
		}
		else
		{
			cancel = (dialog.exec() == QMessageBox::Cancel);
		}

		SettingsManager::setValue("Choices/WarnFormResend", !dialog.checkBox()->isChecked());

		if (cancel)
		{
			return false;
		}
	}

	return QWebPage::acceptNavigationRequest(frame, request, type);
}

bool QtWebKitWebPage::javaScriptConfirm(QWebFrame *frame, const QString &message)
{
	if (m_ignoreJavaScriptPopups)
	{
		return false;
	}

	if (m_webWidget)
	{
		QMessageBox dialog;
		dialog.setModal(false);
		dialog.setWindowTitle(tr("JavaScript"));
		dialog.setText(message.toHtmlEscaped());
		dialog.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		dialog.setCheckBox(new QCheckBox(tr("Disable JavaScript popups")));

		QEventLoop eventLoop;

		m_webWidget->showDialog(&dialog);

		connect(&dialog, SIGNAL(finished(int)), &eventLoop, SLOT(quit()));
		connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

		eventLoop.exec();

		m_webWidget->hideDialog(&dialog);

		if (dialog.checkBox()->isChecked())
		{
			m_ignoreJavaScriptPopups = true;
		}

		return (dialog.buttonRole(dialog.clickedButton()) == QMessageBox::AcceptRole);
	}

	return QWebPage::javaScriptConfirm(frame, message);
}

bool QtWebKitWebPage::javaScriptPrompt(QWebFrame *frame, const QString &message, const QString &defaultValue, QString *result)
{
	if (m_ignoreJavaScriptPopups)
	{
		return false;
	}

	if (m_webWidget)
	{
		QInputDialog dialog;
		dialog.setModal(false);
		dialog.setWindowTitle(tr("JavaScript"));
		dialog.setLabelText(message.toHtmlEscaped());
		dialog.setInputMode(QInputDialog::TextInput);
		dialog.setTextValue(defaultValue);

		QEventLoop eventLoop;

		m_webWidget->showDialog(&dialog);

		connect(&dialog, SIGNAL(finished(int)), &eventLoop, SLOT(quit()));
		connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

		eventLoop.exec();

		m_webWidget->hideDialog(&dialog);

		if (dialog.result() == QDialog::Accepted)
		{
			*result = dialog.textValue();
		}

		return (dialog.result() == QDialog::Accepted);
	}

	return QWebPage::javaScriptPrompt(frame, message, defaultValue, result);
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

bool QtWebKitWebPage::shouldInterruptJavaScript()
{
	if (m_webWidget)
	{
		QMessageBox dialog;
		dialog.setModal(false);
		dialog.setWindowTitle(tr("Question"));
		dialog.setText(tr("The script on this page appears to have a problem."));
		dialog.setInformativeText(tr("Do you want to stop the script?"));
		dialog.setIcon(QMessageBox::Question);
		dialog.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

		QEventLoop eventLoop;

		m_webWidget->showDialog(&dialog);

		connect(&dialog, SIGNAL(finished(int)), &eventLoop, SLOT(quit()));
		connect(this, SIGNAL(destroyed()), &eventLoop, SLOT(quit()));

		eventLoop.exec();

		m_webWidget->hideDialog(&dialog);

		return (dialog.buttonRole(dialog.clickedButton()) == QMessageBox::YesRole);
	}

	return QWebPage::shouldInterruptJavaScript();
}

bool QtWebKitWebPage::supportsExtension(QWebPage::Extension extension) const
{
	return (extension == QWebPage::ErrorPageExtension);
}

}
