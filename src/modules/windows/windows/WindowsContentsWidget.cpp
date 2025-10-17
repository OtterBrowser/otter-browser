/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "WindowsContentsWidget.h"
#include "../../../core/Application.h"
#include "../../../core/SessionModel.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/Action.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/Window.h"

#include "ui_WindowsContentsWidget.h"

#include <QtGui/QPainter>
#include <QtWidgets/QMenu>

namespace Otter
{

int EntryItemDelegate::m_decorationSize = -1;

EntryItemDelegate::EntryItemDelegate(QObject *parent) : ItemDelegate(parent)
{
	connect(ThemesManager::getInstance(), &ThemesManager::widgetStyleChanged, [&]()
	{
		m_decorationSize = -1;
	});
}

void EntryItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	ItemDelegate::paint(painter, option, index);

	if (!index.data(SessionModel::IsAudibleRole).toBool() && !index.data(SessionModel::IsAudioMutedRole).toBool())
	{
		return;
	}

	QStyleOptionViewItem mutableOption(option);
	mutableOption.features |= QStyleOptionViewItem::HasDecoration;

	QRect rectangle(QApplication::style()->subElementRect(QStyle::SE_ItemViewItemText, &mutableOption));
	const int availableWidth(rectangle.width());
	const int decorationWidth(calculateDecorationWidth(&mutableOption, index));

	if (availableWidth > (decorationWidth + 100))
	{
		const int offset((decorationWidth - option.decorationSize.width()) / 2);

		if (option.direction == Qt::RightToLeft)
		{
			rectangle.setWidth(decorationWidth);
		}
		else
		{
			rectangle.setLeft(rectangle.right() - decorationWidth);
		}

		ThemesManager::createIcon(index.data(SessionModel::IsAudioMutedRole).toBool() ? QLatin1String("audio-volume-muted") : QLatin1String("audio-volume-medium")).paint(painter, rectangle.adjusted(offset, 0, -offset, 0));
	}
}

void EntryItemDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
	ItemDelegate::initStyleOption(option, index);

	if (index.data(SessionModel::IsActiveRole).toBool())
	{
		option->font.setBold(true);
	}

	if (index.data(SessionModel::IsAudibleRole).toBool() || index.data(SessionModel::IsAudioMutedRole).toBool())
	{
		const int availableWidth(QApplication::style()->subElementRect(QStyle::SE_ItemViewItemText, option).width());
		const int decorationWidth(calculateDecorationWidth(option, index));

		if (availableWidth > (decorationWidth + 100))
		{
			option->text = Utils::elideText(option->text, QFontMetrics(option->font), nullptr, (availableWidth - decorationWidth));
		}
	}
}

int EntryItemDelegate::calculateDecorationWidth(QStyleOptionViewItem *option, const QModelIndex &index) const
{
	if (m_decorationSize < 0)
	{
		QStyleOptionViewItem mutableOption(*option);

		ItemDelegate::initStyleOption(&mutableOption, index);

		mutableOption.features &= ~QStyleOptionViewItem::HasDecoration;

		const int width(QApplication::style()->subElementRect(QStyle::SE_ItemViewItemText, &mutableOption).width());

		mutableOption.features |= QStyleOptionViewItem::HasDecoration;

		m_decorationSize = (width - QApplication::style()->subElementRect(QStyle::SE_ItemViewItemText, &mutableOption).width());
	}

	return m_decorationSize;
}

WindowsContentsWidget::WindowsContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent) : ContentsWidget(parameters, window, parent),
	m_ui(new Ui::WindowsContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->filterLineEditWidget->setClearOnEscape(true);
	m_ui->windowsViewWidget->setViewMode(ItemViewWidget::TreeView);
	m_ui->windowsViewWidget->setModel(SessionsManager::getModel());
	m_ui->windowsViewWidget->setItemDelegate(new EntryItemDelegate(m_ui->windowsViewWidget));
	m_ui->windowsViewWidget->setFilterRoles({SessionModel::TitleRole, SessionModel::UrlRole});
	m_ui->windowsViewWidget->expandAll();
	m_ui->windowsViewWidget->viewport()->setMouseTracking(true);

	connect(SessionsManager::getModel(), &SessionModel::rowsInserted, this, [&](const QModelIndex &index)
	{
		m_ui->windowsViewWidget->setExpanded(index, true);
	});
	connect(SessionsManager::getModel(), &SessionModel::modelModified, m_ui->windowsViewWidget->viewport(), static_cast<void(QWidget::*)()>(&QWidget::update));
	connect(m_ui->filterLineEditWidget, &LineEditWidget::textChanged, m_ui->windowsViewWidget, &ItemViewWidget::setFilterString);
	connect(m_ui->windowsViewWidget, &ItemViewWidget::customContextMenuRequested, this, &WindowsContentsWidget::showContextMenu);
	connect(m_ui->windowsViewWidget, &ItemViewWidget::clicked, this, &WindowsContentsWidget::activateWindow);
}

