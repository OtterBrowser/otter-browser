/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ActionWidget.h"
#include "../../../core/Application.h"
#include "../../../ui/Action.h"
#include "../../../ui/ContentsWidget.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/ToolBarWidget.h"
#include "../../../ui/Window.h"

#include <QtGui/QMouseEvent>
#include <QtWidgets/QToolTip>

namespace Otter
{

ActionWidget::ActionWidget(int identifier, Window *window, const ToolBarsManager::ToolBarDefinition::Entry &definition, QWidget *parent) : ToolButtonWidget(definition, parent),
	m_action(new Action(identifier, definition.parameters, definition.options, ActionExecutor::Object(window, window), this))
{
	setDefaultAction(m_action);
	setWindow(window);

	if (identifier == ActionsManager::NewTabAction || identifier == ActionsManager::NewTabPrivateAction)
	{
		setAcceptDrops(true);
	}

	const ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (toolBar && toolBar->getIdentifier() != ToolBarsManager::AddressBar)
	{
		connect(toolBar, &ToolBarWidget::windowChanged, this, &ActionWidget::setWindow);
	}
}

void ActionWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() != Qt::LeftButton)
	{
		ToolButtonWidget::mouseReleaseEvent(event);

		return;
	}

	int identifier(m_action->getIdentifier());
	QVariantMap parameters(m_action->getParameters());

	if (identifier == ActionsManager::NewTabAction || identifier == ActionsManager::NewTabPrivateAction)
	{
		SessionsManager::OpenHints hints(SessionsManager::calculateOpenHints(SessionsManager::NewTabOpen, event->button(), event->modifiers()));

		if (identifier == ActionsManager::NewTabPrivateAction)
		{
			hints |= SessionsManager::PrivateOpen;
		}

		parameters[QLatin1String("hints")] = QVariant(hints);

		identifier = ActionsManager::OpenUrlAction;
	}

	if (isCheckable())
	{
		parameters[QLatin1String("isChecked")] = !isChecked();
	}

	Application::triggerAction(identifier, parameters, this);

	setDefaultAction(nullptr);

	ToolButtonWidget::mouseReleaseEvent(event);

	setDefaultAction(m_action);
}

void ActionWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls())
	{
		event->accept();
	}
	else
	{
		event->ignore();
	}
}

void ActionWidget::dropEvent(QDropEvent *event)
{
	if (event->mimeData()->hasUrls())
	{
		QVariantMap parameters(getParameters());
		QVector<QUrl> urls(Utils::extractUrls(event->mimeData()));
		SessionsManager::OpenHints hints(SessionsManager::calculateOpenHints(SessionsManager::NewTabOpen, Qt::LeftButton, event->keyboardModifiers()));

		if (m_action->getIdentifier() == ActionsManager::NewTabPrivateAction)
		{
			hints |= SessionsManager::PrivateOpen;
		}

		parameters[QLatin1String("hints")] = QVariant(hints);

		for (int i = 0; i < urls.count(); ++i)
		{
			QVariantMap actionParameters(parameters);
			actionParameters[QLatin1String("url")] = urls.at(i);

			Application::triggerAction(ActionsManager::OpenUrlAction, actionParameters, this);
		}

		event->accept();
	}
	else
	{
		event->ignore();
	}
}

void ActionWidget::setWindow(Window *window)
{
	if (m_action->getDefinition().scope == ActionsManager::ActionDefinition::WindowScope)
	{
		m_action->setExecutor(ActionExecutor::Object(window, window));
	}
	else
	{
		MainWindow *mainWindow(MainWindow::findMainWindow(this));

		m_action->setExecutor(ActionExecutor::Object(mainWindow, mainWindow));
	}
}

int ActionWidget::getIdentifier() const
{
	return m_action->getIdentifier();
}

bool ActionWidget::event(QEvent *event)
{
	if (event->type() == QEvent::ToolTip)
	{
		const QHelpEvent *helpEvent(static_cast<QHelpEvent*>(event));

		if (helpEvent)
		{
			QToolTip::showText(helpEvent->globalPos(), text() + (m_action->shortcut().isEmpty() ? QString() : QLatin1String(" (") + m_action->shortcut().toString(QKeySequence::NativeText) + QLatin1Char(')')));
		}

		return true;
	}

	return ToolButtonWidget::event(event);
}

}
