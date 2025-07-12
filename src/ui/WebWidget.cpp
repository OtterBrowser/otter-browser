/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2017 Piktas Zuikis <piktas.zuikis@inbox.lt>
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

#include "WebWidget.h"
#include "ContentsDialog.h"
#include "ContentsWidget.h"
#include "Menu.h"
#include "TransferDialog.h"
#include "Window.h"
#include "../core/BookmarksManager.h"
#include "../core/ContentFiltersManager.h"
#include "../core/HandlersManager.h"
#include "../core/HistoryManager.h"
#include "../core/IniSettings.h"
#include "../core/NotesManager.h"
#include "../core/SearchEnginesManager.h"
#include "../core/SettingsManager.h"
#include "../core/ThemesManager.h"
#include "../core/TransfersManager.h"
#include "../core/Utils.h"
#include "../core/WebBackend.h"

#include <QtCore/QDir>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtGui/QGuiApplication>
#include <QtGui/QClipboard>
#include <QtGui/QPainter>
#include <QtWidgets/QToolTip>

namespace Otter
{

QString WebWidget::m_fastForwardScript;

WebWidget::WebWidget(WebBackend *backend, ContentsWidget *parent) : QWidget(parent),
	m_parent(parent),
	m_toolTipParentWidget(nullptr),
	m_backend(backend),
	m_windowIdentifier(0),
	m_loadingTime(0),
	m_loadingTimer(0),
	m_reloadTimer(0),
	m_toolTipTimer(0),
	m_toolTipEntryEnumerator(metaObject()->indexOfEnumerator(QLatin1String("ToolTipEntry").data()))
{
	connect(this, &WebWidget::loadingStateChanged, this, [&](LoadingState state)
	{
		if (m_loadingTimer != 0)
		{
			killTimer(m_loadingTimer);

			m_loadingTimer = 0;
		}

		if (state == OngoingLoadingState)
		{
			m_loadingTime = 0;
			m_loadingTimer = startTimer(1000);

			emit pageInformationChanged(LoadingTimeInformation, 0);
		}
	});
	connect(BookmarksManager::getModel(), &BookmarksModel::modelModified, this, [&]()
	{
		emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::BookmarkCategory});
	});
	connect(PasswordsManager::getInstance(), &PasswordsManager::passwordsModified, this, [&]()
	{
		emit arbitraryActionsStateChanged({ActionsManager::FillPasswordAction});
	});
}

void WebWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_loadingTimer)
	{
		++m_loadingTime;

		emit pageInformationChanged(LoadingTimeInformation, m_loadingTime);
	}
	else if (event->timerId() == m_reloadTimer)
	{
		killTimer(m_reloadTimer);

		m_reloadTimer = 0;

		if (getLoadingState() == FinishedLoadingState)
		{
			triggerAction(ActionsManager::ReloadAction);
		}
	}
	else if (event->timerId() == m_toolTipTimer)
	{
		killTimer(m_toolTipTimer);

		m_toolTipTimer = 0;

		if (!m_toolTip.isEmpty())
		{
			QToolTip::showText(m_toolTipPosition, QStringLiteral("<div style=\"white-space:pre-line;\">%1</div>").arg(m_toolTip.join(QLatin1String("<br>"))), m_toolTipParentWidget, m_toolTipRectangle);
		}
	}
}

void WebWidget::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	Q_UNUSED(identifier)
	Q_UNUSED(parameters)
	Q_UNUSED(trigger)
}

void WebWidget::search(const QString &query, const QString &searchEngine)
{
	Q_UNUSED(query)
	Q_UNUSED(searchEngine)
}

void WebWidget::startWatchingChanges(QObject *object, ChangeWatcher watcher)
{
	if (!m_changeWatchers.contains(watcher))
	{
		m_changeWatchers[watcher] = {object};

		updateWatchedData(watcher);
	}
	else if (!m_changeWatchers[watcher].contains(object))
	{
		m_changeWatchers[watcher].append(object);
	}
	else
	{
		return;
	}

	connect(object, &QObject::destroyed, this, [=]()
	{
		stopWatchingChanges(object, watcher);
	});
}

void WebWidget::stopWatchingChanges(QObject *object, ChangeWatcher watcher)
{
	if (m_changeWatchers.contains(watcher))
	{
		m_changeWatchers[watcher].removeAll(object);
	}
}

void WebWidget::startReloadTimer()
{
	const int reloadTime(getOption(SettingsManager::Content_PageReloadTimeOption).toInt());

	if (reloadTime >= 0)
	{
		triggerAction(ActionsManager::StopScheduledReloadAction);

		if (reloadTime > 0)
		{
			m_reloadTimer = startTimer(reloadTime * 1000);
		}
	}
}

void WebWidget::startTransfer(Transfer *transfer)
{
	if (transfer->getState() == Transfer::CancelledState)
	{
		transfer->deleteLater();

		return;
	}

	const HandlersManager::MimeTypeHandlerDefinition handler(HandlersManager::getMimeTypeHandler(transfer->getMimeType()));

	switch (handler.transferMode)
	{
		case HandlersManager::MimeTypeHandlerDefinition::IgnoreTransfer:
			transfer->cancel();
			transfer->deleteLater();

			break;
		case HandlersManager::MimeTypeHandlerDefinition::AskTransfer:
			{
				TransferDialog *transferDialog(new TransferDialog(transfer, this));
				ContentsDialog *dialog(new ContentsDialog(ThemesManager::createIcon(QLatin1String("download")), transferDialog->windowTitle(), QString(), QString(), QDialogButtonBox::NoButton, transferDialog, this));

				connect(transferDialog, &TransferDialog::finished, dialog, &ContentsDialog::close);

				showDialog(dialog, false);
			}

			break;
		case HandlersManager::MimeTypeHandlerDefinition::OpenTransfer:
			transfer->setOpenCommand(handler.openCommand);

			TransfersManager::addTransfer(transfer);

			break;
		case HandlersManager::MimeTypeHandlerDefinition::SaveTransfer:
			transfer->setTarget(handler.downloadsPath + QDir::separator() + transfer->getSuggestedFileName());

			if (transfer->getState() == Transfer::CancelledState)
			{
				TransfersManager::addTransfer(transfer);
			}
			else
			{
				transfer->deleteLater();
			}

			break;
		case HandlersManager::MimeTypeHandlerDefinition::SaveAsTransfer:
			{
				const QString path(Utils::getSavePath(transfer->getSuggestedFileName(), handler.downloadsPath, {}, true).path);

				if (path.isEmpty())
				{
					transfer->cancel();
					transfer->deleteLater();

					return;
				}

				transfer->setTarget(path, true);

				TransfersManager::addTransfer(transfer);
			}

			break;
	}
}

void WebWidget::clearOptions()
{
	const QString host(Utils::extractHost(getUrl()));
	const QList<int> identifiers(m_options.keys());

	m_options.clear();

	for (int i = 0; i < identifiers.count(); ++i)
	{
		const int identifier(identifiers.at(i));

		emit optionChanged(identifier, SettingsManager::getOption(identifier, host));
	}

	emit arbitraryActionsStateChanged({ActionsManager::ResetQuickPreferencesAction});
}

