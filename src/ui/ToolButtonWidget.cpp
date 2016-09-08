/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "ToolButtonWidget.h"
#include "Menu.h"
#include "ToolBarWidget.h"
#include "../core/ActionsManager.h"
#include "../core/ThemesManager.h"

#include <QtCore/QEvent>
#include <QtWidgets/QStyleOptionToolButton>
#include <QtWidgets/QStylePainter>

namespace Otter
{

ToolButtonWidget::ToolButtonWidget(const ActionsManager::ActionEntryDefinition &definition, QWidget *parent) : QToolButton(parent),
	m_isCustomized(false)
{
	setAutoRaise(true);
	setContextMenuPolicy(Qt::NoContextMenu);
	setOptions(definition.options);

	Menu *menu(NULL);

	if (!definition.entries.isEmpty())
	{
		menu = new Menu(Menu::NoMenuRole, this);

		addMenu(menu, definition.entries);
		setMenu(menu);
	}
	else if (definition.action.endsWith(QLatin1String("Menu")))
	{
		menu = new Menu(Menu::getRole(definition.action), this);

		setDefaultAction(menu->menuAction());
	}

	if (menu)
	{
		setPopupMode(QToolButton::InstantPopup);
		setText(definition.options.value(QLatin1String("text"), tr("Menu")).toString());
	}

	ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (toolBar)
	{
		setButtonStyle(toolBar->getButtonStyle());
		setIconSize(toolBar->getIconSize());
		setMaximumButtonSize(toolBar->getMaximumButtonSize());

		connect(toolBar, SIGNAL(buttonStyleChanged(Qt::ToolButtonStyle)), this, SLOT(setButtonStyle(Qt::ToolButtonStyle)));
		connect(toolBar, SIGNAL(iconSizeChanged(int)), this, SLOT(setIconSize(int)));
		connect(toolBar, SIGNAL(maximumButtonSizeChanged(int)), this, SLOT(setMaximumButtonSize(int)));
	}
}

void ToolButtonWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QStylePainter painter(this);
	QStyleOptionToolButton option;

	initStyleOption(&option);

	if (m_isCustomized)
	{
		if (m_options.contains(QLatin1String("icon")))
		{
			option.icon = m_options[QLatin1String("icon")].value<QIcon>();
		}

		if (m_options.contains(QLatin1String("text")))
		{
			option.text = m_options[QLatin1String("text")].toString();
		}
	}

	option.text = option.fontMetrics.elidedText(option.text, Qt::ElideRight, (option.rect.width() - (option.fontMetrics.width(QLatin1Char(' ')) * 2) - ((toolButtonStyle() == Qt::ToolButtonTextBesideIcon) ? iconSize().width() : 0)));

	painter.drawComplexControl(QStyle::CC_ToolButton, option);
}

void ToolButtonWidget::addMenu(Menu *menu, const QList<ActionsManager::ActionEntryDefinition> &entries)
{
	for (int i = 0; i < entries.count(); ++i)
	{
		if (entries.at(i).entries.isEmpty())
		{
			if (entries.at(i).action.isEmpty() || entries.at(i).action == QLatin1String("separator"))
			{
				menu->addSeparator();
			}
			else
			{
				menu->addAction(ActionsManager::getActionIdentifier(entries.at(i).action), true);
			}
		}
		else
		{
			Menu *subMenu(new Menu());
			Action *subMenuAction(menu->addAction(-1));
			subMenuAction->setText(entries.at(i).options.value(QLatin1String("text"), tr("Menu")).toString());
			subMenuAction->setMenu(subMenu);

			addMenu(subMenu, entries.at(i).entries);
		}
	}
}

void ToolButtonWidget::setOptions(const QVariantMap &options)
{
	m_options = options;
	m_isCustomized = (options.contains(QLatin1String("icon")) || options.contains(QLatin1String("text")));

	if (m_isCustomized && options.contains(QLatin1String("icon")))
	{
		const QString data(options[QLatin1String("icon")].toString());

		if (data.startsWith(QLatin1String("data:image/")))
		{
			m_options[QLatin1String("icon")] = QIcon(QPixmap::fromImage(QImage::fromData(QByteArray::fromBase64(data.mid(data.indexOf(QLatin1String("base64,")) + 7).toUtf8()))));
		}
		else
		{
			m_options[QLatin1String("icon")] = ThemesManager::getIcon(data);
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

QVariantMap ToolButtonWidget::getOptions() const
{
	return m_options;
}

bool ToolButtonWidget::isCustomized() const
{
	return m_isCustomized;
}

bool ToolButtonWidget::event(QEvent *event)
{
	if (event->type() == QEvent::ContextMenu && contextMenuPolicy() == Qt::NoContextMenu)
	{
		return false;
	}

	return QToolButton::event(event);
}

}
