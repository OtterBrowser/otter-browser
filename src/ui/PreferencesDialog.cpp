/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../core/Application.h"
#include "../core/SessionsManager.h"

#include "ui_PreferencesDialog.h"

#include <QtWidgets/QComboBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpinBox>

namespace Otter
{

PreferencesDialog::PreferencesDialog(const QString &section, QWidget *parent) : Dialog(parent),
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

	connect(m_ui->tabWidget, &QTabWidget::currentChanged, this, &PreferencesDialog::currentTabChanged);
	connect(m_ui->buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &PreferencesDialog::save);
	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &PreferencesDialog::save);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &PreferencesDialog::close);
	connect(m_ui->allSettingsButton, &QPushButton::clicked, this, &PreferencesDialog::openConfigurationManager);
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

				connect(this, &PreferencesDialog::requestedSave, pageWidget, &PreferencesGeneralPageWidget::save);
				connect(pageWidget, &PreferencesGeneralPageWidget::settingsModified, this, &PreferencesDialog::markAsModified);
			}

			break;
		case 1:
			{
				PreferencesContentPageWidget *pageWidget(new PreferencesContentPageWidget(this));

				m_ui->contentLayout->addWidget(pageWidget);

				connect(this, &PreferencesDialog::requestedSave, pageWidget, &PreferencesContentPageWidget::save);
				connect(pageWidget, &PreferencesContentPageWidget::settingsModified, this, &PreferencesDialog::markAsModified);
			}

			break;
		case 2:
			{
				PreferencesPrivacyPageWidget *pageWidget(new PreferencesPrivacyPageWidget(this));

				m_ui->privacyLayout->addWidget(pageWidget);

				connect(this, &PreferencesDialog::requestedSave, pageWidget, &PreferencesPrivacyPageWidget::save);
				connect(pageWidget, &PreferencesPrivacyPageWidget::settingsModified, this, &PreferencesDialog::markAsModified);
			}

			break;
		case 3:
			{
				PreferencesSearchPageWidget *pageWidget(new PreferencesSearchPageWidget(this));

				m_ui->searchLayout->addWidget(pageWidget);

				connect(this, &PreferencesDialog::requestedSave, pageWidget, &PreferencesSearchPageWidget::save);
				connect(pageWidget, &PreferencesSearchPageWidget::settingsModified, this, &PreferencesDialog::markAsModified);
			}

			break;
		default:
			{
				PreferencesAdvancedPageWidget *pageWidget(new PreferencesAdvancedPageWidget(this));

				m_ui->advancedLayout->addWidget(pageWidget);

				connect(this, &PreferencesDialog::requestedSave, pageWidget, &PreferencesAdvancedPageWidget::save);
				connect(pageWidget, &PreferencesAdvancedPageWidget::settingsModified, this, &PreferencesDialog::markAsModified);
			}

			break;
	}

	QWidget *widget(m_ui->tabWidget->widget(tab));
	const QList<QAbstractButton*> buttons(widget->findChildren<QAbstractButton*>());

	for (int i = 0; i < buttons.count(); ++i)
	{
		connect(buttons.at(i), &QAbstractButton::toggled, this, &PreferencesDialog::markAsModified);
	}

	const QList<QComboBox*> comboBoxes(widget->findChildren<QComboBox*>());

	for (int i = 0; i < comboBoxes.count(); ++i)
	{
		connect(comboBoxes.at(i), static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &PreferencesDialog::markAsModified);
	}

	const QList<QLineEdit*> lineEdits(widget->findChildren<QLineEdit*>());

	for (int i = 0; i < lineEdits.count(); ++i)
	{
		connect(lineEdits.at(i), &QLineEdit::textChanged, this, &PreferencesDialog::markAsModified);
	}

	const QList<QSpinBox*> spinBoxes(widget->findChildren<QSpinBox*>());

	for (int i = 0; i < spinBoxes.count(); ++i)
	{
		connect(spinBoxes.at(i), static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PreferencesDialog::markAsModified);
	}

	const QList<ItemViewWidget*> viewWidgets(widget->findChildren<ItemViewWidget*>());

	for (int i = 0; i < viewWidgets.count(); ++i)
	{
		connect(viewWidgets.at(i), &ItemViewWidget::modified, this, &PreferencesDialog::markAsModified);
	}
}

void PreferencesDialog::markAsModified()
{
	m_ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
}

void PreferencesDialog::openConfigurationManager()
{
	const QUrl url(QLatin1String("about:config"));

	if (!SessionsManager::hasUrl(url, true))
	{
		Application::triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}}, this);
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