void WebWidget::fillPassword(const PasswordsManager::PasswordInformation &password)
{
	Q_UNUSED(password)
}

void WebWidget::replaceMisspelledWord(const QString &replacement)
{
	Q_UNUSED(replacement)
}

void WebWidget::openUrl(const QUrl &url, SessionsManager::OpenHints hints)
{
	switch (hints)
	{
		case SessionsManager::DefaultOpen:
		case SessionsManager::CurrentTabOpen:
			setUrl(url, false);

			break;
		default:
			{
				WebWidget *widget(clone(false, hints.testFlag(SessionsManager::PrivateOpen), SettingsManager::getOption(SettingsManager::Sessions_OptionsExludedFromInheritingOption).toStringList()));
				widget->setRequestedUrl(url, false);

				emit requestedNewWindow(widget, hints, {});
			}

			break;
	}
}

void WebWidget::handleToolTipEvent(QHelpEvent *event, QWidget *widget)
{
	const HitTestResult hitResult(getHitTestResult(event->pos()));
	const QUrl link(hitResult.linkUrl.isValid() ? hitResult.linkUrl : hitResult.formUrl);

	setStatusMessageOverride(link.isEmpty() ? hitResult.title : link.toString());

	event->accept();

	if (!SettingsManager::getOption(SettingsManager::Interface_EnableToolTipsOption).toBool())
	{
		return;
	}

	const EnumeratorMapper enumeratorMapper(metaObject()->enumerator(m_toolTipEntryEnumerator), QLatin1String("Entry"));
	const QStringList rawLayout(SettingsManager::getOption(SettingsManager::Interface_ToolTipLayoutOption).toStringList());
	QHash<ToolTipEntry, QString> entries;
	QVector<ToolTipEntry> layout;
	layout.reserve(rawLayout.count());

	for (int i = 0; i < rawLayout.count(); ++i)
	{
		const ToolTipEntry entry(static_cast<ToolTipEntry>(enumeratorMapper.mapToValue(rawLayout.at(i))));

		if (entry > UnknownEntry)
		{
			layout.append(entry);
		}
	}

	layout.squeeze();

	if (!link.isEmpty())
	{
		if (layout.contains(LastVisitedEntry))
		{
			const QDateTime lastVisitTime(HistoryManager::getLastVisitTime(link));

			if (lastVisitTime.isValid())
			{
				entries[LastVisitedEntry] = Utils::formatDateTime(lastVisitTime);
			}
		}

		if (layout.contains(LinkEntry))
		{
			entries[LinkEntry] = link.toString();
		}
	}

	if (!hitResult.title.isEmpty())
	{
		entries[TitleEntry] = hitResult.title.toHtmlEscaped();
	}

	const QStringList previousToolTip(m_toolTip);

	m_toolTip.clear();

	if (entries.count() == 1 && entries.contains(TitleEntry))
	{
		m_toolTip.append(entries[TitleEntry]);
	}
	else
	{
		for (int i = 0; i < layout.count(); ++i)
		{
			const ToolTipEntry entry(layout.at(i));

			if (entries.contains(entry))
			{
				switch (entry)
				{
					case LastVisitedEntry:
						m_toolTip.append(tr("Last Visited: %1").arg(entries[LastVisitedEntry]));

						break;
					case LinkEntry:
						m_toolTip.append(tr("Address: %1").arg(entries[LinkEntry]));

						break;
					case TitleEntry:
						m_toolTip.append(tr("Title: %1").arg(entries[TitleEntry]));

						break;
					default:
						break;
				}
			}
		}
	}

	if (!m_toolTip.isEmpty() && previousToolTip != m_toolTip)
	{
		m_toolTipParentWidget = widget;
		m_toolTipRectangle = hitResult.elementGeometry;
		m_toolTipPosition = event->globalPos();
		m_toolTipTimer = startTimer(style()->styleHint(QStyle::SH_ToolTip_WakeUpDelay));
	}
	else if (m_toolTipTimer != 0 && m_toolTip.isEmpty())
	{
		killTimer(m_toolTipTimer);

		m_toolTipTimer = 0;
	}
}

void WebWidget::handleWindowCloseRequest()
{
	const QString host(Utils::extractHost(getUrl()));

	if (isPopup() && SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanCloseSelfOpenedWindowsOption, host).toBool())
	{
		emit requestedCloseWindow();

		return;
	}

	const QString mode(SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanCloseWindowsOption, host).toString());

	if (mode != QLatin1String("ask"))
	{
		if (mode == QLatin1String("allow"))
		{
			emit requestedCloseWindow();
		}

		return;
	}

	ContentsDialog *dialog(new ContentsDialog(ThemesManager::createIcon(QLatin1String("dialog-warning")), tr("JavaScript"), tr("Webpage wants to close this tab, do you want to allow to close it?"), QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), nullptr, this));
	dialog->setCheckBox(tr("Do not show this message again"), false);

	connect(this, &WebWidget::aboutToReload, dialog, &ContentsDialog::close);
	connect(dialog, &ContentsDialog::finished, dialog, [&](int result, bool isChecked)
	{
		const bool isAccepted(result == QDialog::Accepted);

		if (isChecked)
		{
			SettingsManager::setOption(SettingsManager::Permissions_ScriptsCanCloseWindowsOption, (isAccepted ? QLatin1String("allow") : QLatin1String("disallow")));
		}

		if (isAccepted)
		{
			emit requestedCloseWindow();
		}
	});

	showDialog(dialog, false);
}

void WebWidget::notifyRedoActionStateChanged()
{
	emit arbitraryActionsStateChanged({ActionsManager::RedoAction});
}

void WebWidget::notifyUndoActionStateChanged()
{
	emit arbitraryActionsStateChanged({ActionsManager::UndoAction});
}

void WebWidget::updateHitTestResult(const QPoint &position)
{
	m_hitResult = getHitTestResult(position);
}

void WebWidget::updateWatchedData(ChangeWatcher watcher)
{
	Q_UNUSED(watcher)
}

void WebWidget::showDialog(ContentsDialog *dialog, bool lockEventLoop)
{
	if (m_parent)
	{
		m_parent->showDialog(dialog, lockEventLoop);
	}
}

