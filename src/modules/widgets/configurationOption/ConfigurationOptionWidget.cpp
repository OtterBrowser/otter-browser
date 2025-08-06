/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../../../core/SettingsManager.h"
#include "../../../ui/OptionWidget.h"
#include "../../../ui/ToolBarWidget.h"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>

namespace Otter
{

ConfigurationOptionWidget::ConfigurationOptionWidget(Window *window, const ToolBarsManager::ToolBarDefinition::Entry &definition, QWidget *parent) : QWidget(parent),
	m_optionWidget(nullptr),
	m_window(window),
	m_scope(WindowScope),
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

	if (definition.options.value(QLatin1String("scope")).toString() == QLatin1String("global"))
	{
		m_scope = GlobalScope;
	}

	QString text;

	if (definition.options.contains(QLatin1String("text")))
	{
		text = definition.options.value(QLatin1String("text")).toString();
	}
	else
	{
		text = definition.options.value(QLatin1String("optionName")).toString().section(QLatin1Char('/'), -1);
	}

	layout->addWidget(new QLabel(text, this));

	const SettingsManager::OptionDefinition optionDefinition(SettingsManager::getOptionDefinition(m_identifier));
	const QVariant value((m_scope == GlobalScope || !m_window) ? SettingsManager::getOption(m_identifier) : m_window->getOption(m_identifier));

	m_optionWidget = new OptionWidget(value, optionDefinition.type, this);
	m_optionWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

	if (optionDefinition.type == SettingsManager::EnumerationType)
	{
		m_optionWidget->setChoices(optionDefinition.choices);
	}

	layout->addWidget(m_optionWidget);

	if (m_scope == WindowScope)
	{
		setWindow(window);

		const ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

		if (toolBar && toolBar->getDefinition().isGlobal())
		{
			connect(toolBar, &ToolBarWidget::windowChanged, this, &ConfigurationOptionWidget::setWindow);
		}

		connect(SettingsManager::getInstance(), &SettingsManager::hostOptionChanged, this, [&](int option)
		{
			if (option == m_identifier && m_window)
			{
				m_optionWidget->setValue(m_window->getOption(m_identifier));
			}
		});
	}

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &ConfigurationOptionWidget::handleOptionChanged);
	connect(m_optionWidget, &OptionWidget::commitData, this, &ConfigurationOptionWidget::save);
}

void ConfigurationOptionWidget::handleOptionChanged(int option, const QVariant &value)
{
	if (option == m_identifier)
	{
		m_optionWidget->setValue(value);
	}
}

void ConfigurationOptionWidget::setWindow(Window *window)
{
	if (m_window && !m_window->isAboutToClose())
	{
		disconnect(m_window, &Window::optionChanged, this, &ConfigurationOptionWidget::handleOptionChanged);
	}

	m_window = window;

	m_optionWidget->setEnabled(m_window != nullptr);
	m_optionWidget->setValue(m_window ? m_window->getOption(m_identifier) : SettingsManager::getOption(m_identifier));

	if (window)
	{
		connect(window, &Window::optionChanged, this, &ConfigurationOptionWidget::handleOptionChanged);
	}
}

void ConfigurationOptionWidget::save()
{
	if (m_scope == GlobalScope)
	{
		SettingsManager::setOption(m_identifier, m_optionWidget->getValue());
	}
	else if (m_window)
	{
		m_window->setOption(m_identifier, m_optionWidget->getValue());
	}
}

}
