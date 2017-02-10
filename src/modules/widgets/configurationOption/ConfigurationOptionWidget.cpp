/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2017 Piotr Wójcik <chocimier@tlen.pl>
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

#include "ConfigurationOptionWidget.h"
#include "../../../core/ActionsManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../ui/OptionWidget.h"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>

namespace Otter
{

ConfigurationOptionWidget::ConfigurationOptionWidget(const ActionsManager::ActionEntryDefinition &definition, QWidget *parent) : QWidget(parent),
	m_optionWidget(nullptr),
	m_identifier(SettingsManager::getOptionIdentifier(definition.options.value(QLatin1String("optionName")).toString()))
{
	QHBoxLayout *layout(new QHBoxLayout(this));
	layout->setContentsMargins(0, 0, 0, 0);

	setLayout(layout);

	if (m_identifier == -1)
	{
		layout->addWidget(new QLabel(tr("Choose option"), this));

		return;
	}

	QString text;
	const QString key(definition.options.value(QLatin1String("optionName")).toString());

	if (definition.options.contains(QLatin1String("text")))
	{
		text = definition.options.value(QLatin1String("text")).toString();
	}
	else
	{
		text = key.section(QLatin1Char('/'), -1);
	}

	layout->addWidget(new QLabel(text, this));

	const SettingsManager::OptionDefinition optionDefinition(SettingsManager::getOptionDefinition(m_identifier));

	m_optionWidget = new OptionWidget(key, SettingsManager::getValue(m_identifier), optionDefinition.type, this);
	m_optionWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

	if (optionDefinition.type == SettingsManager::EnumerationType)
	{
		m_optionWidget->setChoices(optionDefinition.choices);
	}

	layout->addWidget(m_optionWidget);

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(int,QVariant)), this, SLOT(optionChanged(int,QVariant)));
	connect(m_optionWidget, SIGNAL(commitData(QWidget*)), this, SLOT(save()));
}

void ConfigurationOptionWidget::optionChanged(int option, const QVariant &value)
{
	if (option == m_identifier)
	{
		m_optionWidget->setValue(value);
	}
}

void ConfigurationOptionWidget::save()
{
	SettingsManager::setValue(m_identifier, m_optionWidget->getValue());
}

}