void WebWidget::showContextMenu(const QPoint &position)
{
	const bool hasSelection(this->hasSelection() && !getSelectedText().trimmed().isEmpty());

	if (position.isNull() && (!hasSelection || m_clickPosition.isNull()))
	{
		return;
	}

	const QPoint hitPosition(position.isNull() ? m_clickPosition : position);

	if (isScrollBar(hitPosition) || !canShowContextMenu(hitPosition))
	{
		return;
	}

	updateHitTestResult(hitPosition);

	emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::EditingCategory});

	QStringList includeSections;
	includeSections.reserve(1);

	if (m_hitResult.flags.testFlag(HitTestResult::IsFormTest))
	{
		includeSections.append(QLatin1String("form"));
	}

	if (hasSelection && m_hitResult.flags.testFlag(HitTestResult::IsSelectedTest) && !m_hitResult.imageUrl.isValid())
	{
		includeSections.append(QLatin1String("selection"));
	}

	if (m_hitResult.linkUrl.isValid())
	{
		if (m_hitResult.linkUrl.scheme() == QLatin1String("mailto"))
		{
			includeSections.append(QLatin1String("mail"));
		}
		else
		{
			includeSections.append(QLatin1String("link"));
		}
	}

	if (m_hitResult.imageUrl.isValid())
	{
		includeSections.append(QLatin1String("image"));
	}

	if (m_hitResult.mediaUrl.isValid())
	{
		includeSections.append(QLatin1String("media"));
	}

	if (m_hitResult.flags.testFlag(HitTestResult::IsContentEditableTest))
	{
		includeSections.append(QLatin1String("edit"));
	}

	if (includeSections.isEmpty() || (includeSections.count() == 1 && includeSections.value(0) == QLatin1String("form")))
	{
		includeSections.append(QLatin1String("standard"));

		if (m_hitResult.frameUrl.isValid())
		{
			includeSections.append(QLatin1String("frame"));
		}
	}

	ActionExecutor::Object executor(this, this);

	if (m_parent)
	{
		if (m_parent->getWindow())
		{
			executor = ActionExecutor::Object(m_parent->getWindow(), m_parent->getWindow());
		}
		else
		{
			executor = ActionExecutor::Object(m_parent, m_parent);
		}
	}

	Menu menu(this);
	menu.load(QLatin1String("menu/webWidget.json"), includeSections, executor);
	menu.exec(mapToGlobal(hitPosition));
}

void WebWidget::setParent(QWidget *parent)
{
	QWidget::setParent(parent);

	ContentsWidget *contentsWidget(qobject_cast<ContentsWidget*>(parent));

	if (contentsWidget)
	{
		m_parent = contentsWidget;
	}
}

void WebWidget::setActiveStyleSheet(const QString &styleSheet)
{
	Q_UNUSED(styleSheet)
}

void WebWidget::setClickPosition(const QPoint &position)
{
	m_clickPosition = position;
}

void WebWidget::setStatusMessage(const QString &message)
{
	const QString previousMessage(getStatusMessage());

	m_statusMessage = message;

	const QString currentMessage(getStatusMessage());

	if (currentMessage != previousMessage)
	{
		emit statusMessageChanged(currentMessage);
	}
}

void WebWidget::setStatusMessageOverride(const QString &message)
{
	const QString previousMessage(getStatusMessage());

	m_statusMessageOverride = message;

	const QString currentMessage(getStatusMessage());

	if (currentMessage != previousMessage)
	{
		emit statusMessageChanged(currentMessage);
	}
}

void WebWidget::setPermission(FeaturePermission feature, const QUrl &url, PermissionPolicies policies)
{
	if (policies.testFlag(KeepAskingForPermission))
	{
		return;
	}

	const QString value(policies.testFlag(GrantedPermission) ? QLatin1String("allow") : QLatin1String("disallow"));
	const QString host(Utils::extractHost(url));

	switch (feature)
	{
		case FullScreenFeature:
			SettingsManager::setOption(SettingsManager::Permissions_EnableFullScreenOption, value, host);

			return;
		case GeolocationFeature:
			SettingsManager::setOption(SettingsManager::Permissions_EnableGeolocationOption, value, host);

			return;
		case NotificationsFeature:
			SettingsManager::setOption(SettingsManager::Permissions_EnableNotificationsOption, value, host);

			return;
		case PointerLockFeature:
			SettingsManager::setOption(SettingsManager::Permissions_EnablePointerLockOption, value, host);

			return;
		case CaptureAudioFeature:
			SettingsManager::setOption(SettingsManager::Permissions_EnableMediaCaptureAudioOption, value, host);

			return;
		case CaptureVideoFeature:
			SettingsManager::setOption(SettingsManager::Permissions_EnableMediaCaptureVideoOption, value, host);

			return;
		case CaptureAudioVideoFeature:
			SettingsManager::setOption(SettingsManager::Permissions_EnableMediaCaptureAudioOption, value, host);
			SettingsManager::setOption(SettingsManager::Permissions_EnableMediaCaptureVideoOption, value, host);

			return;
		case PlaybackAudioFeature:
			SettingsManager::setOption(SettingsManager::Permissions_EnableMediaPlaybackAudioOption, value, host);

			return;
		default:
			return;
	}
}

void WebWidget::setOption(int identifier, const QVariant &value)
{
	if (value == m_options.value(identifier))
	{
		return;
	}

	if (value.isNull())
	{
		m_options.remove(identifier);
	}
	else
	{
		m_options[identifier] = value;
	}

	SessionsManager::markSessionAsModified();

	switch (identifier)
	{
		case SettingsManager::Content_PageReloadTimeOption:
			{
				const int reloadTime(value.toInt());

				emit arbitraryActionsStateChanged({ActionsManager::ResetQuickPreferencesAction});
				emit optionChanged(identifier, (value.isNull() ? getOption(identifier) : value));

				if (m_reloadTimer != 0)
				{
					killTimer(m_reloadTimer);

					m_reloadTimer = 0;
				}

				if (reloadTime >= 0)
				{
					triggerAction(ActionsManager::StopScheduledReloadAction);

					if (reloadTime > 0)
					{
						m_reloadTimer = startTimer(reloadTime * 1000);
					}
				}
			}

			break;
		case SettingsManager::Network_EnableReferrerOption:
			emit arbitraryActionsStateChanged({ActionsManager::EnableReferrerAction});

			break;
		case SettingsManager::Permissions_EnableJavaScriptOption:
			emit arbitraryActionsStateChanged({ActionsManager::EnableJavaScriptAction});

			break;
		default:
			break;
	}
}

void WebWidget::setOptions(const QHash<int, QVariant> &options, const QStringList &excludedOptions)
{
	QList<int> identifiers(m_options.keys());
	identifiers.reserve(identifiers.count() + options.count());

	QHash<int, QVariant>::const_iterator iterator;

	for (iterator = options.begin(); iterator != options.end(); ++iterator)
	{
		if (!identifiers.contains(iterator.key()))
		{
			identifiers.append(iterator.key());
		}
	}

	m_options = options;

	for (int i = 0; i < excludedOptions.count(); ++i)
	{
		const int identifier(SettingsManager::getOptionIdentifier(excludedOptions.at(i)));

		if (identifier >= 0 && m_options.contains(identifier))
		{
			m_options.remove(identifier);
		}
	}

	const QString host(Utils::extractHost(getUrl()));

	for (int i = 0; i < identifiers.count(); ++i)
	{
		if (m_options.contains(identifiers.at(i)))
		{
			emit optionChanged(identifiers.at(i), m_options[identifiers.at(i)]);
		}
		else
		{
			emit optionChanged(identifiers.at(i), SettingsManager::getOption(identifiers.at(i), host));
		}
	}

	emit arbitraryActionsStateChanged({ActionsManager::ResetQuickPreferencesAction});
}

void WebWidget::setRequestedUrl(const QUrl &url, bool isTypedIn, bool updateOnly)
{
	m_requestedUrl = url;

	if (updateOnly)
	{
		emit urlChanged(url);
	}
	else
	{
		setUrl(url, isTypedIn);
	}
}

