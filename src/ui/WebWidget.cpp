/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../core/Application.h"
#include "../core/BookmarksManager.h"
#include "../core/HandlersManager.h"
#include "../core/IniSettings.h"
#include "../core/SearchEnginesManager.h"
#include "../core/SettingsManager.h"
#include "../core/ThemesManager.h"
#include "../core/TransfersManager.h"
#include "../core/Utils.h"

#include <QtCore/QDir>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtGui/QClipboard>
#include <QtWidgets/QToolTip>

namespace Otter
{

QString WebWidget::m_fastForwardScript;

WebWidget::WebWidget(const QVariantMap &parameters, WebBackend *backend, ContentsWidget *parent) : QWidget(parent), ActionExecutor(),
	m_parent(parent),
	m_backend(backend),
	m_windowIdentifier(0),
	m_loadingTime(0),
	m_loadingTimer(0),
	m_reloadTimer(0)
{
	Q_UNUSED(parameters)

	connect(this, SIGNAL(loadingStateChanged(WebWidget::LoadingState)), this, SLOT(handleLoadingStateChange(WebWidget::LoadingState)));
	connect(BookmarksManager::getModel(), SIGNAL(modelModified()), this, SLOT(notifyPageActionsChanged()));
	connect(PasswordsManager::getInstance(), SIGNAL(passwordsModified()), this, SLOT(notifyFillPasswordActionStateChanged()));
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
}

void WebWidget::triggerAction(int identifier, const QVariantMap &parameters)
{
	Q_UNUSED(identifier)
	Q_UNUSED(parameters)
}

void WebWidget::search(const QString &query, const QString &searchEngine)
{
	Q_UNUSED(query)
	Q_UNUSED(searchEngine)
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

	const HandlersManager::HandlerDefinition handler(HandlersManager::getHandler(transfer->getMimeType().name()));

	switch (handler.transferMode)
	{
		case HandlersManager::HandlerDefinition::IgnoreTransfer:
			transfer->cancel();
			transfer->deleteLater();

			break;
		case HandlersManager::HandlerDefinition::AskTransfer:
			{
				TransferDialog *transferDialog(new TransferDialog(transfer, this));
				ContentsDialog *dialog(new ContentsDialog(ThemesManager::createIcon(QLatin1String("download")), transferDialog->windowTitle(), QString(), QString(), QDialogButtonBox::NoButton, transferDialog, this));

				connect(transferDialog, SIGNAL(finished(int)), dialog, SLOT(close()));

				showDialog(dialog, false);
			}

			break;
		case HandlersManager::HandlerDefinition::OpenTransfer:
			transfer->setOpenCommand(handler.openCommand);

			TransfersManager::addTransfer(transfer);

			break;
		case HandlersManager::HandlerDefinition::SaveTransfer:
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
		case HandlersManager::HandlerDefinition::SaveAsTransfer:
			{
				const QString path(Utils::getSavePath(transfer->getSuggestedFileName(), handler.downloadsPath, QStringList(), true).path);

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
	const QUrl url(getUrl());
	const QList<int> identifiers(m_options.keys());

	m_options.clear();

	for (int i = 0; i < identifiers.count(); ++i)
	{
		emit optionChanged(identifiers.at(i), SettingsManager::getOption(identifiers.at(i), url));
	}

	emit actionsStateChanged(QVector<int>({ActionsManager::ResetQuickPreferencesAction}));
}

void WebWidget::fillPassword(const PasswordsManager::PasswordInformation &password)
{
	Q_UNUSED(password)
}

void WebWidget::openUrl(const QUrl &url, SessionsManager::OpenHints hints)
{
	WebWidget *widget(clone(false, hints.testFlag(SessionsManager::PrivateOpen), SettingsManager::getOption(SettingsManager::Sessions_OptionsExludedFromInheritingOption).toStringList()));
	widget->setRequestedUrl(url, false);

	emit requestedNewWindow(widget, hints);
}

void WebWidget::handleLoadingStateChange(LoadingState state)
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
}

void WebWidget::handleToolTipEvent(QHelpEvent *event, QWidget *widget)
{
	const HitTestResult hitResult(getHitTestResult(event->pos()));
	const QString toolTipsMode(SettingsManager::getOption(SettingsManager::Browser_ToolTipsModeOption).toString());
	const QString link((hitResult.linkUrl.isValid() ? hitResult.linkUrl : hitResult.formUrl).toString());
	QString text;

	if (toolTipsMode != QLatin1String("disabled"))
	{
		const QString title(QString(hitResult.title).replace(QLatin1Char('&'), QLatin1String("&amp;")).replace(QLatin1Char('<'), QLatin1String("&lt;")).replace(QLatin1Char('>'), QLatin1String("&gt;")));

		if (toolTipsMode == QLatin1String("extended"))
		{
			if (!link.isEmpty())
			{
				text = (title.isEmpty() ? QString() : tr("Title: %1").arg(title) + QLatin1String("<br>")) + tr("Address: %1").arg(link);
			}
			else if (!title.isEmpty())
			{
				text = title;
			}
		}
		else
		{
			text = title;
		}
	}

	setStatusMessage((link.isEmpty() ? hitResult.title : link), true);

	if (!text.isEmpty())
	{
		QToolTip::showText(event->globalPos(), QStringLiteral("<div style=\"white-space:pre-line;\">%1</div>").arg(text), widget);
	}

	event->accept();
}

void WebWidget::handleWindowCloseRequest()
{
	if (isPopup() && SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanCloseSelfOpenedWindowsOption, getUrl()).toBool())
	{
		emit requestedCloseWindow();

		return;
	}

	const QString mode(SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanCloseWindowsOption, getUrl()).toString());

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

