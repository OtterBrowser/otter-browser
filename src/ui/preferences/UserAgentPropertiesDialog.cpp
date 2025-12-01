/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "UserAgentPropertiesDialog.h"
#include "../../core/AddonsManager.h"
#include "../../core/ThemesManager.h"
#include "../../core/WebBackend.h"

#include "ui_UserAgentPropertiesDialog.h"

#include <QtGui/QContextMenuEvent>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolTip>

namespace Otter
{

UserAgentPropertiesDialog::UserAgentPropertiesDialog(const UserAgentDefinition &userAgent, QWidget *parent) : Dialog(parent),
	m_userAgent(userAgent),
	m_ui(new Ui::UserAgentPropertiesDialog)
{
	m_ui->setupUi(this);
	m_ui->titleLineEditWidget->setText(userAgent.getTitle());
	m_ui->valueLineEditWidget->setText(userAgent.value);
	m_ui->previewButton->setIcon(ThemesManager::createIcon(QLatin1String("document-preview")));

	if (userAgent.identifier == QLatin1String("default"))
	{
		m_ui->valueLineEditWidget->setReadOnly(true);
	}
	else
	{
		m_ui->valueLineEditWidget->installEventFilter(this);

		connect(m_ui->valueLineEditWidget, &QLineEdit::customContextMenuRequested, this, [&](const QPoint &position)
		{
			QMenu *contextMenu(m_ui->valueLineEditWidget->createStandardContextMenu());
			contextMenu->addSeparator();

			QMenu *placeholdersMenu(contextMenu->addMenu(tr("Placeholders")));
			placeholdersMenu->addAction(tr("Platform"))->setData(QLatin1String("platform"));
			placeholdersMenu->addAction(tr("Engine Version"))->setData(QLatin1String("engineVersion"));
			placeholdersMenu->addAction(tr("Application Version"))->setData(QLatin1String("applicationVersion"));

			connect(placeholdersMenu, &QMenu::triggered, this, [&](QAction *action)
			{
				const QString placeholder(action->data().toString());

				if (!placeholder.isEmpty())
				{
					m_ui->valueLineEditWidget->insert(QLatin1Char('{') + placeholder + QLatin1Char('}'));
				}
			});

			contextMenu->exec(m_ui->valueLineEditWidget->mapToGlobal(position));
			contextMenu->deleteLater();
		});
	}

	setWindowTitle(userAgent.isValid() ? tr ("Edit User Agent") : tr("Add User Agent"));

	connect(m_ui->previewButton, &QToolButton::clicked, this, [&]()
	{
		QToolTip::showText(m_ui->valueLineEditWidget->mapToGlobal(m_ui->valueLineEditWidget->rect().bottomLeft()), AddonsManager::getWebBackend()->getUserAgent(m_ui->valueLineEditWidget->text()));
	});
}

UserAgentPropertiesDialog::~UserAgentPropertiesDialog()
{
	delete m_ui;
}

void UserAgentPropertiesDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

UserAgentDefinition UserAgentPropertiesDialog::getUserAgent() const
{
	UserAgentDefinition userAgent(m_userAgent);
	userAgent.title = m_ui->titleLineEditWidget->text();
	userAgent.value = m_ui->valueLineEditWidget->text();

	return userAgent;
}

}
