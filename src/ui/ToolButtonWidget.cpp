/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ToolButtonWidget.h"
#include "Action.h"
#include "MainWindow.h"
#include "Menu.h"
#include "ToolBarWidget.h"
#include "../core/Application.h"
#include "../core/ThemesManager.h"

#include <QtCore/QEvent>
#include <QtGui/QActionEvent>
#include <QtWidgets/QStyleOptionToolButton>
#include <QtWidgets/QStylePainter>

namespace Otter
{

ToolButtonWidget::ToolButtonWidget(const ToolBarsManager::ToolBarDefinition::Entry &definition, QWidget *parent) : QToolButton(parent),
	m_parameters(definition.parameters),
	m_isCustomized(false)
{
	setAutoRaise(true);
	setContextMenuPolicy(Qt::NoContextMenu);
	setOptions(definition.options);

	Menu *menu(nullptr);

	if (!definition.entries.isEmpty())
	{
		menu = new Menu(Menu::UnknownMenu, this);

		addMenu(menu, definition.entries);
		setMenu(menu);
	}
	else if (definition.action == QLatin1String("OptionMenu") && definition.options.contains(QLatin1String("option")))
	{
		menu = new Menu(Menu::UnknownMenu, this);
		menu->load(SettingsManager::getOptionIdentifier(definition.options[QLatin1String("option")].toString()));

		setDefaultAction(menu->menuAction());
	}
	else if (definition.action.endsWith(QLatin1String("Menu")))
	{
		menu = new Menu(Menu::getMenuRoleIdentifier(definition.action), this);

		setDefaultAction(menu->menuAction());
	}

	if (menu)
	{
		menu->setActionParameters(definition.parameters);
		menu->setMenuOptions(definition.options);

		setPopupMode(QToolButton::InstantPopup);
		setText(getText());
		setIcon(getIcon());
	}

	const ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (toolBar)
	{
		setButtonStyle(toolBar->getButtonStyle());
		setIconSize(toolBar->getIconSize());
		setMaximumButtonSize(toolBar->getMaximumButtonSize());

		connect(toolBar, &ToolBarWidget::buttonStyleChanged, this, &ToolButtonWidget::setButtonStyle);
		connect(toolBar, &ToolBarWidget::iconSizeChanged, this, &ToolButtonWidget::setIconSize);
		connect(toolBar, &ToolBarWidget::maximumButtonSizeChanged, this, &ToolButtonWidget::setMaximumButtonSize);
	}
}

void ToolButtonWidget::actionEvent(QActionEvent *event)
{
	QToolButton::actionEvent(event);

	if (event->type() == QEvent::ActionChanged || event->type() == QEvent::ActionAdded)
	{
		setText(getText());
		setIcon(getIcon());
	}
}

void ToolButtonWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QStylePainter painter(this);
	QStyleOptionToolButton option;

	initStyleOption(&option);

	option.text = option.fontMetrics.elidedText(option.text, (isRightToLeft() ? Qt::ElideLeft : Qt::ElideRight), (option.rect.width() - (option.fontMetrics.width(QLatin1Char(' ')) * 2) - ((toolButtonStyle() == Qt::ToolButtonTextBesideIcon) ? iconSize().width() : 0)));

	painter.drawComplexControl(QStyle::CC_ToolButton, option);
}

