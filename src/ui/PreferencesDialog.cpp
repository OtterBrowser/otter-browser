/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "PreferencesDialog.h"
#include "ItemViewWidget.h"
#include "preferences/PreferencesAdvancedPageWidget.h"
#include "preferences/PreferencesContentPageWidget.h"
#include "preferences/PreferencesGeneralPageWidget.h"
#include "preferences/PreferencesPrivacyPageWidget.h"
#include "preferences/PreferencesSearchPageWidget.h"
#include "../core/ActionsManager.h"
#include "../core/SessionsManager.h"

#include "ui_PreferencesDialog.h"

#include <QtWidgets/QComboBox>
#include <QtWidgets/QLineEdit>

namespace Otter
{

PreferencesDialog::PreferencesDialog(const QLatin1String &section, QWidget *parent) : Dialog(parent),
	m_ui(new Ui::PreferencesDialog)
{
	m_ui->setupUi(this);

	m_loadedTabs.fill(false, 5);

	int tab(0);

	if (section == QLatin1String("content"))
	{
		tab = 1;
	}
	else if (section == QLatin1String("privacy"))
	{
		tab = 2;
	}
	else if (section == QLatin1String("search"))
	{
		tab = 3;
	}
	else if (section == QLatin1String("advanced"))
	{
		tab = 4;
	}

	currentTabChanged(tab);

	m_ui->tabWidget->setCurrentIndex(tab);
	m_ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);

	connect(m_ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));
	connect(m_ui->buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(save()));
	connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(save()));
	connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));
	connect(m_ui->allSettingsButton, SIGNAL(clicked()), this, SLOT(openConfigurationManager()));
}

PreferencesDialog::~PreferencesDialog()
{
	delete m_ui;
}

void PreferencesDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void PreferencesDialog::currentTabChanged(int tab)
{
	if (m_loadedTabs[tab])
	{
		return;
	}

	m_loadedTabs[tab] = true;

	switch (tab)
	{
		case 0:
			{
				PreferencesGeneralPageWidget *pageWidget(new PreferencesGeneralPageWidget(this));

				m_ui->generalLayout->addWidget(pageWidget);

				connect(this, SIGNAL(requestedSave()), pageWidget, SLOT(save()));
				connect(pageWidget, SIGNAL(settingsModified()), this, SLOT(markModified()));
			}

			break;
		case 1:
			{
				PreferencesContentPageWidget *pageWidget(new PreferencesContentPageWidget(this));

				m_ui->contentLayout->addWidget(pageWidget);

				connect(this, SIGNAL(requestedSave()), pageWidget, SLOT(save()));
				connect(pageWidget, SIGNAL(settingsModified()), this, SLOT(markModified()));
			}

			break;
		case 2:
			{
				PreferencesPrivacyPageWidget *pageWidget(new PreferencesPrivacyPageWidget(this));

				m_ui->privacyLayout->addWidget(pageWidget);

				connect(this, SIGNAL(requestedSave()), pageWidget, SLOT(save()));
				connect(pageWidget, SIGNAL(settingsModified()), this, SLOT(markModified()));
			}

			break;
		case 3:
			{
				PreferencesSearchPageWidget *pageWidget(new PreferencesSearchPageWidget(this));

				m_ui->searchLayout->addWidget(pageWidget);

				connect(this, SIGNAL(requestedSave()), pageWidget, SLOT(save()));
				connect(pageWidget, SIGNAL(settingsModified()), this, SLOT(markModified()));
			}

			break;
		default:
			{
				PreferencesAdvancedPageWidget *pageWidget(new PreferencesAdvancedPageWidget(this));

				m_ui->advancedLayout->addWidget(pageWidget);

				connect(this, SIGNAL(requestedSave()), pageWidget, SLOT(save()));
				connect(pageWidget, SIGNAL(settingsModified()), this, SLOT(markModified()));
			}

			break;
	}

	QWidget *widget(m_ui->tabWidget->widget(tab));
	QList<QLineEdit*> lineEdits(widget->findChildren<QLineEdit*>());

	for (int i = 0; i < lineEdits.count(); ++i)
	{
		connect(lineEdits.at(i), SIGNAL(textChanged(QString)), this, SLOT(markModified()));
	}

	QList<QAbstractButton*> buttons(widget->findChildren<QAbstractButton*>());

	for (int i = 0; i < buttons.count(); ++i)
	{
		connect(buttons.at(i), SIGNAL(toggled(bool)), this, SLOT(markModified()));
	}

	QList<QComboBox*> comboBoxes(widget->findChildren<QComboBox*>());

	for (int i = 0; i < comboBoxes.count(); ++i)
	{
		connect(comboBoxes.at(i), SIGNAL(currentIndexChanged(int)), this, SLOT(markModified()));
	}

	QList<ItemViewWidget*> viewWidgets(widget->findChildren<ItemViewWidget*>());

	for (int i = 0; i < viewWidgets.count(); ++i)
	{
		connect(viewWidgets.at(i), SIGNAL(modified()), this, SLOT(markModified()));
	}
}

void PreferencesDialog::markModified()
{
	m_ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
}

void PreferencesDialog::openConfigurationManager()
{
	const QUrl url(QLatin1String("about:config"));

	if (!SessionsManager::hasUrl(url, true))
	{
		QVariantMap parameters;
		parameters[QLatin1String("url")] = url;

		ActionsManager::triggerAction(ActionsManager::OpenUrlAction, this, parameters);
	}
}

void PreferencesDialog::save()
{
	emit requestedSave();

	if (sender() == m_ui->buttonBox)
	{
		close();
	}
	else
	{
		m_ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
	}
}

}
