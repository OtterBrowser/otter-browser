/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 - 2021 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "PreferencesContentsWidget.h"
#include "PreferencesAdvancedPageWidget.h"
#include "PreferencesContentPageWidget.h"
#include "PreferencesGeneralPageWidget.h"
#include "PreferencesPrivacyPageWidget.h"
#include "PreferencesSearchPageWidget.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/ItemViewWidget.h"

#include "ui_PreferencesContentsWidget.h"

#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpinBox>

namespace Otter
{

PreferencesContentsWidget::PreferencesContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent) : ContentsWidget(parameters, window, parent),
	m_ui(new Ui::PreferencesContentsWidget)
{
	m_ui->setupUi(this);

	m_loadedTabs.fill(false, 5);

	showTab(GeneralTab);

	connect(m_ui->tabWidget, &QTabWidget::currentChanged, this, &PreferencesContentsWidget::showTab);
}

PreferencesContentsWidget::~PreferencesContentsWidget()
{
	delete m_ui;
}

void PreferencesContentsWidget::changeEvent(QEvent *event)
{
	ContentsWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void PreferencesContentsWidget::markAsModified()
{
///TODO
}

void PreferencesContentsWidget::showTab(int tab)
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
				PreferencesContentPageWidget *pageWidget(new PreferencesContentPageWidget(this));

				m_ui->contentLayout->addWidget(pageWidget);

				connect(this, &PreferencesContentsWidget::requestedSave, pageWidget, &PreferencesContentPageWidget::save);
				connect(pageWidget, &PreferencesContentPageWidget::settingsModified, this, &PreferencesContentsWidget::markAsModified);
			}

			break;
		case PrivacyTab:
			{
				PreferencesPrivacyPageWidget *pageWidget(new PreferencesPrivacyPageWidget(this));

				m_ui->privacyLayout->addWidget(pageWidget);

				connect(this, &PreferencesContentsWidget::requestedSave, pageWidget, &PreferencesPrivacyPageWidget::save);
				connect(pageWidget, &PreferencesPrivacyPageWidget::settingsModified, this, &PreferencesContentsWidget::markAsModified);
			}

			break;
		case SearchTab:
			{
				PreferencesSearchPageWidget *pageWidget(new PreferencesSearchPageWidget(this));

				m_ui->searchLayout->addWidget(pageWidget);

				connect(this, &PreferencesContentsWidget::requestedSave, pageWidget, &PreferencesSearchPageWidget::save);
				connect(pageWidget, &PreferencesSearchPageWidget::settingsModified, this, &PreferencesContentsWidget::markAsModified);
			}

			break;
		case AdvancedTab:
			{
				PreferencesAdvancedPageWidget *pageWidget(new PreferencesAdvancedPageWidget(this));

				m_ui->advancedLayout->addWidget(pageWidget);

				connect(this, &PreferencesContentsWidget::requestedSave, pageWidget, &PreferencesAdvancedPageWidget::save);
				connect(pageWidget, &PreferencesAdvancedPageWidget::settingsModified, this, &PreferencesContentsWidget::markAsModified);
			}

			break;
		default:
			{
				PreferencesGeneralPageWidget *pageWidget(new PreferencesGeneralPageWidget(this));

				m_ui->generalLayout->addWidget(pageWidget);

				connect(this, &PreferencesContentsWidget::requestedSave, pageWidget, &PreferencesGeneralPageWidget::save);
				connect(pageWidget, &PreferencesGeneralPageWidget::settingsModified, this, &PreferencesContentsWidget::markAsModified);
			}

			break;
	}

	QWidget *widget(m_ui->tabWidget->widget(tab));
	const QList<QAbstractButton*> buttons(widget->findChildren<QAbstractButton*>());

	for (int i = 0; i < buttons.count(); ++i)
	{
		connect(buttons.at(i), &QAbstractButton::toggled, this, &PreferencesContentsWidget::markAsModified);
	}

	const QList<QComboBox*> comboBoxes(widget->findChildren<QComboBox*>());

	for (int i = 0; i < comboBoxes.count(); ++i)
	{
		connect(comboBoxes.at(i), static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &PreferencesContentsWidget::markAsModified);
	}

	const QList<QLineEdit*> lineEdits(widget->findChildren<QLineEdit*>());

	for (int i = 0; i < lineEdits.count(); ++i)
	{
		connect(lineEdits.at(i), &QLineEdit::textChanged, this, &PreferencesContentsWidget::markAsModified);
	}

	const QList<QSpinBox*> spinBoxes(widget->findChildren<QSpinBox*>());

	for (int i = 0; i < spinBoxes.count(); ++i)
	{
		connect(spinBoxes.at(i), static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PreferencesContentsWidget::markAsModified);
	}

	const QList<ItemViewWidget*> viewWidgets(widget->findChildren<ItemViewWidget*>());

	for (int i = 0; i < viewWidgets.count(); ++i)
	{
		connect(viewWidgets.at(i), &ItemViewWidget::modified, this, &PreferencesContentsWidget::markAsModified);
	}
}

QString PreferencesContentsWidget::getTitle() const
{
	return tr("Preferences");
}

QLatin1String PreferencesContentsWidget::getType() const
{
	return QLatin1String("preferences");
}

QUrl PreferencesContentsWidget::getUrl() const
{
	return QUrl(QLatin1String("about:preferences"));
}

QIcon PreferencesContentsWidget::getIcon() const
{
	return ThemesManager::createIcon(QLatin1String("configuration"), false);
}

}