void ToolButtonWidget::addMenu(Menu *menu, const QVector<ToolBarsManager::ToolBarDefinition::Entry> &entries)
{
	const ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parentWidget()));
	ActionExecutor::Object executor(Application::getInstance(), Application::getInstance());

	if (toolBar && toolBar->getMainWindow())
	{
		executor = ActionExecutor::Object(toolBar->getMainWindow(), toolBar->getMainWindow());
	}

	for (int i = 0; i < entries.count(); ++i)
	{
		const ToolBarsManager::ToolBarDefinition::Entry entry(entries.at(i));

		if (entry.entries.isEmpty())
		{
			if (entry.action.isEmpty() || entry.action == QLatin1String("separator"))
			{
				menu->addSeparator();
			}
			else
			{
				menu->addAction(new Action(ActionsManager::getActionIdentifier(entry.action), entry.parameters, entry.options, executor, menu));
			}
		}
		else
		{
			Menu *subMenu(new Menu());
			QAction *subMenuAction(new QAction(menu));
			subMenuAction->setText(entry.options.value(QLatin1String("text"), tr("Menu")).toString());
			subMenuAction->setMenu(subMenu);

			menu->addAction(subMenuAction);

			addMenu(subMenu, entry.entries);
		}
	}
}

void ToolButtonWidget::setOptions(const QVariantMap &options)
{
	m_options = options;
	m_isCustomized = (options.contains(QLatin1String("icon")) || options.contains(QLatin1String("text")));

	if (m_isCustomized)
	{
		if (options.contains(QLatin1String("text")))
		{
			setText(getText());
		}

		if (options.contains(QLatin1String("icon")))
		{
			setIcon(getIcon());
		}
	}

	setButtonStyle(toolButtonStyle());
	update();
}

void ToolButtonWidget::setButtonStyle(Qt::ToolButtonStyle buttonStyle)
{
	if (m_options.contains(QLatin1String("buttonStyle")))
	{
		const QString buttonStyleString(m_options[QLatin1String("buttonStyle")].toString());

		if (buttonStyleString == QLatin1String("auto"))
		{
			buttonStyle = Qt::ToolButtonFollowStyle;
		}
		else if (buttonStyleString == QLatin1String("textOnly"))
		{
			buttonStyle = Qt::ToolButtonTextOnly;
		}
		else if (buttonStyleString == QLatin1String("textBesideIcon"))
		{
			buttonStyle = Qt::ToolButtonTextBesideIcon;
		}
		else if (buttonStyleString == QLatin1String("textUnderIcon"))
		{
			buttonStyle = Qt::ToolButtonTextUnderIcon;
		}
		else
		{
			buttonStyle = Qt::ToolButtonIconOnly;
		}
	}

	setToolButtonStyle(buttonStyle);
}

void ToolButtonWidget::setIconSize(int size)
{
	QToolButton::setIconSize(QSize(size, size));
}

void ToolButtonWidget::setMaximumButtonSize(int size)
{
	if (size > 0)
	{
		setMaximumSize(size, size);
	}
	else
	{
		setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
	}
}

QString ToolButtonWidget::getText() const
{
	if (m_isCustomized && m_options.contains(QLatin1String("text")))
	{
		return m_options[QLatin1String("text")].toString();
	}

	if (defaultAction())
	{
		return defaultAction()->text();
	}

	return text();
}

QIcon ToolButtonWidget::getIcon() const
{
	if (m_isCustomized && m_options.contains(QLatin1String("icon")))
	{
		const QVariant iconData(m_options[QLatin1String("icon")]);

		if (iconData.type() == QVariant::Icon)
		{
			return iconData.value<QIcon>();
		}

		return ThemesManager::createIcon(iconData.toString());
	}

	return (defaultAction() ? defaultAction()->icon() : icon());
}

QVariantMap ToolButtonWidget::getOptions() const
{
	return m_options;
}

QVariantMap ToolButtonWidget::getParameters() const
{
	return m_parameters;
}

bool ToolButtonWidget::isCustomized() const
{
	return m_isCustomized;
}

bool ToolButtonWidget::event(QEvent *event)
{
	switch (event->type())
	{
		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonRelease:
			if (!isEnabled())
			{
				return false;
			}

			break;
		case QEvent::ContextMenu:
			if (contextMenuPolicy() == Qt::NoContextMenu)
			{
				return false;
			}

			break;
		default:
			break;
	}

	return QToolButton::event(event);
}

}