void WebWidget::setWindowIdentifier(quint64 identifier)
{
	m_windowIdentifier = identifier;
}

QWidget* WebWidget::getInspector()
{
	return nullptr;
}

QWidget* WebWidget::getViewport()
{
	return this;
}

WebBackend* WebWidget::getBackend() const
{
	return m_backend;
}

QString WebWidget::getDescription() const
{
	return {};
}

QString WebWidget::suggestSaveFileName(const QString &extension) const
{
	const QUrl url(getUrl());
	QString fileName(url.fileName());

	if (fileName.isEmpty() && !url.path().isEmpty() && url.path() != QLatin1String("/"))
	{
		fileName = QDir(url.path()).dirName();
	}

	if (fileName.isEmpty() && !url.host().isEmpty())
	{
		fileName = url.host() + extension;
	}

	if (fileName.isEmpty())
	{
		fileName = QLatin1String("file") + extension;
	}

	if (!extension.isEmpty() && !fileName.contains(QLatin1Char('.')))
	{
		fileName.append(extension);
	}

	return fileName;
}

QString WebWidget::suggestSaveFileName(SaveFormat format) const
{
	switch (format)
	{
		case MhtmlSaveFormat:
			return suggestSaveFileName(QLatin1String(".mht"));
		case PdfSaveFormat:
			return suggestSaveFileName(QLatin1String(".pdf"));
		case SingleFileSaveFormat:
			return suggestSaveFileName(QLatin1String(".html"));
		default:
			break;
	}

	return suggestSaveFileName({});
}

QString WebWidget::getSavePath(const QVector<SaveFormat> &allowedFormats, SaveFormat *selectedFormat) const
{
	const QMap<SaveFormat, QString> formats({{SingleFileSaveFormat, tr("HTML file (*.html *.htm)")}, {CompletePageSaveFormat, tr("HTML file with all resources (*.html *.htm)")}, {MhtmlSaveFormat, tr("Web archive (*.mht)")}, {PdfSaveFormat, tr("PDF document (*.pdf)")}});
	QStringList filters;
	filters.reserve(allowedFormats.count());

	for (int i = 0; i < allowedFormats.count(); ++i)
	{
		filters.append(formats.value(allowedFormats.at(i)));
	}

	const SaveInformation result(Utils::getSavePath(suggestSaveFileName(SingleFileSaveFormat), {}, filters));

	if (!result.canSave)
	{
		return {};
	}

	*selectedFormat = formats.key(result.filter);

	return result.path;
}

QString WebWidget::getOpenActionText(SessionsManager::OpenHints hints) const
{
	if (hints == SessionsManager::CurrentTabOpen)
	{
		return QCoreApplication::translate("actions", "Open in This Tab");
	}

	if (hints == SessionsManager::NewTabOpen)
	{
		return QCoreApplication::translate("actions", "Open in New Tab");
	}

	if (hints == (SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen))
	{
		return QCoreApplication::translate("actions", "Open in New Background Tab");
	}

	if (hints == SessionsManager::NewWindowOpen)
	{
		return QCoreApplication::translate("actions", "Open in New Window");
	}

	if (hints == (SessionsManager::NewWindowOpen | SessionsManager::BackgroundOpen))
	{
		return QCoreApplication::translate("actions", "Open in New Background Window");
	}

	if (hints == (SessionsManager::NewTabOpen | SessionsManager::PrivateOpen))
	{
		return QCoreApplication::translate("actions", "Open in New Private Tab");
	}

	if (hints == (SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen | SessionsManager::PrivateOpen))
	{
		return QCoreApplication::translate("actions", "Open in New Private Background Tab");
	}

	if (hints == (SessionsManager::NewWindowOpen | SessionsManager::PrivateOpen))
	{
		return QCoreApplication::translate("actions", "Open in New Private Window");
	}

	if (hints == (SessionsManager::NewWindowOpen | SessionsManager::BackgroundOpen | SessionsManager::PrivateOpen))
	{
		return QCoreApplication::translate("actions", "Open in New Private Background Window");
	}

	return QCoreApplication::translate("actions", "Open");
}

QString WebWidget::getFastForwardScript(bool isSelectingTheBestLink)
{
	QString script(m_fastForwardScript);

	if (m_fastForwardScript.isEmpty())
	{
		IniSettings settings(SessionsManager::getReadableDataPath(QLatin1String("fastforward.ini")));
		QFile file(SessionsManager::getReadableDataPath(QLatin1String("fastforward.js")));

		if (!file.open(QIODevice::ReadOnly))
		{
			return {};
		}

		script = QString::fromLatin1(file.readAll());

		file.close();

		const QStringList categories({QLatin1String("Href"), QLatin1String("Class"), QLatin1String("Id"), QLatin1String("Text")});

		for (int i = 0; i < categories.count(); ++i)
		{
			const QString category(categories.at(i));

			settings.beginGroup(category);

			const QStringList keys(settings.getKeys());
			QJsonArray tokensArray;

			for (int j = 0; j < keys.count(); ++j)
			{
				const QString key(keys.at(j));

				tokensArray.append(QJsonObject({{QLatin1String("value"), key.toUpper()}, {QLatin1String("score"), settings.getValue(key).toInt()}}));
			}

			settings.endGroup();

			script.replace(QLatin1Char('{') + category.toLower() + QLatin1String("Tokens}"), QString::fromUtf8(QJsonDocument(tokensArray).toJson(QJsonDocument::Compact)));
		}

		m_fastForwardScript = script;
	}

	return script.replace(QLatin1String("{isSelectingTheBestLink}"), (isSelectingTheBestLink ? QLatin1String("true") : QLatin1String("false")));
}

QString WebWidget::getActiveStyleSheet() const
{
	return {};
}

QString WebWidget::getCharacterEncoding() const
{
	return {};
}

QString WebWidget::getMisspelledWord() const
{
	return {};
}

QString WebWidget::getSelectedText() const
{
	return {};
}

QString WebWidget::getStatusMessage() const
{
	return (m_statusMessageOverride.isEmpty() ? m_statusMessage : m_statusMessageOverride);
}

QVariant WebWidget::getOption(int identifier, const QUrl &url) const
{
	if (m_options.contains(identifier))
	{
		return m_options[identifier];
	}

	return SettingsManager::getOption(identifier, Utils::extractHost(url.isEmpty() ? getUrl() : url));
}

QVariant WebWidget::getPageInformation(PageInformation key) const
{
	if (key == LoadingTimeInformation)
	{
		return m_loadingTime;
	}

	return {};
}

QUrl WebWidget::extractUrl(const QVariantMap &parameters) const
{
	if (parameters.contains(QLatin1String("url")))
	{
		return parameters[QLatin1String("url")].toUrl();
	}

	return m_hitResult.linkUrl;
}

QUrl WebWidget::getRequestedUrl() const
{
	return ((getUrl().isEmpty() || getLoadingState() == OngoingLoadingState) ? m_requestedUrl : getUrl());
}

