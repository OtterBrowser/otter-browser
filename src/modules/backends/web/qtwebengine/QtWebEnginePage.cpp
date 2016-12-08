/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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
#include "QtWebEngineWebBackend.h"
#include "QtWebEngineWebWidget.h"
#include "../../../../core/AddonsManager.h"
#include "../../../../core/Console.h"
#include "../../../../core/ContentBlockingManager.h"
#include "../../../../core/ThemesManager.h"
#include "../../../../core/UserScript.h"
#include "../../../../core/Utils.h"
#include "../../../../ui/ContentsDialog.h"

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtGui/QDesktopServices>
#include <QtWebEngineWidgets/QWebEngineProfile>
#include <QtWebEngineWidgets/QWebEngineScript>
#include <QtWebEngineWidgets/QWebEngineScriptCollection>
#include <QtWebEngineWidgets/QWebEngineSettings>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>

namespace Otter
{

QtWebEnginePage::QtWebEnginePage(bool isPrivate, QtWebEngineWebWidget *parent) : QWebEnginePage((isPrivate ? new QWebEngineProfile(parent) : QWebEngineProfile::defaultProfile()), parent),
	m_widget(parent),
	m_previousNavigationType(QtWebEnginePage::NavigationTypeOther),
	m_ignoreJavaScriptPopups(false),
	m_isViewingMedia(false),
	m_isPopup(false)
{
	if (isPrivate)
	{
		connect(profile(), SIGNAL(downloadRequested(QWebEngineDownloadItem*)), this, SLOT(downloadFile(QWebEngineDownloadItem*)));
	}

	connect(this, SIGNAL(loadFinished(bool)), this, SLOT(pageLoadFinished()));
}

void QtWebEnginePage::pageLoadFinished()
{
	m_ignoreJavaScriptPopups = false;

	toHtml([&](const QString &result)
	{
		if (m_widget)
		{
			const QVector<int> profiles(ContentBlockingManager::getProfileList(m_widget->getOption(SettingsManager::ContentBlocking_ProfilesOption, url()).toStringList()));

			if (!profiles.isEmpty() && ContentBlockingManager::getCosmeticFiltersMode() != ContentBlockingManager::NoFiltersMode)
			{
				const ContentBlockingManager::CosmeticFiltersMode mode(ContentBlockingManager::checkUrl(profiles, url(), url(), NetworkManager::OtherType).comesticFiltersMode);
				QStringList styleSheetBlackList;
				QStringList styleSheetWhiteList;

				if (mode != ContentBlockingManager::NoFiltersMode)
				{
					if (mode != ContentBlockingManager::DomainOnlyFiltersMode)
					{
						styleSheetBlackList = ContentBlockingManager::getStyleSheet(profiles);
					}

					const QStringList domainList(ContentBlockingManager::createSubdomainList(url().host()));

					for (int i = 0; i < domainList.count(); ++i)
					{
						styleSheetBlackList += ContentBlockingManager::getStyleSheetBlackList(domainList.at(i), profiles);
						styleSheetWhiteList += ContentBlockingManager::getStyleSheetWhiteList(domainList.at(i), profiles);
					}
				}

				if (!styleSheetBlackList.isEmpty() || !styleSheetWhiteList.isEmpty())
				{
					QFile file(QLatin1String(":/modules/backends/web/qtwebengine/resources/hideElements.js"));

					if (file.open(QIODevice::ReadOnly))
					{
						runJavaScript(QString(file.readAll()).arg(createJavaScriptList(styleSheetWhiteList)).arg(createJavaScriptList(styleSheetBlackList)));

						file.close();
					}
				}
			}

			const QStringList blockedRequests(qobject_cast<QtWebEngineWebBackend*>(m_widget->getBackend())->getBlockedElements(url().host()));

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

		const QRegularExpressionMatch match(QRegularExpression(QStringLiteral(">(<img style=\"-webkit-user-select: none;(?: cursor: zoom-in;)?\"|<video controls=\"\" autoplay=\"\" name=\"media\"><source) src=\"%1").arg(QRegularExpression::escape(string))).match(result));
		const bool isViewingMedia(match.hasMatch());

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
	});
}

void QtWebEnginePage::removePopup(const QUrl &url)
{
	QtWebEnginePage *page(qobject_cast<QtWebEnginePage*>(sender()));

	if (page)
	{
		m_popups.removeAll(page);

		page->deleteLater();
	}

	emit requestedPopupWindow(requestedUrl(), url);
}

void QtWebEnginePage::markAsPopup()
{
	m_isPopup = true;
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

void QtWebEnginePage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &note, int line, const QString &source)
{
	Console::MessageLevel mappedLevel(Console::LogLevel);

	if (level == QWebEnginePage::WarningMessageLevel)
	{
		mappedLevel = Console::WarningLevel;
	}
	else if (level == QWebEnginePage::ErrorMessageLevel)
	{
		mappedLevel = Console::ErrorLevel;
	}

	Console::addMessage(note, Console::JavaScriptCategory, mappedLevel, source, line, (m_widget ? m_widget->getWindowIdentifier() : 0));
}

QWebEnginePage* QtWebEnginePage::createWindow(QWebEnginePage::WebWindowType type)
{
	if (type == QWebEnginePage::WebDialog)
	{
		QtWebEngineWebWidget *widget(nullptr);

		if (!m_widget || m_widget->getLastUrlClickTime().isNull() || m_widget->getLastUrlClickTime().secsTo(QDateTime::currentDateTime()) > 1)
		{
			const QString popupsPolicy(SettingsManager::getValue(SettingsManager::Content_PopupsPolicyOption, (m_widget ? m_widget->getRequestedUrl() : QUrl())).toString());

			if (popupsPolicy == QLatin1String("blockAll"))
			{
				return nullptr;
			}

			if (popupsPolicy == QLatin1String("ask"))
			{
				QtWebEnginePage *page(new QtWebEnginePage(false, nullptr));
				page->markAsPopup();

				connect(page, SIGNAL(aboutToNavigate(QUrl, QWebEnginePage::NavigationType)), this, SLOT(removePopup(QUrl)));

				return page;
			}
		}

		if (m_widget)
		{
			widget = qobject_cast<QtWebEngineWebWidget*>(m_widget->clone(false));
		}
		else
		{
			widget = new QtWebEngineWebWidget(false, nullptr, nullptr);
		}

		widget->pageLoadStarted();

		emit requestedNewWindow(widget, WindowsManager::calculateOpenHints(WindowsManager::NewTabOpen));

		return widget->getPage();
	}

	return QWebEnginePage::createWindow(type);
}

QString QtWebEnginePage::createJavaScriptList(QStringList rules) const
{
	if (rules.isEmpty())
	{
		return QString();
	}

	for (int i = 0; i < rules.count(); ++i)
	{
		rules[i] = rules[i].replace(QLatin1Char('\''), QLatin1String("\\'"));
	}

	return QStringLiteral("'%1'").arg(rules.join("','"));
}

QStringList QtWebEnginePage::chooseFiles(QWebEnginePage::FileSelectionMode mode, const QStringList &oldFiles, const QStringList &acceptedMimeTypes)
{
	Q_UNUSED(acceptedMimeTypes)

	return Utils::getOpenPaths(oldFiles, QStringList(), (mode == QWebEnginePage::FileSelectOpenMultiple));
}

bool QtWebEnginePage::acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool isMainFrame)
{
	if (m_isPopup)
	{
		emit aboutToNavigate(url, type);

		return false;
	}

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

	if (isMainFrame && type == QWebEnginePage::NavigationTypeReload && m_previousNavigationType == QWebEnginePage::NavigationTypeFormSubmitted && SettingsManager::getValue(SettingsManager::Choices_WarnFormResendOption).toBool())
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

	if (isMainFrame && type != QWebEnginePage::NavigationTypeReload)
	{
		m_previousNavigationType = type;
	}

	if (isMainFrame)
	{
		scripts().clear();

		const QList<UserScript*> scripts(AddonsManager::getUserScriptsForUrl(url));

		for (int i = 0; i < scripts.count(); ++i)
		{
			QWebEngineScript::InjectionPoint injectionPoint(QWebEngineScript::DocumentReady);

			if (scripts.at(i)->getInjectionTime() == UserScript::DocumentCreationTime)
			{
				injectionPoint = QWebEngineScript::DocumentCreation;
			}
			else if (scripts.at(i)->getInjectionTime() == UserScript::DeferredTime)
			{
				injectionPoint = QWebEngineScript::Deferred;
			}

			QWebEngineScript script;
			script.setSourceCode(scripts.at(i)->getSource());
			script.setRunsOnSubFrames(scripts.at(i)->shouldRunOnSubFrames());
			script.setInjectionPoint(injectionPoint);

			this->scripts().insert(script);
		}

		emit aboutToNavigate(url, type);
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

bool QtWebEnginePage::isViewingMedia() const
{
	return m_isViewingMedia;
}

}
