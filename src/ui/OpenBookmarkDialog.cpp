/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "OpenBookmarkDialog.h"
#include "../core/BookmarksManager.h"
#include "../core/BookmarksModel.h"
#include "../core/SessionsManager.h"

#include "ui_OpenBookmarkDialog.h"

#include <QtCore/QStringListModel>

namespace Otter
{

OpenBookmarkDialog::OpenBookmarkDialog(ActionExecutor::Object executor, QWidget *parent) : Dialog(parent),
	m_completer(nullptr),
	m_executor(executor),
	m_ui(new Ui::OpenBookmarkDialog)
{
	m_ui->setupUi(this);

	m_completer = new QCompleter(new QStringListModel(BookmarksManager::getKeywords()), m_ui->lineEditWidget);
	m_completer->setCaseSensitivity(Qt::CaseSensitive);
	m_completer->setCompletionMode(QCompleter::InlineCompletion);
	m_completer->setFilterMode(Qt::MatchStartsWith);

	connect(this, &OpenBookmarkDialog::accepted, this, &OpenBookmarkDialog::openBookmark);
	connect(m_ui->lineEditWidget, &LineEditWidget::textEdited, this, &OpenBookmarkDialog::setCompletion);
}

OpenBookmarkDialog::~OpenBookmarkDialog()
{
	delete m_ui;
}

void OpenBookmarkDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void OpenBookmarkDialog::openBookmark()
{
	const BookmarksModel::Bookmark *bookmark(BookmarksManager::getBookmark(m_ui->lineEditWidget->text()));

	if (bookmark && m_executor.isValid())
	{
		m_executor.triggerAction(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), bookmark->getIdentifier()}, {QLatin1String("hints"), QVariant(SessionsManager::calculateOpenHints(SessionsManager::DefaultOpen))}});
	}
}

void OpenBookmarkDialog::setCompletion(const QString &text)
{
	m_completer->setCompletionPrefix(text);

	if (m_completer->completionCount() == 1)
	{
		const BookmarksModel::Bookmark *bookmark(BookmarksManager::getBookmark(m_completer->currentCompletion()));

		if (bookmark && m_executor.isValid())
		{
			m_executor.triggerAction(ActionsManager::OpenBookmarkAction, {{QLatin1String("bookmark"), bookmark->getIdentifier()}, {QLatin1String("hints"), QVariant(SessionsManager::calculateOpenHints(SessionsManager::DefaultOpen))}});
		}

		close();
	}
}

}
