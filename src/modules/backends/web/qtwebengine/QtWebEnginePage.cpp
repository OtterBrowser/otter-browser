/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "QtWebEnginePage.h"
#include "QtWebEngineWebWidget.h"
#include "../../../../core/Console.h"
#include "../../../../core/Utils.h"
#include "../../../../ui/ContentsDialog.h"

#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtGui/QDesktopServices>
#include <QtWebEngineWidgets/QWebEngineProfile>
#include <QtWebEngineWidgets/QWebEngineSettings>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>

template<typename Arg, typename R, typename C>

struct InvokeWrapper
{
	R *receiver;
	void (C::*memberFunction)(Arg);

	void operator()(Arg result)
	{
		(receiver->*memberFunction)(result);
	}
};

template<typename Arg, typename R, typename C>

InvokeWrapper<Arg, R, C> invoke(R *receiver, void (C::*memberFunction)(Arg))
{
	InvokeWrapper<Arg, R, C> wrapper = {receiver, memberFunction};

	return wrapper;
}

namespace Otter
{

QtWebEnginePage::QtWebEnginePage(bool isPrivate, QtWebEngineWebWidget *parent) : QWebEnginePage((isPrivate ? new QWebEngineProfile(parent) : QWebEngineProfile::defaultProfile()), parent),
	m_widget(parent),
	m_previousNavigationType(QtWebEnginePage::NavigationTypeOther),
	m_ignoreJavaScriptPopups(false),
	m_isViewingMedia(false)
{
	connect(this, SIGNAL(loadFinished(bool)), this, SLOT(pageLoadFinished()));
}

void QtWebEnginePage::pageLoadFinished()
{
	m_ignoreJavaScriptPopups = false;

	toHtml(invoke(this, &QtWebEnginePage::handlePageLoaded));
}

void QtWebEnginePage::handlePageLoaded(const QString &result)
{
	QString string(url().toString());
	string.truncate(1000);

	const QRegularExpressionMatch match = QRegularExpression(QStringLiteral(">(<img style=\"-webkit-user-select: none;(?: cursor: zoom-in;)?\"|<video controls=\"\" autoplay=\"\" name=\"media\"><source) src=\"%1").arg(QRegularExpression::escape(string))).match(result);
	const bool isViewingMedia = match.hasMatch();

	if (isViewingMedia && match.captured().startsWith(QLatin1String("><img")))
	{
		settings()->setAttribute(QWebEngineSettings::AutoLoadImages, true);
		settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);

		QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/imageViewer.js"));
		file.open(QIODevice::ReadOnly);

		runJavaScript(file.readAll());

		file.close();
	}

	if (isViewingMedia != m_isViewingMedia)
	{
		m_isViewingMedia = isViewingMedia;

		emit viewingMediaChanged(m_isViewingMedia);
	}
}

void QtWebEnginePage::javaScriptAlert(const QUrl &url, const QString &message)
{
	if (m_ignoreJavaScriptPopups)
	{
		return;
	}

	if (!m_widget || !m_widget->parentWidget())
	{
		QWebEnginePage::javaScriptAlert(url, message);

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

void QtWebEnginePage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &note, int line, const QString &source)
{
	MessageLevel mappedLevel = Otter::LogMessageLevel;

	if (level == QWebEnginePage::WarningMessageLevel)
	{
		mappedLevel = Otter::WarningMessageLevel;
	}
	else if (level == QWebEnginePage::ErrorMessageLevel)
	{
		mappedLevel = Otter::ErrorMessageLevel;
	}

	Console::addMessage(note, JavaScriptMessageCategory, mappedLevel, source, line);
}

QWebEnginePage* QtWebEnginePage::createWindow(QWebEnginePage::WebWindowType type)
{
	if (type == QtWebEnginePage::WebBrowserWindow || type == QWebEnginePage::WebBrowserTab)
	{
		QtWebEngineWebWidget *widget = NULL;

		if (m_widget)
		{
			widget = qobject_cast<QtWebEngineWebWidget*>(m_widget->clone(false));
			widget->setRequestedUrl(m_widget->getRequestedUrl(), false, true);
		}
		else
		{
			widget = new QtWebEngineWebWidget(false, NULL, NULL);
		}

		emit requestedNewWindow(widget, WindowsManager::DefaultOpen);

		return widget->getPage();
	}

	return QWebEnginePage::createWindow(type);
}

bool QtWebEnginePage::acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool isMainFrame)
{
	if (isMainFrame && url.scheme() == QLatin1String("javascript"))
	{
		runJavaScript(url.path());

		return false;
	}

	if (url.scheme() == QLatin1String("mailto"))
	{
		QDesktopServices::openUrl(url);

		return false;
	}

	if (isMainFrame && type == QWebEnginePage::NavigationTypeReload && m_previousNavigationType == QWebEnginePage::NavigationTypeFormSubmitted && SettingsManager::getValue(QLatin1String("Choices/WarnFormResend")).toBool())
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

	if (isMainFrame && type != QWebEnginePage::NavigationTypeReload)
	{
		m_previousNavigationType = type;
	}

	return true;
}

bool QtWebEnginePage::javaScriptConfirm(const QUrl &url, const QString &message)
{
	if (m_ignoreJavaScriptPopups)
	{
		return false;
	}

	if (!m_widget || !m_widget->parentWidget())
	{
		return QWebEnginePage::javaScriptConfirm(url, message);
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

bool QtWebEnginePage::javaScriptPrompt(const QUrl &url, const QString &message, const QString &defaultValue, QString *result)
{
	if (m_ignoreJavaScriptPopups)
	{
		return false;
	}

	if (!m_widget || !m_widget->parentWidget())
	{
		return QWebEnginePage::javaScriptPrompt(url, message, defaultValue, result);
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

bool QtWebEnginePage::isViewingMedia() const
{
	return m_isViewingMedia;
}

}