QPixmap WebWidget::createThumbnail(const QSize &size)
{
	const QString host(Utils::extractHost(getUrl()));
	const QString text(host.mid(0, 1).toUpper());
	QCryptographicHash hash(QCryptographicHash::Md5);
	hash.addData(host.toUtf8());

	QPixmap pixmap(size.isValid() ? size : getDefaultThumbnailSize());
	pixmap.setDevicePixelRatio(devicePixelRatio());
	pixmap.fill(Qt::white);

	QPainter painter(&pixmap);
	QFont font(painter.font());
	font.setPixelSize((pixmap.rect().height() / 3) * 2);

	painter.setFont(font);
	painter.setRenderHints(QPainter::Antialiasing);
	painter.setPen(QPen(QColor(Qt::gray)));
	painter.drawText(pixmap.rect().adjusted(0, 1, 1, 0), Qt::AlignCenter, text);
	painter.setPen(QPen(QColor(QLatin1Char('#') + QString::fromLatin1(hash.result().toHex()).mid(0, 6))));
	painter.drawText(pixmap.rect(), Qt::AlignCenter, text);
	painter.end();

	return pixmap;
}

QPoint WebWidget::getClickPosition() const
{
	return m_clickPosition;
}

QSize WebWidget::getDefaultThumbnailSize()
{
	return QSize(260, 170);
}

QRect WebWidget::getGeometry(bool excludeScrollBars) const
{
	Q_UNUSED(excludeScrollBars)

	return geometry();
}

