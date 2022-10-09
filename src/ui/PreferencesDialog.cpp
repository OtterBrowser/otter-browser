/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2022 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../modules/windows/preferences/AdvancedPreferencesPage.h"
#include "../modules/windows/preferences/ContentPreferencesPage.h"
#include "../modules/windows/preferences/GeneralPreferencesPage.h"
#include "../modules/windows/preferences/InputPreferencesPage.h"
#include "../modules/windows/preferences/PrivacyPreferencesPage.h"
#include "../modules/windows/preferences/SearchPreferencesPage.h"
#include "../modules/windows/preferences/WebsitesPreferencesPage.h"
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

	m_loadedTabs.fill(false, 7);

	int tab(GeneralTab);

	if (section == QLatin1String("content"))
	{
		tab = ContentTab;
	}
	else if (section == QLatin1String("privacy"))
	{
		tab = PrivacyTab;
	}
	else if (section == QLatin1String("search"))
	{
		tab = SearchTab;
	}
	else if (section == QLatin1String("input"))
	{
		tab = InputTab;
	}
	else if (section == QLatin1String("websites"))
	{
		tab = WebsitesTab;
	}
	else if (section == QLatin1String("advanced"))
	{
		tab = AdvancedTab;
	}

	showTab(tab);

	m_ui->tabWidget->setCurrentIndex(tab);
	m_ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);

	connect(m_ui->tabWidget, &QTabWidget::currentChanged, this, &PreferencesDialog::showTab);
	connect(m_ui->buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, [&]()
	{
		m_ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);

		emit requestedSave();
	});
	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, [&]()
	{
		close();

		emit requestedSave();
	});
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &PreferencesDialog::close);
	connect(m_ui->allSettingsButton, &QPushButton::clicked, this,[&]()
	{
		accept();

		const QUrl url(QLatin1String("about:config"));

		if (!SessionsManager::hasUrl(url, true))
		{
			Application::triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}}, this);
		}
	});
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

void PreferencesDialog::showTab(int tab)
{
	if (tab < GeneralTab || tab > AdvancedTab || m_loadedTabs.value(tab))
	{
		return;
	}

	m_loadedTabs[tab] = true;

	switch (tab)
	{
		case ContentTab:
			{
				ContentPreferencesPage *page(new ContentPreferencesPage(this));

				m_ui->contentLayout->addWidget(page);

				connect(this, &PreferencesDialog::requestedSave, page, &ContentPreferencesPage::save);
				connect(page, &ContentPreferencesPage::settingsModified, this, &PreferencesDialog::markAsModified);
			}

			break;
		case PrivacyTab:
			{
				PrivacyPreferencesPage *page(new PrivacyPreferencesPage(this));

				m_ui->privacyLayout->addWidget(page);

				connect(this, &PreferencesDialog::requestedSave, page, &PrivacyPreferencesPage::save);
				connect(page, &PrivacyPreferencesPage::settingsModified, this, &PreferencesDialog::markAsModified);
			}

			break;
		case SearchTab:
			{
				SearchPreferencesPage *page(new SearchPreferencesPage(this));

				m_ui->searchLayout->addWidget(page);

				connect(this, &PreferencesDialog::requestedSave, page, &SearchPreferencesPage::save);
				connect(page, &SearchPreferencesPage::settingsModified, this, &PreferencesDialog::markAsModified);
			}

			break;
		case InputTab:
			{
				InputPreferencesPage *page(new InputPreferencesPage(this));

				m_ui->inputLayout->addWidget(page);

				connect(this, &PreferencesDialog::requestedSave, page, &InputPreferencesPage::save);
				connect(page, &InputPreferencesPage::settingsModified, this, &PreferencesDialog::markAsModified);
			}

			break;
		case WebsitesTab:
			{
				WebsitesPreferencesPage *page(new WebsitesPreferencesPage(this));

				m_ui->websitesLayout->addWidget(page);

				connect(this, &PreferencesDialog::requestedSave, page, &WebsitesPreferencesPage::save);
				connect(page, &WebsitesPreferencesPage::settingsModified, this, &PreferencesDialog::markAsModified);
			}

			break;
		case AdvancedTab:
			{
				AdvancedPreferencesPage *page(new AdvancedPreferencesPage(this));

				m_ui->advancedLayout->addWidget(page);

				connect(this, &PreferencesDialog::requestedSave, page, &AdvancedPreferencesPage::save);
				connect(page, &AdvancedPreferencesPage::settingsModified, this, &PreferencesDialog::markAsModified);
			}

			break;
		default:
			{
				GeneralPreferencesPage *page(new GeneralPreferencesPage(this));

				m_ui->generalLayout->addWidget(page);

				connect(this, &PreferencesDialog::requestedSave, page, &GeneralPreferencesPage::save);
				connect(page, &GeneralPreferencesPage::settingsModified, this, &PreferencesDialog::markAsModified);
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

}