WindowsContentsWidget::~WindowsContentsWidget()
{
	delete m_ui;
}

void WindowsContentsWidget::changeEvent(QEvent *event)
{
	ContentsWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void WindowsContentsWidget::print(QPrinter *printer)
{
	m_ui->windowsViewWidget->render(printer);
}

void WindowsContentsWidget::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	switch (identifier)
	{
		case ActionsManager::SelectAllAction:
			m_ui->windowsViewWidget->selectAll();

			break;
		case ActionsManager::ActivateContentAction:
			m_ui->windowsViewWidget->setFocus();

			break;
		default:
			ContentsWidget::triggerAction(identifier, parameters, trigger);

			break;
	}
}

void WindowsContentsWidget::activateWindow(const QModelIndex &index)
{
	switch (static_cast<SessionModel::EntityType>(index.data(SessionModel::TypeRole).toInt()))
	{
		case SessionModel::MainWindowEntity:
			Application::getInstance()->triggerAction(ActionsManager::ActivateWindowAction, {{QLatin1String("window"), index.data(SessionModel::IdentifierRole).toULongLong()}});

			break;
		case SessionModel::WindowEntity:
			{
				const WindowSessionItem *windowItem(static_cast<WindowSessionItem*>(m_ui->windowsViewWidget->getSourceModel()->itemFromIndex(index)));
				const Window *window(windowItem ? windowItem->getActiveWindow() : nullptr);

				if (window)
				{
					const MainWindow *mainWindow(window->getMainWindow());

					if (mainWindow)
					{
						Application::getInstance()->triggerAction(ActionsManager::ActivateWindowAction, {{QLatin1String("window"), mainWindow->getIdentifier()}});
					}

					Application::triggerAction(ActionsManager::ActivateTabAction, {{QLatin1String("tab"), index.data(SessionModel::IdentifierRole).toULongLong()}}, window->getMainWindow());
				}
			}

			break;
		default:
			break;
	}
}

void WindowsContentsWidget::showContextMenu(const QPoint &position)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(this));
	const QModelIndex index(m_ui->windowsViewWidget->indexAt(position));
	ActionExecutor::Object executor(mainWindow, mainWindow);
	QMenu menu(this);
	menu.addAction(new Action(ActionsManager::NewWindowAction, {}, executor, &menu));
	menu.addAction(new Action(ActionsManager::NewWindowPrivateAction, {}, executor, &menu));

	if (!index.data(SessionModel::IsTrashedRole).toBool())
	{
		int closeAction(ActionsManager::CloseTabAction);

		executor = ActionExecutor::Object();

		switch (static_cast<SessionModel::EntityType>(index.data(SessionModel::TypeRole).toInt()))
		{
			case SessionModel::MainWindowEntity:
				{
					const MainWindowSessionItem *mainWindowItem(static_cast<MainWindowSessionItem*>(SessionsManager::getModel()->itemFromIndex(index)));

					if (mainWindowItem)
					{
						executor = ActionExecutor::Object(mainWindowItem->getMainWindow(), mainWindowItem->getMainWindow());
						closeAction = ActionsManager::CloseWindowAction;
					}
				}

				break;
			case SessionModel::WindowEntity:
				{
					const WindowSessionItem *windowItem(static_cast<WindowSessionItem*>(SessionsManager::getModel()->itemFromIndex(index)));

					if (windowItem)
					{
						Window *window(windowItem->getActiveWindow());

						executor = ActionExecutor::Object(window->getMainWindow(), window->getMainWindow());
					}
				}

				break;
			default:
				break;
		}

		if (executor.isValid())
		{
			menu.addAction(new Action(ActionsManager::NewTabAction, {}, executor, &menu));
			menu.addAction(new Action(ActionsManager::NewTabPrivateAction, {}, executor, &menu));
			menu.addSeparator();
			menu.addAction(new Action(closeAction, {}, executor, &menu));
		}
	}

	menu.exec(m_ui->windowsViewWidget->mapToGlobal(position));
}

QString WindowsContentsWidget::getTitle() const
{
	return tr("Windows and Tabs");
}

QLatin1String WindowsContentsWidget::getType() const
{
	return QLatin1String("windows");
}

QUrl WindowsContentsWidget::getUrl() const
{
	return {QLatin1String("about:windows")};
}

QIcon WindowsContentsWidget::getIcon() const
{
	return ThemesManager::createIcon(QLatin1String("tab"), false);
}

}