ActionsManager::ActionDefinition::State WebWidget::getActionState(int identifier, const QVariantMap &parameters) const
{
	ActionsManager::ActionDefinition::State state(ActionsManager::getActionDefinition(identifier).getDefaultState());

	switch (identifier)
	{
		case ActionsManager::ClearTabHistoryAction:
			state.isEnabled = !getHistory().isEmpty();

			if (parameters.value(QLatin1String("clearGlobalHistory"), false).toBool())
			{
				state.text = QCoreApplication::translate("actions", "Purge Tab History");
			}

			break;
		case ActionsManager::PurgeTabHistoryAction:
			state.isEnabled = !getHistory().isEmpty();

			break;
		case ActionsManager::OpenLinkAction:
		case ActionsManager::OpenLinkInCurrentTabAction:
		case ActionsManager::OpenLinkInNewTabAction:
		case ActionsManager::OpenLinkInNewTabBackgroundAction:
		case ActionsManager::OpenLinkInNewWindowAction:
		case ActionsManager::OpenLinkInNewWindowBackgroundAction:
		case ActionsManager::OpenLinkInNewPrivateTabAction:
		case ActionsManager::OpenLinkInNewPrivateTabBackgroundAction:
		case ActionsManager::OpenLinkInNewPrivateWindowAction:
		case ActionsManager::OpenLinkInNewPrivateWindowBackgroundAction:
		case ActionsManager::CopyLinkToClipboardAction:
		case ActionsManager::BookmarkLinkAction:
		case ActionsManager::ShowLinkAsQuickResponseCodeAction:
		case ActionsManager::SaveLinkToDiskAction:
		case ActionsManager::SaveLinkToDownloadsAction:
			{
				const QUrl url(extractUrl(parameters));

				state.isEnabled = url.isValid();

				switch (identifier)
				{
					case ActionsManager::BookmarkLinkAction:
						state.text = (BookmarksManager::hasBookmark(url) ? QCoreApplication::translate("actions", "Edit Link Bookmark…") : QCoreApplication::translate("actions", "Bookmark Link…"));

						break;
					case ActionsManager::OpenLinkAction:
						if (parameters.contains(QLatin1String("hints")))
						{
							const SessionsManager::OpenHints hints(SessionsManager::calculateOpenHints(parameters));

							state.text = getOpenActionText(hints);

							if (hints != SessionsManager::DefaultOpen)
							{
								state.icon = {};
							}
						}

						break;
				}
			}

			break;
		case ActionsManager::OpenFrameAction:
		case ActionsManager::OpenFrameInCurrentTabAction:
		case ActionsManager::OpenFrameInNewTabAction:
		case ActionsManager::OpenFrameInNewTabBackgroundAction:
		case ActionsManager::CopyFrameLinkToClipboardAction:
		case ActionsManager::ReloadFrameAction:
		case ActionsManager::ViewFrameSourceAction:
			state.isEnabled = m_hitResult.frameUrl.isValid();

			if (identifier == ActionsManager::OpenFrameAction && parameters.contains(QLatin1String("hints")))
			{
				state.text = getOpenActionText(SessionsManager::calculateOpenHints(parameters));
			}

			break;
		case ActionsManager::OpenImageAction:
		case ActionsManager::OpenImageInNewTabAction:
		case ActionsManager::OpenImageInNewTabBackgroundAction:
			if (m_hitResult.imageUrl.isValid())
			{
				const QString fileName((m_hitResult.imageUrl.scheme() == QLatin1String("data")) ? QString() : fontMetrics().elidedText(m_hitResult.imageUrl.fileName(), Qt::ElideMiddle, 256));

				if (identifier == ActionsManager::OpenImageAction)
				{
					switch (SessionsManager::calculateOpenHints(parameters))
					{
						case SessionsManager::CurrentTabOpen:
							state.text = QCoreApplication::translate("actions", "Open Image in This Tab");

							break;
						case SessionsManager::NewTabOpen:
							state.text = QCoreApplication::translate("actions", "Open Image in New Tab");

							break;
						case (SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen):
							state.text = QCoreApplication::translate("actions", "Open Image in New Background Tab");

							break;
						case SessionsManager::NewWindowOpen:
							state.text = QCoreApplication::translate("actions", "Open Image in New Window");

							break;
						case (SessionsManager::NewWindowOpen | SessionsManager::BackgroundOpen):
							state.text = QCoreApplication::translate("actions", "Open Image in New Background Window");

							break;
						case (SessionsManager::NewTabOpen | SessionsManager::PrivateOpen):
							state.text = QCoreApplication::translate("actions", "Open Image in New Private Tab");

							break;
						case (SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen | SessionsManager::PrivateOpen):
							state.text = QCoreApplication::translate("actions", "Open Image in New Private Background Tab");

							break;
						case (SessionsManager::NewWindowOpen | SessionsManager::PrivateOpen):
							state.text = QCoreApplication::translate("actions", "Open Image in New Private Window");

							break;
						case (SessionsManager::NewWindowOpen | SessionsManager::BackgroundOpen | SessionsManager::PrivateOpen):
							state.text = QCoreApplication::translate("actions", "Open Image in New Private Background Window");

							break;
						default:
							break;
					}

					if (!fileName.isEmpty())
					{
						state.text.append(QLatin1String(" (") + fileName + QLatin1Char(')'));
					}
				}
				else if (!fileName.isEmpty())
				{
					if (identifier == ActionsManager::OpenImageInNewTabBackgroundAction)
					{
						state.text = tr("Open Image in New Background Tab (%1)").arg(fileName);
					}
					else
					{
						state.text = tr("Open Image in New Tab (%1)").arg(fileName);
					}
				}

				state.isEnabled = !getUrl().matches(m_hitResult.imageUrl, (QUrl::NormalizePathSegments | QUrl::RemoveFragment | QUrl::StripTrailingSlash));
			}
			else
			{
				state.isEnabled = false;
			}

			break;
		case ActionsManager::SaveImageToDiskAction:
		case ActionsManager::CopyImageToClipboardAction:
		case ActionsManager::CopyImageUrlToClipboardAction:
		case ActionsManager::ReloadImageAction:
		case ActionsManager::ImagePropertiesAction:
			state.isEnabled = m_hitResult.imageUrl.isValid();

			break;
		case ActionsManager::SaveMediaToDiskAction:
			state.text = ((m_hitResult.tagName == QLatin1String("video")) ? QCoreApplication::translate("actions", "Save Video…") : QCoreApplication::translate("actions", "Save Audio…"));
			state.isEnabled = m_hitResult.mediaUrl.isValid();

			break;
		case ActionsManager::CopyMediaUrlToClipboardAction:
			state.text = ((m_hitResult.tagName == QLatin1String("video")) ? QCoreApplication::translate("actions", "Copy Video Link to Clipboard") : QCoreApplication::translate("actions", "Copy Audio Link to Clipboard"));
			state.isEnabled = m_hitResult.mediaUrl.isValid();

			break;
		case ActionsManager::MediaControlsAction:
			state.isChecked = m_hitResult.flags.testFlag(HitTestResult::MediaHasControlsTest);
			state.isEnabled = m_hitResult.mediaUrl.isValid();

			break;
		case ActionsManager::MediaLoopAction:
			state.isChecked = m_hitResult.flags.testFlag(HitTestResult::MediaIsLoopedTest);
			state.isEnabled = m_hitResult.mediaUrl.isValid();

			break;
		case ActionsManager::MediaPlayPauseAction:
			state.text = (m_hitResult.flags.testFlag(HitTestResult::MediaIsPausedTest) ? QCoreApplication::translate("actions", "Play") : QCoreApplication::translate("actions", "Pause"));
			state.icon = ThemesManager::createIcon(m_hitResult.flags.testFlag(HitTestResult::MediaIsPausedTest) ? QLatin1String("media-playback-start") : QLatin1String("media-playback-pause"));
			state.isEnabled = m_hitResult.mediaUrl.isValid();

			break;
		case ActionsManager::MediaMuteAction:
			state.text = (m_hitResult.flags.testFlag(HitTestResult::MediaIsMutedTest) ? QCoreApplication::translate("actions", "Unmute") : QCoreApplication::translate("actions", "Mute"));
			state.icon = ThemesManager::createIcon(m_hitResult.flags.testFlag(HitTestResult::MediaIsMutedTest) ? QLatin1String("audio-volume-medium") : QLatin1String("audio-volume-muted"));
			state.isEnabled = m_hitResult.mediaUrl.isValid();

			break;
		case ActionsManager::MediaPlaybackRateAction:
			{
				const qreal rate(parameters.value(QLatin1String("rate")).toReal());

				state.text = tr("Playback Rate: %1x").arg(QLocale().toString(rate));
				state.isChecked = qFuzzyCompare(rate, m_hitResult.playbackRate);
				state.isEnabled = m_hitResult.mediaUrl.isValid();
			}

			break;
		case ActionsManager::FillPasswordAction:
			state.isEnabled = (!Utils::isUrlEmpty(getUrl()) && PasswordsManager::hasPasswords(getUrl(), PasswordsManager::FormPassword));

			break;
		case ActionsManager::MuteTabMediaAction:
			state.icon = ThemesManager::createIcon(isAudioMuted() ? QLatin1String("audio-volume-muted") : QLatin1String("audio-volume-medium"));
			state.text = (isAudioMuted() ? QCoreApplication::translate("actions", "Unmute Tab Media") : QCoreApplication::translate("actions", "Mute Tab Media"));

			break;
		case ActionsManager::GoBackAction:
		case ActionsManager::RewindAction:
			state.isEnabled = canGoBack();

			break;
		case ActionsManager::GoForwardAction:
			state.isEnabled = canGoForward();

			break;
		case ActionsManager::GoToHistoryIndexAction:
			if (parameters.contains(QLatin1String("index")))
			{
				const Session::Window::History history(getHistory());
				const int index(parameters[QLatin1String("index")].toInt());

				if (index >= 0 && index < history.entries.count())
				{
					state.icon = HistoryManager::getIcon(QUrl(history.entries.at(index).url));
					state.text = history.entries.at(index).getTitle().replace(QLatin1Char('&'), QLatin1String("&&"));
					state.isEnabled = true;
				}
			}

			break;
		case ActionsManager::FastForwardAction:
			state.isEnabled = canFastForward();

			break;
		case ActionsManager::RemoveHistoryIndexAction:
			{
				const int index(parameters.contains(QLatin1String("index")) ? parameters[QLatin1String("index")].toInt() : -1);

				if (parameters.value(QLatin1String("clearGlobalHistory"), false).toBool())
				{
					state.text = QCoreApplication::translate("actions", "Purge History Entry");
					state.isEnabled = (m_backend->getCapabilityScopes(WebBackend::HistoryMetaDataCapability).testFlag(WebBackend::TabScope) && getGlobalHistoryEntryIdentifier(index) > 0);
				}
				else if (index >= 0 && index < getHistory().entries.count())
				{
					state.isEnabled = true;
				}
			}

			break;
		case ActionsManager::StopAction:
			state.isEnabled = (getLoadingState() == OngoingLoadingState);

			break;
		case ActionsManager::ReloadAction:
			state.isEnabled = (getLoadingState() != OngoingLoadingState);

			break;
		case ActionsManager::ReloadOrStopAction:
			state = getActionState((getLoadingState() == OngoingLoadingState) ? ActionsManager::StopAction : ActionsManager::ReloadAction);

			break;
		case ActionsManager::ScheduleReloadAction:
			if (!parameters.contains(QLatin1String("time")) || (parameters.contains(QLatin1String("time")) && parameters[QLatin1String("time")].type() == QVariant::String && parameters[QLatin1String("time")].toString() == QLatin1String("custom")))
			{
				state.isChecked = (m_options.contains(SettingsManager::Content_PageReloadTimeOption) && !QVector<int>({-1, 0, 60, 1800, 3600, 7200}).contains(m_options[SettingsManager::Content_PageReloadTimeOption].toInt()));
			}
			else if (parameters.contains(QLatin1String("time")) && parameters[QLatin1String("time")].type() != QVariant::String)
			{
				const int reloadTime(parameters[QLatin1String("time")].toInt());

				if (reloadTime < 0)
				{
					state.isChecked = (!m_options.contains(SettingsManager::Content_PageReloadTimeOption) || m_options[SettingsManager::Content_PageReloadTimeOption].toInt() < 0);
					state.text = tr("Page Default");
				}
				else
				{
					state.isChecked = (m_options.contains(SettingsManager::Content_PageReloadTimeOption) && reloadTime == m_options[SettingsManager::Content_PageReloadTimeOption].toInt());

					if (reloadTime == 0)
					{
						state.text = tr("Never Reload");
					}
					else
					{
						state.text = tr("Reload Every: %n second(s)", "", reloadTime);
					}
				}
			}

			break;
		case ActionsManager::UndoAction:
			state.isEnabled = canUndo();

			break;
		case ActionsManager::RedoAction:
			state.isEnabled = canRedo();

			break;
		case ActionsManager::CutAction:
			state.isEnabled = (hasSelection() && !getSelectedText().trimmed().isEmpty() && m_hitResult.flags.testFlag(HitTestResult::IsContentEditableTest));

			break;
		case ActionsManager::CopyAction:
		case ActionsManager::CopyPlainTextAction:
		case ActionsManager::CopyToNoteAction:
		case ActionsManager::UnselectAction:
			state.isEnabled = (hasSelection() && !getSelectedText().trimmed().isEmpty());

			break;
		case ActionsManager::PasteAction:
			if (m_hitResult.flags.testFlag(HitTestResult::IsContentEditableTest))
			{
				if (parameters.contains(QLatin1String("note")))
				{
					const BookmarksModel::Bookmark *bookmark(NotesManager::getModel()->getBookmark(parameters[QLatin1String("note")].toULongLong()));

					if (bookmark)
					{
						state.statusTip = bookmark->getUrl().toString();
						state.toolTip = bookmark->getDescription();
						state.text = bookmark->getTitle();
						state.icon = bookmark->getIcon();
						state.isEnabled = true;
					}
				}
				else
				{
					const QClipboard *clipboard(QGuiApplication::clipboard());

					state.isEnabled = (parameters.contains(QLatin1String("text")) || (clipboard->mimeData() && clipboard->mimeData()->hasText()));
				}
			}

			break;
		case ActionsManager::PasteAndGoAction:
			{
				const QClipboard *clipboard(QGuiApplication::clipboard());

				state.isEnabled = (clipboard->mimeData() && clipboard->mimeData()->hasText());
			}

			break;
		case ActionsManager::DeleteAction:
			state.isEnabled = (m_hitResult.flags.testFlag(HitTestResult::IsContentEditableTest) && !m_hitResult.flags.testFlag(HitTestResult::IsEmptyTest) && hasSelection() && !getSelectedText().trimmed().isEmpty());

			break;
		case ActionsManager::SelectAllAction:
			state.isEnabled = !m_hitResult.flags.testFlag(HitTestResult::IsEmptyTest);

			break;
		case ActionsManager::ClearAllAction:
			state.isEnabled = (m_hitResult.flags.testFlag(HitTestResult::IsContentEditableTest) && !m_hitResult.flags.testFlag(HitTestResult::IsEmptyTest));

			break;
		case ActionsManager::CheckSpellingAction:
			{
				state.isEnabled = (getOption(SettingsManager::Browser_EnableSpellCheckOption, getUrl()).toBool() && !getDictionaries().isEmpty());

				if (parameters.contains(QLatin1String("dictionary")))
				{
					const QString language(parameters[QLatin1String("dictionary")].toString());
					const SpellCheckManager::DictionaryInformation dictionary(SpellCheckManager::getDictionary(language));
					QStringList activeDictionaries;

					if (getOption(SettingsManager::Browser_SpellCheckDictionaryOption).isNull())
					{
						activeDictionaries = QStringList(SpellCheckManager::getDefaultDictionary());
					}
					else
					{
						activeDictionaries = getOption(SettingsManager::Browser_SpellCheckDictionaryOption).toStringList();
					}

					state.text = language;
					state.isChecked = activeDictionaries.contains(language);

					if (dictionary.isValid())
					{
						state.text = dictionary.title;
					}
				}
				else
				{
					state.isChecked = m_hitResult.flags.testFlag(HitTestResult::IsSpellCheckEnabledTest);
				}
			}

			break;
		case ActionsManager::SearchAction:
			{
				const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(parameters.contains(QLatin1String("searchEngine")) ? parameters[QLatin1String("searchEngine")].toString() : getOption(SettingsManager::Search_DefaultQuickSearchEngineOption).toString()));

				state.text = (searchEngine.isValid() ? searchEngine.title : QCoreApplication::translate("actions", "Search"));
				state.icon = (searchEngine.icon.isNull() ? ThemesManager::createIcon(QLatin1String("edit-find")) : searchEngine.icon);
				state.isEnabled = searchEngine.isValid();
			}

			break;
		case ActionsManager::CreateSearchAction:
			state.isEnabled = m_hitResult.flags.testFlag(HitTestResult::IsFormTest);

			break;
		case ActionsManager::TakeScreenshotAction:
			state.isEnabled = canTakeScreenshot();

			break;
		case ActionsManager::BookmarkPageAction:
			{
				const QUrl url(getUrl());

				state.text = (BookmarksManager::hasBookmark(url) ? QCoreApplication::translate("actions", "Edit Bookmark…") : QCoreApplication::translate("actions", "Add Bookmark…"));
				state.isEnabled = !Utils::isUrlEmpty(url);
			}

			break;
		case ActionsManager::LoadPluginsAction:
			state.isEnabled = (getAmountOfDeferredPlugins() > 0);

			break;
		case ActionsManager::EnableJavaScriptAction:
			state.isChecked = getOption(SettingsManager::Permissions_EnableJavaScriptOption, getUrl()).toBool();

			break;
		case ActionsManager::EnableReferrerAction:
			state.isChecked = getOption(SettingsManager::Network_EnableReferrerOption, getUrl()).toBool();

			break;
		case ActionsManager::ViewSourceAction:
			state.isEnabled = canViewSource();

			break;
		case ActionsManager::InspectPageAction:
			state.isChecked = isInspecting();
			state.isEnabled = canInspect();

			break;
		case ActionsManager::InspectElementAction:
			state.isEnabled = canInspect();

			break;
		case ActionsManager::WebsitePreferencesAction:
			state.isEnabled = (!Utils::isUrlEmpty(getUrl()) && getUrl().scheme() != QLatin1String("about"));

			break;
		case ActionsManager::ResetQuickPreferencesAction:
			state.isEnabled = !m_options.isEmpty();

			break;
		default:
			break;
	}

	return state;
}

