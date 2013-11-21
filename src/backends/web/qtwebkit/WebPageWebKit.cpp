#include "WebPageWebKit.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtWidgets/QApplication>
#include <QtWebKit/QWebHistory>
#include <QtWebKitWidgets/QWebFrame>

namespace Otter
{

WebPageWebKit::WebPageWebKit(QObject *parent) : QWebPage(parent)
{
}

void WebPageWebKit::triggerAction(WebAction action, bool checked)
{
	if (action == QWebPage::Back || action == QWebPage::Forward || action == QWebPage::OpenLink)
	{
		QVariantHash data;
		data["position"] = mainFrame()->scrollPosition();
		data["zoom"] = (mainFrame()->zoomFactor() * 100);

		history()->currentItem().setUserData(data);
	}

	QWebPage::triggerAction(action, checked);
}

bool WebPageWebKit::extension(QWebPage::Extension extension, const QWebPage::ExtensionOption *option, QWebPage::ExtensionReturn *output)
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

bool WebPageWebKit::supportsExtension(QWebPage::Extension extension) const
{
	return (extension == QWebPage::ErrorPageExtension);
}

}