	connect(this, SIGNAL(aboutToReload()), dialog, SLOT(close()));
	connect(dialog, &ContentsDialog::finished, [&](int result, bool isChecked)
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

void WebWidget::notifyFillPasswordActionStateChanged()
{
	emit actionsStateChanged(QVector<int>(ActionsManager::FillPasswordAction));
}

void WebWidget::notifyRedoActionStateChanged()
{
	emit actionsStateChanged(QVector<int>({ActionsManager::RedoAction}));
}

void WebWidget::notifyUndoActionStateChanged()
{
	emit actionsStateChanged(QVector<int>({ActionsManager::UndoAction}));
}

void WebWidget::notifyPageActionsChanged()
{
	emit actionsStateChanged(ActionsManager::ActionDefinition::BookmarkCategory);
}

void WebWidget::updateHitTestResult(const QPoint &position)
{
	m_hitResult = getHitTestResult(position);
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

	emit actionsStateChanged(ActionsManager::ActionDefinition::EditingCategory);

	QStringList includeSections;

	if (m_hitResult.flags.testFlag(HitTestResult::IsFormTest))
	{
		includeSections.append(QLatin1String("form"));
	}

	if (!m_hitResult.imageUrl.isValid() && m_hitResult.flags.testFlag(HitTestResult::IsSelectedTest) && hasSelection)
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

	if (!m_hitResult.imageUrl.isEmpty())
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

	if (includeSections.isEmpty() || (includeSections.count() == 1 && includeSections.first() == QLatin1String("form")))
	{
		includeSections.append(QLatin1String("standard"));

		if (m_hitResult.frameUrl.isValid())
		{
			includeSections.append(QLatin1String("frame"));
		}
	}

	if (includeSections.isEmpty())
	{
		return;
	}

	ActionExecutor::Object executor;

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
	else
	{
		executor = ActionExecutor::Object(this, this);
	}

	Menu menu(Menu::NoMenuRole, this);
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

void WebWidget::setStatusMessage(const QString &message, bool override)
{
	const QString previousMessage(getStatusMessage());

	if (override)
	{
		m_overridingStatusMessage = message;
	}
	else
	{
		m_javaScriptStatusMessage = message;
	}

	const QString currentMessage(getStatusMessage());

	if (currentMessage != previousMessage)
	{
		emit statusMessageChanged(currentMessage);
	}
}

void WebWidget::setPermission(FeaturePermission feature, const QUrl &url, PermissionPolicies policies)
{
	if (policies.testFlag(KeepAskingPermission))
	{
		return;
	}

	const QString value(policies.testFlag(GrantedPermission) ? QLatin1String("allow") : QLatin1String("disallow"));

	switch (feature)
	{
		case FullScreenFeature:
			SettingsManager::setOption(SettingsManager::Permissions_EnableFullScreenOption, value, url);

			return;
		case GeolocationFeature:
			SettingsManager::setOption(SettingsManager::Permissions_EnableGeolocationOption, value, url);

			return;
		case NotificationsFeature:
			SettingsManager::setOption(SettingsManager::Permissions_EnableNotificationsOption, value, url);

			return;
		case PointerLockFeature:
			SettingsManager::setOption(SettingsManager::Permissions_EnablePointerLockOption, value, url);

			return;
		case CaptureAudioFeature:
			SettingsManager::setOption(SettingsManager::Permissions_EnableMediaCaptureAudioOption, value, url);

			return;
		case CaptureVideoFeature:
			SettingsManager::setOption(SettingsManager::Permissions_EnableMediaCaptureVideoOption, value, url);

			return;
		case CaptureAudioVideoFeature:
			SettingsManager::setOption(SettingsManager::Permissions_EnableMediaCaptureAudioOption, value, url);
			SettingsManager::setOption(SettingsManager::Permissions_EnableMediaCaptureVideoOption, value, url);

			return;
		case PlaybackAudioFeature:
			SettingsManager::setOption(SettingsManager::Permissions_EnableMediaPlaybackAudioOption, value, url);

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

	SessionsManager::markSessionModified();

	switch (identifier)
	{
		case SettingsManager::Content_PageReloadTimeOption:
			{
				const int reloadTime(value.toInt());

				emit actionsStateChanged(QVector<int>({ActionsManager::ResetQuickPreferencesAction}));
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
			emit actionsStateChanged(QVector<int>({ActionsManager::EnableReferrerAction}));

			break;
		case SettingsManager::Permissions_EnableJavaScriptOption:
			emit actionsStateChanged(QVector<int>({ActionsManager::EnableJavaScriptAction}));

			break;
		default:
			break;
	}
}

void WebWidget::setOptions(const QHash<int, QVariant> &options, const QStringList &excludedOptions)
{
	const QList<int> identifiers((m_options.keys() + options.keys()).toSet().toList());

	m_options = options;

	for (int i = 0; i < excludedOptions.count(); ++i)
	{
		const int identifier(SettingsManager::getOptionIdentifier(excludedOptions.at(i)));

		if (identifier >= 0 && m_options.contains(identifier))
		{
			m_options.remove(identifier);
		}
	}

	const QUrl url(getUrl());

	for (int i = 0; i < identifiers.count(); ++i)
	{
		if (m_options.contains(identifiers.at(i)))
		{
			emit optionChanged(identifiers.at(i), m_options[identifiers.at(i)]);
		}
		else
		{
			emit optionChanged(identifiers.at(i), SettingsManager::getOption(identifiers.at(i), url));
		}
	}

	emit actionsStateChanged(QVector<int>({ActionsManager::ResetQuickPreferencesAction}));
}

void WebWidget::setRequestedUrl(const QUrl &url, bool isTyped, bool onlyUpdate)
{
	m_requestedUrl = url;

	if (!onlyUpdate)
	{
		setUrl(url, isTyped);
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
	return QString();
}

QString WebWidget::suggestSaveFileName(SaveFormat format) const
{
	const QUrl url(getUrl());
	QString fileName(url.fileName());
	QString extension;

	switch (format)
	{
		case MhtmlSaveFormat:
			extension = QLatin1String(".mht");

			break;
		case SingleHtmlFileSaveFormat:
			extension = QLatin1String(".html");

			break;
		default:
			break;
	}

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

	if (!fileName.contains(QLatin1Char('.')) && !extension.isEmpty())
	{
		fileName.append(extension);
	}

	return fileName;
}

QString WebWidget::getFastForwardScript(bool isSelectingTheBestLink)
{
	if (m_fastForwardScript.isEmpty())
	{
		IniSettings settings(SessionsManager::getReadableDataPath(QLatin1String("fastforward.ini")));
		QFile file(SessionsManager::getReadableDataPath(QLatin1String("fastforward.js")));

		if (!file.open(QIODevice::ReadOnly))
		{
			return QString();
		}

		QString script(file.readAll());

		file.close();

		const QStringList categories({QLatin1String("Href"), QLatin1String("Class"), QLatin1String("Id"), QLatin1String("Text")});

		for (int i = 0; i < categories.count(); ++i)
		{
			settings.beginGroup(categories.at(i));

			const QStringList keys(settings.getKeys());
			QJsonArray tokensArray;

			for (int j = 0; j < keys.length(); ++j)
			{
				QJsonObject tokenObject;
				tokenObject.insert(QLatin1Literal("value"), keys.at(j).toUpper());
				tokenObject.insert(QLatin1Literal("score"), settings.getValue(keys.at(j)).toInt());

				tokensArray.append(tokenObject);
			}

			settings.endGroup();

			script.replace(QStringLiteral("{%1Tokens}").arg(categories.at(i).toLower()), QString::fromUtf8(QJsonDocument(tokensArray).toJson(QJsonDocument::Compact)));
		}

		m_fastForwardScript = script;
	}

	return QString(m_fastForwardScript).replace(QLatin1String("{isSelectingTheBestLink}"), QLatin1String(isSelectingTheBestLink ? "true" : "false"));
}

QString WebWidget::getActiveStyleSheet() const
{
	return QString();
}

QString WebWidget::getCharacterEncoding() const
{
	return QString();
}

QString WebWidget::getSelectedText() const
{
	return QString();
}

QString WebWidget::getStatusMessage() const
{
	return (m_overridingStatusMessage.isEmpty() ? m_javaScriptStatusMessage : m_overridingStatusMessage);
}

QVariant WebWidget::getOption(int identifier, const QUrl &url) const
{
	if (m_options.contains(identifier))
	{
		return m_options[identifier];
	}

	return SettingsManager::getOption(identifier, (url.isEmpty() ? getUrl() : url));
}

QVariant WebWidget::getPageInformation(PageInformation key) const
{
	if (key == LoadingTimeInformation)
	{
		return m_loadingTime;
	}

	return QVariant();
}

QUrl WebWidget::getRequestedUrl() const
{
	return ((getUrl().isEmpty() || getLoadingState() == OngoingLoadingState) ? m_requestedUrl : getUrl());
}

QPoint WebWidget::getClickPosition() const
{
	return m_clickPosition;
}

QRect WebWidget::getProgressBarGeometry() const
{
	return (isVisible() ? QRect(QPoint(0, (height() - 30)), QSize(width(), 30)) : QRect());
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
				state.text = QT_TRANSLATE_NOOP("actions", "Purge Tab History");
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
		case ActionsManager::SaveLinkToDiskAction:
		case ActionsManager::SaveLinkToDownloadsAction:
			state.isEnabled = m_hitResult.linkUrl.isValid();

			break;
		case ActionsManager::BookmarkLinkAction:
			state.text = (BookmarksManager::hasBookmark(m_hitResult.linkUrl) ? QT_TRANSLATE_NOOP("actions", "Edit Link Bookmark…") : QT_TRANSLATE_NOOP("actions", "Bookmark Link…"));
			state.isEnabled = m_hitResult.linkUrl.isValid();

			break;
		case ActionsManager::OpenFrameInCurrentTabAction:
		case ActionsManager::OpenFrameInNewTabAction:
		case ActionsManager::OpenFrameInNewTabBackgroundAction:
		case ActionsManager::CopyFrameLinkToClipboardAction:
		case ActionsManager::ReloadFrameAction:
		case ActionsManager::ViewFrameSourceAction:
			state.isEnabled = m_hitResult.frameUrl.isValid();

			break;
		case ActionsManager::OpenImageInNewTabAction:
		case ActionsManager::OpenImageInNewTabBackgroundAction:
			if (m_hitResult.imageUrl.isEmpty())
			{
				state.isEnabled = false;
			}
			else
			{
				const QString fileName((m_hitResult.imageUrl.scheme() == QLatin1String("data")) ? QString() : fontMetrics().elidedText(m_hitResult.imageUrl.fileName(), Qt::ElideMiddle, 256));

				if (!fileName.isEmpty())
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

			break;
		case ActionsManager::SaveImageToDiskAction:
		case ActionsManager::CopyImageToClipboardAction:
		case ActionsManager::CopyImageUrlToClipboardAction:
		case ActionsManager::ReloadImageAction:
		case ActionsManager::ImagePropertiesAction:
			state.isEnabled = !m_hitResult.imageUrl.isEmpty();

			break;
		case ActionsManager::SaveMediaToDiskAction:
			state.text = ((m_hitResult.tagName == QLatin1String("video")) ? QT_TRANSLATE_NOOP("actions", "Save Video…") : QT_TRANSLATE_NOOP("actions", "Save Audio…"));
			state.isEnabled = m_hitResult.mediaUrl.isValid();

			break;
		case ActionsManager::CopyMediaUrlToClipboardAction:
			state.text = ((m_hitResult.tagName == QLatin1String("video")) ? QT_TRANSLATE_NOOP("actions", "Copy Video Link to Clipboard") : QT_TRANSLATE_NOOP("actions", "Copy Audio Link to Clipboard"));
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
			state.text = (m_hitResult.flags.testFlag(HitTestResult::MediaIsPausedTest) ? QT_TRANSLATE_NOOP("actions", "Play") : QT_TRANSLATE_NOOP("actions", "Pause"));
			state.icon = ThemesManager::createIcon(m_hitResult.flags.testFlag(HitTestResult::MediaIsPausedTest) ? QLatin1String("media-playback-start") : QLatin1String("media-playback-pause"));
			state.isEnabled = m_hitResult.mediaUrl.isValid();

			break;
		case ActionsManager::MediaMuteAction:
			state.text = (m_hitResult.flags.testFlag(HitTestResult::MediaIsMutedTest) ? QT_TRANSLATE_NOOP("actions", "Unmute") : QT_TRANSLATE_NOOP("actions", "Mute"));
			state.icon = ThemesManager::createIcon(m_hitResult.flags.testFlag(HitTestResult::MediaIsMutedTest) ? QLatin1String("audio-volume-medium") : QLatin1String("audio-volume-muted"));
			state.isEnabled = m_hitResult.mediaUrl.isValid();

			break;
		case ActionsManager::MediaPlaybackRateAction:
			{
				const qreal rate(parameters.value(QLatin1String("rate")).toReal());

				state.text = tr("Playback Rate: %1x").arg(QLocale().toString(rate));
				state.isChecked = (rate == m_hitResult.playbackRate);
				state.isEnabled = m_hitResult.mediaUrl.isValid();
			}

			break;
		case ActionsManager::FillPasswordAction:
			state.isEnabled = (!Utils::isUrlEmpty(getUrl()) && PasswordsManager::hasPasswords(getUrl(), PasswordsManager::FormPassword));

			break;
		case ActionsManager::MuteTabMediaAction:
			state.icon = ThemesManager::createIcon(isAudioMuted() ? QLatin1String("audio-volume-muted") : QLatin1String("audio-volume-medium"));
			state.text = (isAudioMuted() ? QT_TRANSLATE_NOOP("actions", "Unmute Tab Media") : QT_TRANSLATE_NOOP("actions", "Mute Tab Media"));

			break;
		case ActionsManager::GoBackAction:
		case ActionsManager::RewindAction:
			state.isEnabled = canGoBack();

			break;
		case ActionsManager::GoForwardAction:
			state.isEnabled = canGoForward();

			break;
		case ActionsManager::FastForwardAction:
			state.isEnabled = canFastForward();

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
					state.text = tr("Page Defaults");
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
						state.text = tr("Reload Every: %n seconds", "", reloadTime);
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
			state.isEnabled = (this->hasSelection() && !getSelectedText().trimmed().isEmpty() && m_hitResult.flags.testFlag(HitTestResult::IsContentEditableTest));

			break;
		case ActionsManager::CopyAction:
		case ActionsManager::CopyPlainTextAction:
		case ActionsManager::CopyToNoteAction:
		case ActionsManager::UnselectAction:
			state.isEnabled = (this->hasSelection() && !getSelectedText().trimmed().isEmpty());

			break;
		case ActionsManager::PasteAction:
			state.isEnabled = (m_hitResult.flags.testFlag(HitTestResult::IsContentEditableTest) && (parameters.contains(QLatin1String("note")) || parameters.contains(QLatin1String("text")) || (QApplication::clipboard()->mimeData() && QApplication::clipboard()->mimeData()->hasText())));

			break;
		case ActionsManager::PasteAndGoAction:
			state.isEnabled = (QApplication::clipboard()->mimeData() && QApplication::clipboard()->mimeData()->hasText());

			break;
		case ActionsManager::DeleteAction:
			state.isEnabled = (m_hitResult.flags.testFlag(HitTestResult::IsContentEditableTest) && !m_hitResult.flags.testFlag(HitTestResult::IsEmptyTest));

			break;
		case ActionsManager::SelectAllAction:
			state.isEnabled = (!m_hitResult.flags.testFlag(HitTestResult::IsEmptyTest));

			break;
		case ActionsManager::ClearAllAction:
			state.isEnabled = (m_hitResult.flags.testFlag(HitTestResult::IsContentEditableTest) && !m_hitResult.flags.testFlag(HitTestResult::IsEmptyTest));

			break;
		case ActionsManager::CheckSpellingAction:
			state.isChecked = (m_hitResult.flags.testFlag(HitTestResult::IsSpellCheckEnabled));
			state.isEnabled = (getOption(SettingsManager::Browser_EnableSpellCheckOption, getUrl()).toBool() && !getDictionaries().isEmpty());

			break;
		case ActionsManager::SearchAction:
			{
				const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(parameters.contains(QLatin1String("searchEngine")) ? parameters[QLatin1String("searchEngine")].toString() : getOption(SettingsManager::Search_DefaultQuickSearchEngineOption).toString()));

				state.text = (searchEngine.isValid() ? searchEngine.title : QT_TRANSLATE_NOOP("actions", "Search"));
				state.icon = (searchEngine.icon.isNull() ? ThemesManager::createIcon(QLatin1String("edit-find")) : searchEngine.icon);
				state.isEnabled = searchEngine.isValid();
			}

			break;
		case ActionsManager::CreateSearchAction:
			state.isEnabled = m_hitResult.flags.testFlag(HitTestResult::IsFormTest);

			break;
		case ActionsManager::BookmarkPageAction:
			state.text = (BookmarksManager::hasBookmark(getUrl()) ? QT_TRANSLATE_NOOP("actions", "Edit Bookmark…") : QT_TRANSLATE_NOOP("actions", "Add Bookmark…"));

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
	return LinkUrl();
}

WebWidget::LinkUrl WebWidget::getActiveImage() const
{
	return LinkUrl();
}

WebWidget::LinkUrl WebWidget::getActiveLink() const
{
	return LinkUrl();
}

WebWidget::SslInformation WebWidget::getSslInformation() const
{
	return SslInformation();
}

QStringList WebWidget::getStyleSheets() const
{
	return QStringList();
}

QVector<SpellCheckManager::DictionaryInformation> WebWidget::getDictionaries() const
{
	return SpellCheckManager::getDictionaries();
}

QVector<WebWidget::LinkUrl> WebWidget::getFeeds() const
{
	return QVector<LinkUrl>();
}

QVector<WebWidget::LinkUrl> WebWidget::getSearchEngines() const
{
	return QVector<LinkUrl>();
}

QVector<NetworkManager::ResourceInformation> WebWidget::getBlockedRequests() const
{
	return QVector<NetworkManager::ResourceInformation>();
}

QHash<int, QVariant> WebWidget::getOptions() const
{
	return m_options;
}

QHash<QByteArray, QByteArray> WebWidget::getHeaders() const
{
	return QHash<QByteArray, QByteArray>();
}

WebWidget::HitTestResult WebWidget::getCurrentHitTestResult() const
{
	return m_hitResult;
}

WebWidget::HitTestResult WebWidget::getHitTestResult(const QPoint &position)
{
	Q_UNUSED(position)

	return HitTestResult();
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

	return RemoteContentState;
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
				const QString valueCaptureAudio(getOption(SettingsManager::Permissions_EnableMediaCaptureAudioOption, url).toString());
				const QString valueCaptureVideo(getOption(SettingsManager::Permissions_EnableMediaCaptureVideoOption, url).toString());

				if (valueCaptureAudio == QLatin1String("allow") && valueCaptureVideo == QLatin1String("allow"))
				{
					value = QLatin1String("allow");
				}
				else if (valueCaptureAudio == QLatin1String("disallow") || valueCaptureVideo == QLatin1String("disallow"))
				{
					value = QLatin1String("disallow");
				}
				else
				{
					value = QLatin1String("ask");
				}
			}

			break;
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

	return KeepAskingPermission;
}

WebWidget::SaveFormats WebWidget::getSupportedSaveFormats() const
{
	return SingleHtmlFileSaveFormat;
}

quint64 WebWidget::getWindowIdentifier() const
{
	return m_windowIdentifier;
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

bool WebWidget::hasOption(int identifier) const
{
	return m_options.contains(identifier);
}

bool WebWidget::hasSelection() const
{
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

}
