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
#include "Action.h"
#include "ContentsDialog.h"
#include "ContentsWidget.h"
#include "Menu.h"
#include "TransferDialog.h"
#include "Window.h"
#include "../core/Application.h"
#include "../core/BookmarksManager.h"
#include "../core/HandlersManager.h"
#include "../core/IniSettings.h"
#include "../core/NotesManager.h"
#include "../core/SearchEnginesManager.h"
#include "../core/SettingsManager.h"
#include "../core/ThemesManager.h"
#include "../core/TransfersManager.h"
#include "../core/Utils.h"

#include <QtCore/QDir>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QMimeDatabase>
#include <QtGui/QClipboard>
#include <QtGui/QContextMenuEvent>
#include <QtWidgets/QToolTip>

namespace Otter
{

QString WebWidget::m_fastForwardScript;

WebWidget::WebWidget(bool isPrivate, WebBackend *backend, ContentsWidget *parent) : QWidget(parent),
	m_parent(parent),
	m_backend(backend),
	m_pasteNoteMenu(nullptr),
	m_windowIdentifier(0),
	m_loadingTime(0),
	m_loadingTimer(0),
	m_reloadTimer(0)
{
	Q_UNUSED(isPrivate)

	connect(this, SIGNAL(loadingStateChanged(WebWidget::LoadingState)), this, SLOT(handleLoadingStateChange(WebWidget::LoadingState)));
	connect(BookmarksManager::getModel(), SIGNAL(modelModified()), this, SLOT(updateBookmarkActions()));
	connect(PasswordsManager::getInstance(), &PasswordsManager::passwordsModified, [&]()
	{
		updatePageActions(getUrl());

		emit actionsStateChanged(ActionsManager::ActionDefinition::PageCategory);
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
}

void WebWidget::bounceAction(int identifier, QVariantMap parameters)
{
	parameters[QLatin1String("isBounced")] = true;

	Application::triggerAction(identifier, parameters, parentWidget());
}

void WebWidget::triggerAction()
{
	Action *action(qobject_cast<Action*>(sender()));

	if (action)
	{
		QVariantMap parameters(action->getParameters());

		if (action->isCheckable())
		{
			parameters[QLatin1String("isChecked")] = action->isChecked();
		}

		if (action->getDefinition().scope == ActionsManager::ActionDefinition::WindowScope)
		{
			triggerAction(action->getIdentifier(), parameters);
		}
		else
		{
			Application::triggerAction(action->getIdentifier(), parameters, parentWidget());
		}
	}
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
				ContentsDialog *dialog(new ContentsDialog(ThemesManager::getIcon(QLatin1String("download")), transferDialog->windowTitle(), QString(), QString(), QDialogButtonBox::NoButton, transferDialog, this));

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

void WebWidget::pasteNote(QAction *action)
{
	if (action && action->data().isValid())
	{
		BookmarksItem *note(NotesManager::getModel()->getBookmark(action->data().toModelIndex()));

		if (note)
		{
			pasteText(note->data(BookmarksModel::DescriptionRole).toString());
		}
	}
}

void WebWidget::selectDictionary(QAction *action)
{
	if (action)
	{
		setOption(SettingsManager::Browser_SpellCheckDictionaryOption, action->data().toString());
	}
}

void WebWidget::selectDictionaryMenuAboutToShow()
{
	QMenu *menu(qobject_cast<QMenu*>(sender()));

	if (!menu)
	{
		return;
	}

	QString dictionary(getOption(SettingsManager::Browser_SpellCheckDictionaryOption, getUrl()).toString());

	if (dictionary.isEmpty())
	{
		dictionary = SpellCheckManager::getDefaultDictionary();
	}

	for (int i = 0; i < menu->actions().count(); ++i)
	{
		QAction *action(menu->actions().at(i));

		if (action)
		{
			action->setChecked(action->data().toString() == dictionary);
		}
	}
}

void WebWidget::openInApplicationMenuAboutToShow()
{
	Menu *menu(qobject_cast<Menu*>(sender()));

	if (!menu || !menu->actions().isEmpty())
	{
		return;
	}

	Action *parentAction(qobject_cast<Action*>(menu->menuAction()));
	QString query(QLatin1String("{pageUrl}"));

	if (parentAction)
	{
		switch (parentAction->getIdentifier())
		{
			case ActionsManager::OpenLinkInApplicationAction:
				query = QLatin1String("{linkUrl}");

				break;
			case ActionsManager::OpenFrameInApplicationAction:
				query = QLatin1String("{frameUrl}");

				break;
			default:
				break;
		}
	}

	const QVector<ApplicationInformation> applications(Utils::getApplicationsForMimeType(QMimeDatabase().mimeTypeForName(QLatin1String("text/html"))));

	if (applications.isEmpty())
	{
		Action *action(menu->addAction(ActionsManager::OpenUrlAction));
		action->setOverrideText(QT_TRANSLATE_NOOP("actions", "Default Application"));
		action->setParameters({{QLatin1String("application"), QString()}, {QLatin1String("urlPlaceholder"), query}});

		connect(action, SIGNAL(triggered()), this, SLOT(triggerAction()));
	}
	else
	{
		for (int i = 0; i < applications.count(); ++i)
		{
			Action *action(menu->addAction(ActionsManager::OpenUrlAction));
			action->setIcon(applications.at(i).icon);
			action->setOverrideText(((applications.at(i).name.isEmpty()) ? QT_TRANSLATE_NOOP("actions", "Unknown") : applications.at(i).name));
			action->setParameters({{QLatin1String("application"), applications.at(i).command}, {QLatin1String("urlPlaceholder"), query}});

			connect(action, SIGNAL(triggered()), this, SLOT(triggerAction()));

			if (i == 0)
			{
				menu->addSeparator();
			}
		}
	}
}

void WebWidget::clearOptions()
{
	const QList<int> identifiers(m_options.keys());

	m_options.clear();

	for (int i = 0; i < identifiers.count(); ++i)
	{
		emit optionChanged(identifiers.at(i), QVariant());
	}

	if (m_actions.contains(ActionsManager::ResetQuickPreferencesAction))
	{
		m_actions[ActionsManager::ResetQuickPreferencesAction]->setEnabled(false);
	}
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

void WebWidget::handleAudibleStateChange(bool isAudible)
{
	if (m_actions.contains(ActionsManager::MuteTabMediaAction))
	{
		m_actions[ActionsManager::MuteTabMediaAction]->setEnabled(isAudible || isAudioMuted());
		m_actions[ActionsManager::MuteTabMediaAction]->setIcon(ThemesManager::getIcon(isAudioMuted() ? QLatin1String("audio-volume-muted") : QLatin1String("audio-volume-medium")));
		m_actions[ActionsManager::MuteTabMediaAction]->setOverrideText(isAudioMuted() ? QT_TRANSLATE_NOOP("actions", "Unmute Tab Media") : QT_TRANSLATE_NOOP("actions", "Mute Tab Media"));
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

	ContentsDialog *dialog(new ContentsDialog(ThemesManager::getIcon(QLatin1String("dialog-warning")), tr("JavaScript"), tr("Webpage wants to close this tab, do you want to allow to close it?"), QString(), (QDialogButtonBox::Ok | QDialogButtonBox::Cancel), nullptr, this));
	dialog->setCheckBox(tr("Do not show this message again"), false);

	connect(this, SIGNAL(aboutToReload()), dialog, SLOT(close()));
	connect(dialog, &ContentsDialog::finished, [&](int result, bool isChecked)
	{
		const bool isAccepted(result == QDialog::Accepted);

		if (isChecked)
		{
			SettingsManager::setValue(SettingsManager::Permissions_ScriptsCanCloseWindowsOption, (isAccepted ? QLatin1String("allow") : QLatin1String("disallow")));
		}

		if (isAccepted)
		{
			emit requestedCloseWindow();
		}
	});

	showDialog(dialog, false);
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
	updateEditActions();

	emit actionsStateChanged(ActionsManager::ActionDefinition::EditingCategory);

	QStringList flags;

	if (m_hitResult.flags.testFlag(IsFormTest))
	{
		flags.append(QLatin1String("form"));
	}

	if (!m_hitResult.imageUrl.isValid() && m_hitResult.flags.testFlag(IsSelectedTest) && hasSelection)
	{
		flags.append(QLatin1String("selection"));
	}

	if (m_hitResult.linkUrl.isValid())
	{
		if (m_hitResult.linkUrl.scheme() == QLatin1String("mailto"))
		{
			flags.append(QLatin1String("mail"));
		}
		else
		{
			flags.append(QLatin1String("link"));
		}
	}

	if (!m_hitResult.imageUrl.isEmpty())
	{
		flags.append(QLatin1String("image"));
	}

	if (m_hitResult.mediaUrl.isValid())
	{
		flags.append(QLatin1String("media"));
	}

	if (m_hitResult.flags.testFlag(IsContentEditableTest))
	{
		flags.append(QLatin1String("edit"));
	}

	if (flags.isEmpty() || (flags.count() == 1 && flags.first() == QLatin1String("form")))
	{
		flags.append(QLatin1String("standard"));

		if (m_hitResult.frameUrl.isValid())
		{
			flags.append(QLatin1String("frame"));
		}
	}

	if (flags.isEmpty())
	{
		return;
	}

	Menu menu(Menu::NoMenuRole, this);
	menu.load(QLatin1String("menu/webWidget.json"), flags);
	menu.exec(mapToGlobal(hitPosition));
}

void WebWidget::updatePageActions(const QUrl &url)
{
	if (m_actions.contains(ActionsManager::FillPasswordAction))
	{
		m_actions[ActionsManager::FillPasswordAction]->setEnabled(!Utils::isUrlEmpty(url) && PasswordsManager::hasPasswords(url, PasswordsManager::FormPassword));
	}

	if (m_actions.contains(ActionsManager::BookmarkPageAction))
	{
		m_actions[ActionsManager::BookmarkPageAction]->setOverrideText(BookmarksManager::hasBookmark(url) ? QT_TRANSLATE_NOOP("actions", "Edit Bookmark…") : QT_TRANSLATE_NOOP("actions", "Add Bookmark…"));
	}

	if (m_actions.contains(ActionsManager::EnableJavaScriptAction))
	{
		m_actions[ActionsManager::EnableJavaScriptAction]->setChecked(getOption(SettingsManager::Permissions_EnableJavaScriptOption, url).toBool());
	}

	if (m_actions.contains(ActionsManager::EnableReferrerAction))
	{
		m_actions[ActionsManager::EnableReferrerAction]->setChecked(getOption(SettingsManager::Network_EnableReferrerOption, url).toBool());
	}

	if (m_actions.contains(ActionsManager::WebsitePreferencesAction))
	{
		m_actions[ActionsManager::WebsitePreferencesAction]->setEnabled(!url.isEmpty() && url.scheme() != QLatin1String("about"));
	}
}

void WebWidget::updateNavigationActions()
{
	if (m_actions.contains(ActionsManager::GoBackAction))
	{
		m_actions[ActionsManager::GoBackAction]->setEnabled(canGoBack());
	}

	if (m_actions.contains(ActionsManager::GoForwardAction))
	{
		m_actions[ActionsManager::GoForwardAction]->setEnabled(canGoForward());
	}

	if (m_actions.contains(ActionsManager::RewindAction))
	{
		m_actions[ActionsManager::RewindAction]->setEnabled(canGoBack());
	}

	if (m_actions.contains(ActionsManager::FastForwardAction))
	{
		m_actions[ActionsManager::FastForwardAction]->setEnabled(canFastForward());
	}

	if (m_actions.contains(ActionsManager::StopAction))
	{
		m_actions[ActionsManager::StopAction]->setEnabled(getLoadingState() == OngoingLoadingState);
	}

	if (m_actions.contains(ActionsManager::ReloadAction))
	{
		m_actions[ActionsManager::ReloadAction]->setEnabled(getLoadingState() != OngoingLoadingState);
	}

	if (m_actions.contains(ActionsManager::ReloadOrStopAction))
	{
		m_actions[ActionsManager::ReloadOrStopAction]->setup((getLoadingState() == OngoingLoadingState) ? createAction(ActionsManager::StopAction) : createAction(ActionsManager::ReloadAction));
	}

	if (m_actions.contains(ActionsManager::LoadPluginsAction))
	{
		m_actions[ActionsManager::LoadPluginsAction]->setEnabled(getAmountOfNotLoadedPlugins() > 0);
	}

	if (m_actions.contains(ActionsManager::ViewSourceAction))
	{
		m_actions[ActionsManager::ViewSourceAction]->setEnabled(canViewSource());
	}
}

void WebWidget::updateEditActions()
{
	const bool canPaste(QApplication::clipboard()->mimeData() && QApplication::clipboard()->mimeData()->hasText());
	const bool canSpellCheck(getOption(SettingsManager::Browser_EnableSpellCheckOption, getUrl()).toBool() && !getDictionaries().isEmpty());
	const bool hasSelection(this->hasSelection() && !getSelectedText().trimmed().isEmpty());

	if (m_actions.contains(ActionsManager::CutAction))
	{
		m_actions[ActionsManager::CutAction]->setEnabled(hasSelection && m_hitResult.flags.testFlag(IsContentEditableTest));
	}

	if (m_actions.contains(ActionsManager::CopyAction))
	{
		m_actions[ActionsManager::CopyAction]->setEnabled(hasSelection);
	}

	if (m_actions.contains(ActionsManager::CopyPlainTextAction))
	{
		m_actions[ActionsManager::CopyPlainTextAction]->setEnabled(hasSelection);
	}

	if (m_actions.contains(ActionsManager::CopyToNoteAction))
	{
		m_actions[ActionsManager::CopyToNoteAction]->setEnabled(hasSelection);
	}

	if (m_actions.contains(ActionsManager::PasteAction))
	{
		m_actions[ActionsManager::PasteAction]->setEnabled(canPaste && m_hitResult.flags.testFlag(IsContentEditableTest));
	}

	if (m_actions.contains(ActionsManager::PasteAndGoAction))
	{
		m_actions[ActionsManager::PasteAndGoAction]->setEnabled(canPaste);
	}

	if (m_actions.contains(ActionsManager::PasteNoteAction))
	{
		m_actions[ActionsManager::PasteNoteAction]->setEnabled(canPaste && m_hitResult.flags.testFlag(IsContentEditableTest) && NotesManager::getModel()->getRootItem()->hasChildren());
	}

	if (m_actions.contains(ActionsManager::DeleteAction))
	{
		m_actions[ActionsManager::DeleteAction]->setEnabled(m_hitResult.flags.testFlag(IsContentEditableTest) && !m_hitResult.flags.testFlag(IsEmptyTest));
	}

	if (m_actions.contains(ActionsManager::SelectAllAction))
	{
		m_actions[ActionsManager::SelectAllAction]->setEnabled(!m_hitResult.flags.testFlag(IsEmptyTest));
	}

	if (m_actions.contains(ActionsManager::UnselectAction))
	{
		m_actions[ActionsManager::UnselectAction]->setEnabled(hasSelection);
	}

	if (m_actions.contains(ActionsManager::CheckSpellingAction))
	{
		m_actions[ActionsManager::CheckSpellingAction]->setChecked(m_hitResult.flags.testFlag(IsSpellCheckEnabled));
		m_actions[ActionsManager::CheckSpellingAction]->setEnabled(canSpellCheck);
	}

	if (m_actions.contains(ActionsManager::SelectDictionaryAction))
	{
		m_actions[ActionsManager::SelectDictionaryAction]->setEnabled(canSpellCheck);
	}

	if (m_actions.contains(ActionsManager::ClearAllAction))
	{
		m_actions[ActionsManager::ClearAllAction]->setEnabled(m_hitResult.flags.testFlag(IsContentEditableTest) && !m_hitResult.flags.testFlag(IsEmptyTest));
	}

	if (m_actions.contains(ActionsManager::SearchAction))
	{
		const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(getOption(SettingsManager::Search_DefaultQuickSearchEngineOption).toString()));

		m_actions[ActionsManager::SearchAction]->setEnabled(searchEngine.isValid());
		m_actions[ActionsManager::SearchAction]->setParameters({{QLatin1String("searchEngine"), searchEngine.identifier}, {QLatin1String("queryPlaceholder"), QLatin1String("{selection}")}});
		m_actions[ActionsManager::SearchAction]->setIcon(searchEngine.icon.isNull() ? ThemesManager::getIcon(QLatin1String("edit-find")) : searchEngine.icon);
		m_actions[ActionsManager::SearchAction]->setOverrideText(searchEngine.isValid() ? searchEngine.title : QT_TRANSLATE_NOOP("actions", "Search"));
		m_actions[ActionsManager::SearchAction]->setToolTip(searchEngine.isValid() ? searchEngine.description : tr("No search engines defined"));
	}

	if (m_actions.contains(ActionsManager::CreateSearchAction))
	{
		m_actions[ActionsManager::CreateSearchAction]->setEnabled(m_hitResult.flags.testFlag(IsFormTest));
	}

	updateLinkActions();
	updateFrameActions();
	updateImageActions();
	updateMediaActions();
}

void WebWidget::updateLinkActions()
{
	const bool isLink(m_hitResult.linkUrl.isValid());

	if (m_actions.contains(ActionsManager::OpenLinkAction))
	{
		m_actions[ActionsManager::OpenLinkAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInCurrentTabAction))
	{
		m_actions[ActionsManager::OpenLinkInCurrentTabAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewTabAction))
	{
		m_actions[ActionsManager::OpenLinkInNewTabAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewTabBackgroundAction))
	{
		m_actions[ActionsManager::OpenLinkInNewTabBackgroundAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewWindowAction))
	{
		m_actions[ActionsManager::OpenLinkInNewWindowAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewWindowBackgroundAction))
	{
		m_actions[ActionsManager::OpenLinkInNewWindowBackgroundAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewPrivateTabAction))
	{
		m_actions[ActionsManager::OpenLinkInNewPrivateTabAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewPrivateTabBackgroundAction))
	{
		m_actions[ActionsManager::OpenLinkInNewPrivateTabBackgroundAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewPrivateWindowAction))
	{
		m_actions[ActionsManager::OpenLinkInNewPrivateWindowAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInNewPrivateWindowBackgroundAction))
	{
		m_actions[ActionsManager::OpenLinkInNewPrivateWindowBackgroundAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::OpenLinkInApplicationAction))
	{
		m_actions[ActionsManager::OpenLinkInApplicationAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::CopyLinkToClipboardAction))
	{
		m_actions[ActionsManager::CopyLinkToClipboardAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::BookmarkLinkAction))
	{
		m_actions[ActionsManager::BookmarkLinkAction]->setOverrideText(BookmarksManager::hasBookmark(m_hitResult.linkUrl) ? QT_TRANSLATE_NOOP("actions", "Edit Link Bookmark…") : QT_TRANSLATE_NOOP("actions", "Bookmark Link…"));
		m_actions[ActionsManager::BookmarkLinkAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::SaveLinkToDiskAction))
	{
		m_actions[ActionsManager::SaveLinkToDiskAction]->setEnabled(isLink);
	}

	if (m_actions.contains(ActionsManager::SaveLinkToDownloadsAction))
	{
		m_actions[ActionsManager::SaveLinkToDownloadsAction]->setEnabled(isLink);
	}
}

void WebWidget::updateFrameActions()
{
	const bool isFrame(m_hitResult.frameUrl.isValid());

	if (m_actions.contains(ActionsManager::OpenFrameInCurrentTabAction))
	{
		m_actions[ActionsManager::OpenFrameInCurrentTabAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(ActionsManager::OpenFrameInNewTabAction))
	{
		m_actions[ActionsManager::OpenFrameInNewTabAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(ActionsManager::OpenFrameInNewTabBackgroundAction))
	{
		m_actions[ActionsManager::OpenFrameInNewTabBackgroundAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(ActionsManager::OpenFrameInApplicationAction))
	{
		m_actions[ActionsManager::OpenFrameInApplicationAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(ActionsManager::CopyFrameLinkToClipboardAction))
	{
		m_actions[ActionsManager::CopyFrameLinkToClipboardAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(ActionsManager::ReloadFrameAction))
	{
		m_actions[ActionsManager::ReloadFrameAction]->setEnabled(isFrame);
	}

	if (m_actions.contains(ActionsManager::ViewFrameSourceAction))
	{
		m_actions[ActionsManager::ViewFrameSourceAction]->setEnabled(isFrame);
	}
}

void WebWidget::updateImageActions()
{
	const bool isImage(!m_hitResult.imageUrl.isEmpty());
	const bool isOpened(getUrl().matches(m_hitResult.imageUrl, (QUrl::NormalizePathSegments | QUrl::RemoveFragment | QUrl::StripTrailingSlash)));
	const QString fileName(fontMetrics().elidedText(m_hitResult.imageUrl.fileName(), Qt::ElideMiddle, 256));

	if (m_actions.contains(ActionsManager::OpenImageInNewTabAction))
	{
		m_actions[ActionsManager::OpenImageInNewTabAction]->setOverrideText(isImage ? (fileName.isEmpty() || m_hitResult.imageUrl.scheme() == QLatin1String("data")) ? tr("Open Image in New Tab (Untitled)") : tr("Open Image in New Tab (%1)").arg(fileName) : QT_TRANSLATE_NOOP("actions", "Open Image in New Tab"));
		m_actions[ActionsManager::OpenImageInNewTabAction]->setEnabled(isImage && !isOpened);
	}

	if (m_actions.contains(ActionsManager::OpenImageInNewTabBackgroundAction))
	{
		m_actions[ActionsManager::OpenImageInNewTabBackgroundAction]->setOverrideText(isImage ? (fileName.isEmpty() || m_hitResult.imageUrl.scheme() == QLatin1String("data")) ? tr("Open Image in New Background Tab (Untitled)") : tr("Open Image in New Background Tab (%1)").arg(fileName) : QT_TRANSLATE_NOOP("actions", "Open Image in New Background Tab"));
		m_actions[ActionsManager::OpenImageInNewTabBackgroundAction]->setEnabled(isImage && !isOpened);
	}

	if (m_actions.contains(ActionsManager::SaveImageToDiskAction))
	{
		m_actions[ActionsManager::SaveImageToDiskAction]->setEnabled(isImage);
	}

	if (m_actions.contains(ActionsManager::CopyImageToClipboardAction))
	{
		m_actions[ActionsManager::CopyImageToClipboardAction]->setEnabled(isImage);
	}

	if (m_actions.contains(ActionsManager::CopyImageUrlToClipboardAction))
	{
		m_actions[ActionsManager::CopyImageUrlToClipboardAction]->setEnabled(isImage);
	}

	if (m_actions.contains(ActionsManager::ReloadImageAction))
	{
		m_actions[ActionsManager::ReloadImageAction]->setEnabled(isImage);
	}

	if (m_actions.contains(ActionsManager::ImagePropertiesAction))
	{
		m_actions[ActionsManager::ImagePropertiesAction]->setEnabled(isImage);
	}
}

void WebWidget::updateMediaActions()
{
	const bool isMedia(m_hitResult.mediaUrl.isValid());
	const bool isVideo(m_hitResult.tagName == QLatin1String("video"));
	const bool isPaused(m_hitResult.flags.testFlag(MediaIsPausedTest));
	const bool isMuted(m_hitResult.flags.testFlag(MediaIsMutedTest));

	if (m_actions.contains(ActionsManager::SaveMediaToDiskAction))
	{
		m_actions[ActionsManager::SaveMediaToDiskAction]->setOverrideText(isVideo ? QT_TRANSLATE_NOOP("actions", "Save Video…") : QT_TRANSLATE_NOOP("actions", "Save Audio…"));
		m_actions[ActionsManager::SaveMediaToDiskAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(ActionsManager::CopyMediaUrlToClipboardAction))
	{
		m_actions[ActionsManager::CopyMediaUrlToClipboardAction]->setOverrideText(isVideo ? QT_TRANSLATE_NOOP("actions", "Copy Video Link to Clipboard") : QT_TRANSLATE_NOOP("actions", "Copy Audio Link to Clipboard"));
		m_actions[ActionsManager::CopyMediaUrlToClipboardAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(ActionsManager::MediaControlsAction))
	{
		m_actions[ActionsManager::MediaControlsAction]->setChecked(m_hitResult.flags.testFlag(MediaHasControlsTest));
		m_actions[ActionsManager::MediaControlsAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(ActionsManager::MediaLoopAction))
	{
		m_actions[ActionsManager::MediaLoopAction]->setChecked(m_hitResult.flags.testFlag(MediaIsLoopedTest));
		m_actions[ActionsManager::MediaLoopAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(ActionsManager::MediaPlayPauseAction))
	{
		m_actions[ActionsManager::MediaPlayPauseAction]->setOverrideText(isPaused ? QT_TRANSLATE_NOOP("actions", "Play") : QT_TRANSLATE_NOOP("actions", "Pause"));
		m_actions[ActionsManager::MediaPlayPauseAction]->setIcon(ThemesManager::getIcon(isPaused ? QLatin1String("media-playback-start") : QLatin1String("media-playback-pause")));
		m_actions[ActionsManager::MediaPlayPauseAction]->setEnabled(isMedia);
	}

	if (m_actions.contains(ActionsManager::MediaMuteAction))
	{
		m_actions[ActionsManager::MediaMuteAction]->setOverrideText(isMuted ? QT_TRANSLATE_NOOP("actions", "Unmute") : QT_TRANSLATE_NOOP("actions", "Mute"));
		m_actions[ActionsManager::MediaMuteAction]->setIcon(ThemesManager::getIcon(isMuted ? QLatin1String("audio-volume-medium") : QLatin1String("audio-volume-muted")));
		m_actions[ActionsManager::MediaMuteAction]->setEnabled(isMedia);
	}
}

void WebWidget::updateBookmarkActions()
{
	emit actionsStateChanged(ActionsManager::ActionDefinition::LinkCategory | ActionsManager::ActionDefinition::PageCategory);

	updatePageActions(getUrl());
	updateLinkActions();
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
			SettingsManager::setValue(SettingsManager::Permissions_EnableFullScreenOption, value, url);

			return;
		case GeolocationFeature:
			SettingsManager::setValue(SettingsManager::Permissions_EnableGeolocationOption, value, url);

			return;
		case NotificationsFeature:
			SettingsManager::setValue(SettingsManager::Permissions_EnableNotificationsOption, value, url);

			return;
		case PointerLockFeature:
			SettingsManager::setValue(SettingsManager::Permissions_EnablePointerLockOption, value, url);

			return;
		case CaptureAudioFeature:
			SettingsManager::setValue(SettingsManager::Permissions_EnableMediaCaptureAudioOption, value, url);

			return;
		case CaptureVideoFeature:
			SettingsManager::setValue(SettingsManager::Permissions_EnableMediaCaptureVideoOption, value, url);

			return;
		case CaptureAudioVideoFeature:
			SettingsManager::setValue(SettingsManager::Permissions_EnableMediaCaptureAudioOption, value, url);
			SettingsManager::setValue(SettingsManager::Permissions_EnableMediaCaptureVideoOption, value, url);

			return;
		case PlaybackAudioFeature:
			SettingsManager::setValue(SettingsManager::Permissions_EnableMediaPlaybackAudioOption, value, url);

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

	if (m_actions.contains(ActionsManager::ResetQuickPreferencesAction))
	{
		m_actions[ActionsManager::ResetQuickPreferencesAction]->setEnabled(!m_options.isEmpty());
	}

	SessionsManager::markSessionModified();

	emit optionChanged(identifier, (value.isNull() ? getOption(identifier) : value));

	switch (identifier)
	{
		case SettingsManager::Content_PageReloadTimeOption:
			{
				const int reloadTime(value.toInt());

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

	if (m_actions.contains(ActionsManager::ResetQuickPreferencesAction))
	{
		m_actions[ActionsManager::ResetQuickPreferencesAction]->setEnabled(!m_options.isEmpty());
	}

	for (int i = 0; i < identifiers.count(); ++i)
	{
		emit optionChanged(identifiers.at(i), m_options.value(identifiers.at(i)));
	}
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

Action* WebWidget::createAction(int identifier, const QVariantMap parameters, bool followState)
{
	Q_UNUSED(parameters)
	Q_UNUSED(followState)

	if (identifier < 0)
	{
		return nullptr;
	}

	if (m_actions.contains(identifier))
	{
		return m_actions[identifier];
	}

	Action *action(new Action(identifier, this));
	action->setState(getActionState(identifier));

	m_actions[identifier] = action;

	connect(action, SIGNAL(triggered()), this, SLOT(triggerAction()));

	switch (identifier)
	{
		case ActionsManager::PasteNoteAction:
			if (!m_pasteNoteMenu)
			{
				m_pasteNoteMenu = new Menu(Menu::NotesMenuRole, this);

				connect(m_pasteNoteMenu, SIGNAL(triggered(QAction*)), this, SLOT(pasteNote(QAction*)));
			}

			action->setMenu(m_pasteNoteMenu);

			break;
		case ActionsManager::OpenLinkInApplicationAction:
			action->setMenu(new Menu(Menu::NoMenuRole, this));

			connect(action->menu(), SIGNAL(aboutToShow()), this, SLOT(openInApplicationMenuAboutToShow()));

			break;
		case ActionsManager::OpenFrameInApplicationAction:
			action->setMenu(new Menu(Menu::NoMenuRole, this));

			connect(action->menu(), SIGNAL(aboutToShow()), this, SLOT(openInApplicationMenuAboutToShow()));

			break;
		case ActionsManager::SearchAction:
			action->setParameters({{QLatin1String("searchEngine"), getOption(SettingsManager::Search_DefaultQuickSearchEngineOption)}, {QLatin1String("queryPlaceholder"), QLatin1String("{selection}")}});

			break;
		case ActionsManager::OpenPageInApplicationAction:
			action->setMenu(new Menu(Menu::NoMenuRole, this));

			connect(action->menu(), SIGNAL(aboutToShow()), this, SLOT(openInApplicationMenuAboutToShow()));

			break;
		case ActionsManager::SelectDictionaryAction:
			{
				const QVector<SpellCheckManager::DictionaryInformation> dictionaries(getDictionaries());
				QMenu *menu(new QMenu(this));
				QActionGroup *dictionariesGroup(new QActionGroup(menu));
				dictionariesGroup->setExclusive(true);

				action->setMenu(menu);

				for (int i = 0; i < dictionaries.count(); ++i)
				{
					QAction *action(menu->addAction(dictionaries.at(i).title));
					action->setCheckable(true);
					action->setData(dictionaries.at(i).name);

					dictionariesGroup->addAction(action);
				}

				connect(menu, SIGNAL(aboutToShow()), this, SLOT(selectDictionaryMenuAboutToShow()));
				connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(selectDictionary(QAction*)));
			}

			break;
		default:
			break;
	}

	return action;
}

QWidget* WebWidget::getInspector()
{
	return nullptr;
}

QWidget* WebWidget::getViewport()
{
	return this;
}

Action* WebWidget::getExistingAction(int identifier)
{
	return (m_actions.contains(identifier) ? m_actions[identifier] : nullptr);
}

WebBackend* WebWidget::getBackend()
{
	return m_backend;
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

QVariant WebWidget::getPageInformation(WebWidget::PageInformation key) const
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
	ActionsManager::ActionDefinition::State state(ActionsManager::getActionDefinition(identifier).defaultState);

	switch (identifier)
	{
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
		case ActionsManager::OpenLinkInApplicationAction:
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
		case ActionsManager::OpenFrameInApplicationAction:
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
			state.isChecked = m_hitResult.flags.testFlag(MediaHasControlsTest);
			state.isEnabled = m_hitResult.mediaUrl.isValid();

			break;
		case ActionsManager::MediaLoopAction:
			state.isChecked = m_hitResult.flags.testFlag(MediaIsLoopedTest);
			state.isEnabled = m_hitResult.mediaUrl.isValid();

			break;
		case ActionsManager::MediaPlayPauseAction:
			state.text = (m_hitResult.flags.testFlag(MediaIsPausedTest) ? QT_TRANSLATE_NOOP("actions", "Play") : QT_TRANSLATE_NOOP("actions", "Pause"));
			state.icon = ThemesManager::getIcon(m_hitResult.flags.testFlag(MediaIsPausedTest) ? QLatin1String("media-playback-start") : QLatin1String("media-playback-pause"));
			state.isEnabled = m_hitResult.mediaUrl.isValid();

			break;
		case ActionsManager::MediaMuteAction:
			state.text = (m_hitResult.flags.testFlag(MediaIsMutedTest) ? QT_TRANSLATE_NOOP("actions", "Unmute") : QT_TRANSLATE_NOOP("actions", "Mute"));
			state.icon = ThemesManager::getIcon(m_hitResult.flags.testFlag(MediaIsMutedTest) ? QLatin1String("audio-volume-medium") : QLatin1String("audio-volume-muted"));
			state.isEnabled = m_hitResult.mediaUrl.isValid();

			break;
		case ActionsManager::FillPasswordAction:
			state.isEnabled = (!Utils::isUrlEmpty(getUrl()) && PasswordsManager::hasPasswords(getUrl(), PasswordsManager::FormPassword));

			break;
		case ActionsManager::MuteTabMediaAction:
			state.isEnabled = (isAudible() || isAudioMuted());
			state.icon = ThemesManager::getIcon(isAudioMuted() ? QLatin1String("audio-volume-muted") : QLatin1String("audio-volume-medium"));
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
			if (parameters.contains(QLatin1String("time")) && parameters[QLatin1String("time")].type() != QVariant::String)
			{
				const int reloadTime(parameters[QLatin1String("time")].toInt());

				if (reloadTime < 0)
				{
					state.isChecked = (!m_options.contains(SettingsManager::Content_PageReloadTimeOption) || m_options[SettingsManager::Content_PageReloadTimeOption].toInt() < 0);
				}
				else
				{
					state.isChecked = (m_options.contains(SettingsManager::Content_PageReloadTimeOption) && reloadTime == m_options[SettingsManager::Content_PageReloadTimeOption].toInt());
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
			state.isEnabled = (this->hasSelection() && !getSelectedText().trimmed().isEmpty() && m_hitResult.flags.testFlag(IsContentEditableTest));

			break;
		case ActionsManager::CopyAction:
		case ActionsManager::CopyPlainTextAction:
		case ActionsManager::CopyToNoteAction:
		case ActionsManager::UnselectAction:
			state.isEnabled = (this->hasSelection() && !getSelectedText().trimmed().isEmpty());

			break;
		case ActionsManager::PasteAction:
			state.isEnabled = (m_hitResult.flags.testFlag(IsContentEditableTest) && QApplication::clipboard()->mimeData() && QApplication::clipboard()->mimeData()->hasText());

			break;
		case ActionsManager::PasteAndGoAction:
			state.isEnabled = (QApplication::clipboard()->mimeData() && QApplication::clipboard()->mimeData()->hasText());

			break;
		case ActionsManager::PasteNoteAction:
			state.isEnabled = (m_hitResult.flags.testFlag(IsContentEditableTest) && QApplication::clipboard()->mimeData() && QApplication::clipboard()->mimeData()->hasText() && NotesManager::getModel()->getRootItem()->hasChildren());

			break;
		case ActionsManager::DeleteAction:
			state.isEnabled = (m_hitResult.flags.testFlag(IsContentEditableTest) && !m_hitResult.flags.testFlag(IsEmptyTest));

			break;
		case ActionsManager::SelectAllAction:
			state.isEnabled = (!m_hitResult.flags.testFlag(IsEmptyTest));

			break;
		case ActionsManager::ClearAllAction:
			state.isEnabled = (m_hitResult.flags.testFlag(IsContentEditableTest) && !m_hitResult.flags.testFlag(IsEmptyTest));

			break;
		case ActionsManager::CheckSpellingAction:
			state.isChecked = (m_hitResult.flags.testFlag(IsSpellCheckEnabled));
			state.isEnabled = (getOption(SettingsManager::Browser_EnableSpellCheckOption, getUrl()).toBool() && !getDictionaries().isEmpty());

			break;
		case ActionsManager::SelectDictionaryAction:
			state.isEnabled = (getOption(SettingsManager::Browser_EnableSpellCheckOption, getUrl()).toBool() && !getDictionaries().isEmpty());

			break;
		case ActionsManager::SearchAction:
			{
				const SearchEnginesManager::SearchEngineDefinition searchEngine(SearchEnginesManager::getSearchEngine(parameters.contains(QLatin1String("searchEngine")) ? parameters[QLatin1String("searchEngine")].toString() : getOption(SettingsManager::Search_DefaultQuickSearchEngineOption).toString()));

				state.text = (searchEngine.isValid() ? searchEngine.title : QT_TRANSLATE_NOOP("actions", "Search"));
				state.icon = (searchEngine.icon.isNull() ? ThemesManager::getIcon(QLatin1String("edit-find")) : searchEngine.icon);
				state.isEnabled = searchEngine.isValid();
			}

			break;
		case ActionsManager::CreateSearchAction:
			state.isEnabled = m_hitResult.flags.testFlag(IsFormTest);

			break;
		case ActionsManager::BookmarkPageAction:
			state.text = (BookmarksManager::hasBookmark(getUrl()) ? QT_TRANSLATE_NOOP("actions", "Edit Bookmark…") : QT_TRANSLATE_NOOP("actions", "Add Bookmark…"));

			break;
		case ActionsManager::LoadPluginsAction:
			state.isEnabled = (getAmountOfNotLoadedPlugins() > 0);

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

WebWidget::PermissionPolicy WebWidget::getPermission(WebWidget::FeaturePermission feature, const QUrl &url) const
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

int WebWidget::getAmountOfNotLoadedPlugins() const
{
	return 0;
}

bool WebWidget::calculateCheckedState(int identifier, const QVariantMap &parameters)
{
	Action *action(getExistingAction(identifier));

	if (action || parameters.contains(QLatin1String("isChecked")))
	{
		return Action::calculateCheckedState(parameters, action);
	}

	switch (identifier)
	{
		case ActionsManager::MediaControlsAction:
			return !m_hitResult.flags.testFlag(MediaHasControlsTest);
		case ActionsManager::MediaLoopAction:
			return !m_hitResult.flags.testFlag(MediaIsLoopedTest);
		case ActionsManager::CheckSpellingAction:
			return !m_hitResult.flags.testFlag(IsSpellCheckEnabled);
		case ActionsManager::InspectPageAction:
			return !isInspecting();
		default:
			break;
	}

	return Action::calculateCheckedState(parameters, createAction(identifier));
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