WebWidget::LinkUrl WebWidget::getActiveFrame() const
{
	return {};
}

WebWidget::LinkUrl WebWidget::getActiveImage() const
{
	return {};
}

WebWidget::LinkUrl WebWidget::getActiveLink() const
{
	return {};
}

WebWidget::LinkUrl WebWidget::getActiveMedia() const
{
	return {};
}

WebWidget::SslInformation WebWidget::getSslInformation() const
{
	return {};
}

QStringList WebWidget::getSpellCheckerSuggestions() const
{
	return {};
}

QStringList WebWidget::getStyleSheets() const
{
	return {};
}

QVector<SpellCheckManager::DictionaryInformation> WebWidget::getDictionaries() const
{
	return SpellCheckManager::getDictionaries();
}

QVector<WebWidget::LinkUrl> WebWidget::getFeeds() const
{
	return {};
}

QVector<WebWidget::LinkUrl> WebWidget::getLinks() const
{
	return {};
}

QVector<WebWidget::LinkUrl> WebWidget::getSearchEngines() const
{
	return {};
}

QVector<NetworkManager::ResourceInformation> WebWidget::getBlockedRequests() const
{
	return {};
}

QHash<int, QVariant> WebWidget::getOptions() const
{
	return m_options;
}

QMap<QByteArray, QByteArray> WebWidget::getHeaders() const
{
	return {};
}

