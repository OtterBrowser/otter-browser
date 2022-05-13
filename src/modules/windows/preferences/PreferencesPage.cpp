/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2021 - 2022 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "PreferencesPage.h"
#include "../../../ui/ItemViewWidget.h"

#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpinBox>

namespace Otter
{

PreferencesPage::PreferencesPage(QWidget *parent) : CategoryPage(parent)
{
	connect(this, &PreferencesPage::loaded, this, [&]()
	{
		const QList<QAbstractButton*> buttons(findChildren<QAbstractButton*>());

		for (int i = 0; i < buttons.count(); ++i)
		{
			connect(buttons.at(i), &QAbstractButton::toggled, this, &PreferencesPage::settingsModified);
		}

		const QList<QComboBox*> comboBoxes(findChildren<QComboBox*>());

		for (int i = 0; i < comboBoxes.count(); ++i)
		{
			connect(comboBoxes.at(i), static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &PreferencesPage::settingsModified);
		}

		const QList<QLineEdit*> lineEdits(findChildren<QLineEdit*>());

		for (int i = 0; i < lineEdits.count(); ++i)
		{
			connect(lineEdits.at(i), &QLineEdit::textChanged, this, &PreferencesPage::settingsModified);
		}

		const QList<QSpinBox*> spinBoxes(findChildren<QSpinBox*>());

		for (int i = 0; i < spinBoxes.count(); ++i)
		{
			connect(spinBoxes.at(i), static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PreferencesPage::settingsModified);
		}

		const QList<ItemViewWidget*> viewWidgets(findChildren<ItemViewWidget*>());

		for (int i = 0; i < viewWidgets.count(); ++i)
		{
			connect(viewWidgets.at(i), &ItemViewWidget::modified, this, &PreferencesPage::settingsModified);
		}
	});
}

void PreferencesPage::save()
{
}

}