QMultiMap<QString, QString> WebWidget::getMetaData() const
{
	return {};
}

WebWidget::HitTestResult WebWidget::getCurrentHitTestResult() const
{
	return m_hitResult;
}

WebWidget::HitTestResult WebWidget::getHitTestResult(const QPoint &position)
{
	Q_UNUSED(position)

	return {};
}

WebWidget::ContentStates WebWidget::getContentState() const
{
	const QUrl url(getUrl());

	if (url.isEmpty() || url.scheme() == QLatin1String("about"))
	{
		return ApplicationContentState;
	}

	if (url.scheme() == QLatin1String("file"))
	{
		return LocalContentState;
	}

	ContentStates state(RemoteContentState);

	if (getOption(SettingsManager::Security_EnableFraudCheckingOption, url).toBool() && ContentFiltersManager::isFraud(url))
	{
		state |= FraudContentState;
	}

	if (Utils::isUrlAmbiguous(url))
	{
		state |= AmbiguousContentState;
	}

	return state;
}

WebWidget::PermissionPolicy WebWidget::getPermission(FeaturePermission feature, const QUrl &url) const
{
	if (!url.isValid())
	{
		return DeniedPermission;
	}

	QString value;

	switch (feature)
	{
		case FullScreenFeature:
			value = getOption(SettingsManager::Permissions_EnableFullScreenOption, url).toString();

			break;
		case GeolocationFeature:
			value = getOption(SettingsManager::Permissions_EnableGeolocationOption, url).toString();

			break;
		case NotificationsFeature:
			value = getOption(SettingsManager::Permissions_EnableNotificationsOption, url).toString();

			break;
		case PointerLockFeature:
			value = getOption(SettingsManager::Permissions_EnablePointerLockOption, url).toString();

			break;
		case CaptureAudioFeature:
			value = getOption(SettingsManager::Permissions_EnableMediaCaptureAudioOption, url).toString();

			break;
		case CaptureVideoFeature:
			value = getOption(SettingsManager::Permissions_EnableMediaCaptureVideoOption, url).toString();

			break;
		case CaptureAudioVideoFeature:
			{
				const QString captureAudioValue(getOption(SettingsManager::Permissions_EnableMediaCaptureAudioOption, url).toString());
				const QString captureVideoValue(getOption(SettingsManager::Permissions_EnableMediaCaptureVideoOption, url).toString());

				if (captureAudioValue == QLatin1String("allow") && captureVideoValue == QLatin1String("allow"))
				{
					return GrantedPermission;
				}

				if (captureAudioValue == QLatin1String("disallow") || captureVideoValue == QLatin1String("disallow"))
				{
					return DeniedPermission;
				}
			}

			return KeepAskingForPermission;
		case PlaybackAudioFeature:
			value = getOption(SettingsManager::Permissions_EnableMediaPlaybackAudioOption, url).toString();

			break;
		default:
			return DeniedPermission;
	}

	if (value == QLatin1String("allow"))
	{
		return GrantedPermission;
	}

	if (value == QLatin1String("disallow"))
	{
		return DeniedPermission;
	}

	return KeepAskingForPermission;
}

SessionsManager::OpenHints WebWidget::mapOpenActionToOpenHints(int identifier)
{
	switch (identifier)
	{
		case ActionsManager::OpenLinkInCurrentTabAction:
		case ActionsManager::OpenFrameInCurrentTabAction:
			return SessionsManager::CurrentTabOpen;
		case ActionsManager::OpenLinkInNewTabAction:
		case ActionsManager::OpenFrameInNewTabAction:
		case ActionsManager::OpenImageInNewTabAction:
			return SessionsManager::calculateOpenHints(SessionsManager::NewTabOpen);
		case ActionsManager::OpenLinkInNewTabBackgroundAction:
		case ActionsManager::OpenFrameInNewTabBackgroundAction:
		case ActionsManager::OpenImageInNewTabBackgroundAction:
			return (SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen);
		case ActionsManager::OpenLinkInNewWindowAction:
			return SessionsManager::calculateOpenHints(SessionsManager::NewWindowOpen);
		case ActionsManager::OpenLinkInNewWindowBackgroundAction:
			return (SessionsManager::NewWindowOpen | SessionsManager::BackgroundOpen);
		case ActionsManager::OpenLinkInNewPrivateTabAction:
			return (SessionsManager::NewTabOpen | SessionsManager::PrivateOpen);
		case ActionsManager::OpenLinkInNewPrivateTabBackgroundAction:
			return (SessionsManager::NewTabOpen | SessionsManager::BackgroundOpen | SessionsManager::PrivateOpen);
		case ActionsManager::OpenLinkInNewPrivateWindowAction:
			return (SessionsManager::NewWindowOpen | SessionsManager::PrivateOpen);
		case ActionsManager::OpenLinkInNewPrivateWindowBackgroundAction:
			return (SessionsManager::NewWindowOpen | SessionsManager::BackgroundOpen | SessionsManager::PrivateOpen);
		default:
			break;
	}

	return SessionsManager::DefaultOpen;
}

quint64 WebWidget::getWindowIdentifier() const
{
	return m_windowIdentifier;
}

quint64 WebWidget::getGlobalHistoryEntryIdentifier(int index) const
{
	Q_UNUSED(index)

	return 0;
}

int WebWidget::getAmountOfDeferredPlugins() const
{
	return 0;
}

bool WebWidget::canGoBack() const
{
	return false;
}

bool WebWidget::canGoForward() const
{
	return false;
}

bool WebWidget::canFastForward() const
{
	return false;
}

bool WebWidget::canInspect() const
{
	return false;
}

bool WebWidget::canTakeScreenshot() const
{
	return false;
}

bool WebWidget::canRedo() const
{
	return false;
}

bool WebWidget::canUndo() const
{
	return false;
}

bool WebWidget::canShowContextMenu(const QPoint &position) const
{
	Q_UNUSED(position)

	return true;
}

bool WebWidget::canViewSource() const
{
	return true;
}

bool WebWidget::hasOption(int identifier) const
{
	return m_options.contains(identifier);
}

bool WebWidget::hasSelection() const
{
	return false;
}

bool WebWidget::hasWatchedChanges(ChangeWatcher watcher) const
{
	Q_UNUSED(watcher)

	return false;
}

bool WebWidget::isAudible() const
{
	return false;
}

bool WebWidget::isAudioMuted() const
{
	return false;
}

bool WebWidget::isFullScreen() const
{
	return false;
}

bool WebWidget::isInspecting() const
{
	return false;
}

bool WebWidget::isPopup() const
{
	return false;
}

bool WebWidget::isScrollBar(const QPoint &position) const
{
	Q_UNUSED(position)

	return false;
}

bool WebWidget::isWatchingChanges(ChangeWatcher watcher) const
{
	return m_changeWatchers.contains(watcher);
}

}
