/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ErrorConsoleWidget.h"
#include "../../../core/Application.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/Window.h"

#include "ui_ErrorConsoleWidget.h"

#if QT_VERSION >= 0x060000
#include <QtGui/QActionGroup>
#endif
#include <QtGui/QClipboard>
#if QT_VERSION < 0x060000
#include <QtWidgets/QActionGroup>
#endif
#include <QtWidgets/QMenu>

namespace Otter
{

ErrorConsoleWidget::ErrorConsoleWidget(QWidget *parent) : QWidget(parent),
	m_model(nullptr),
	m_messageScopes(AllTabsScope | OtherSourcesScope),
	m_ui(new Ui::ErrorConsoleWidget)
{
	m_ui->setupUi(this);
	m_ui->closeButton->setIcon(ThemesManager::createIcon(QLatin1String("window-close")));

	const ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (toolBar)
	{
		const int identifier(toolBar->getIdentifier());

		connect(m_ui->closeButton, &QToolButton::clicked, [=]()
		{
			Application::getInstance()->triggerAction(ActionsManager::ShowToolBarAction, {{QLatin1String("toolBar"), identifier}, {QLatin1String("isChecked"), false}});
		});
	}
	else
	{
		m_ui->closeButton->hide();
	}

	QMenu *menu(new QMenu(m_ui->scopeButton));
	QAction *allTabsAction(menu->addAction(tr("All Tabs")));
	allTabsAction->setData(AllTabsScope);
	allTabsAction->setCheckable(true);
	allTabsAction->setChecked(true);

	QAction *currentTabAction(menu->addAction(tr("Current Tab Only")));
	currentTabAction->setData(CurrentTabScope);
	currentTabAction->setCheckable(true);

	menu->addSeparator();

	QAction *otherSourcesAction(menu->addAction(tr("Other Sources")));
	otherSourcesAction->setData(OtherSourcesScope);
	otherSourcesAction->setCheckable(true);
	otherSourcesAction->setChecked(true);

	QActionGroup *actionGroup(new QActionGroup(menu));
	actionGroup->setExclusive(true);
	actionGroup->addAction(allTabsAction);
	actionGroup->addAction(currentTabAction);

	m_ui->scopeButton->setMenu(menu);

	connect(menu, &QMenu::triggered, this, &ErrorConsoleWidget::filterCategories);
	connect(m_ui->networkButton, &QToolButton::clicked, this, &ErrorConsoleWidget::filterCategories);
	connect(m_ui->securityButton, &QToolButton::clicked, this, &ErrorConsoleWidget::filterCategories);
	connect(m_ui->cssButton, &QToolButton::clicked, this, &ErrorConsoleWidget::filterCategories);
	connect(m_ui->javaScriptButton, &QToolButton::clicked, this, &ErrorConsoleWidget::filterCategories);
	connect(m_ui->otherButton, &QToolButton::clicked, this, &ErrorConsoleWidget::filterCategories);
	connect(m_ui->clearButton, &QToolButton::clicked, this, [&]()
	{
		if (m_model)
		{
			m_model->clear();
		}
	});
	connect(m_ui->filterLineEditWidget, &LineEditWidget::textChanged, this, &ErrorConsoleWidget::filterMessages);
	connect(m_ui->consoleView, &QTreeView::customContextMenuRequested, this, &ErrorConsoleWidget::showContextMenu);
}

ErrorConsoleWidget::~ErrorConsoleWidget()
{
	delete m_ui;
}

void ErrorConsoleWidget::showEvent(QShowEvent *event)
{
	if (!m_model)
	{
		const MainWindow *mainWindow(MainWindow::findMainWindow(this));

		if (mainWindow)
		{
			connect(mainWindow, &MainWindow::activeWindowChanged, this, &ErrorConsoleWidget::filterCategories);
		}

		m_model = new QStandardItemModel(this);
		m_model->setSortRole(TimeRole);

		const QVector<Console::Message> messages(Console::getMessages());

		for (int i = 0; i < messages.count(); ++i)
		{
			addMessage(messages.at(i));
		}

		m_ui->consoleView->setModel(m_model);

		connect(Console::getInstance(), &Console::messageAdded, this, &ErrorConsoleWidget::addMessage);
	}

	QWidget::showEvent(event);
}

void ErrorConsoleWidget::addMessage(const Console::Message &message)
{
	if (!m_model)
	{
		return;
	}

	QIcon icon;
	QString category;

	switch (message.level)
	{
		case Console::ErrorLevel:
			icon = ThemesManager::createIcon(QLatin1String("dialog-error"));

			break;
		case Console::WarningLevel:
			icon = ThemesManager::createIcon(QLatin1String("dialog-warning"));

			break;
		default:
			icon = ThemesManager::createIcon(QLatin1String("dialog-information"));

			break;
	}

	switch (message.category)
	{
		case Console::NetworkCategory:
			category = tr("Network");

			break;
		case Console::SecurityCategory:
			category = tr("Security");

			break;
		case Console::JavaScriptCategory:
			category = tr("JS");

			break;
		default:
			category = tr("Other");

			break;
	}

	const QString source(message.source + ((message.line > 0) ? QStringLiteral(":%1").arg(message.line) : QString()));
	const QString description(message.note.isEmpty() ? tr("<empty>") : message.note);
	QString entry(QStringLiteral("[%1] %2").arg(message.time.toLocalTime().toString(QLatin1String("yyyy-dd-MM hh:mm:ss")), category));

	if (!message.source.isEmpty())
	{
		entry.append(QLatin1String(" - ") + source);
	}

	QStandardItem *messageItem(new QStandardItem(icon, entry));
	messageItem->setData(entry, Qt::ToolTipRole);
	messageItem->setData(message.time.toMSecsSinceEpoch(), TimeRole);
	messageItem->setData(message.category, CategoryRole);
	messageItem->setData(source, SourceRole);
	messageItem->setData(message.window, WindowRole);

	QStandardItem *descriptionItem(new QStandardItem(description));
	descriptionItem->setData(description, Qt::ToolTipRole);
	descriptionItem->setFlags(descriptionItem->flags() | Qt::ItemNeverHasChildren);

	messageItem->appendRow(descriptionItem);

	m_model->appendRow(messageItem);
	m_model->sort(0, Qt::DescendingOrder);

	applyFilters(messageItem->index(), m_ui->filterLineEditWidget->text(), getCategories(), getActiveWindow());
}

void ErrorConsoleWidget::filterCategories()
{
	QMenu *menu(qobject_cast<QMenu*>(sender()));

	if (menu)
	{
		MessagesScopes messageScopes(NoScope);

		for (int i = 0; i < menu->actions().count(); ++i)
		{
			const QAction *action(menu->actions().at(i));

			if (action && action->isChecked())
			{
				messageScopes |= static_cast<MessagesScope>(action->data().toInt());
			}
		}

		m_messageScopes = messageScopes;
	}

	applyFilters(m_ui->filterLineEditWidget->text(), getCategories(), getActiveWindow());
}

void ErrorConsoleWidget::filterMessages(const QString &filter)
{
	if (m_model)
	{
		applyFilters(filter, getCategories(), getActiveWindow());
	}
}

void ErrorConsoleWidget::applyFilters(const QString &filter, const QVector<Console::MessageCategory> &categories, quint64 activeWindow)
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		applyFilters(m_model->index(i, 0), filter, categories, activeWindow);
	}
}

void ErrorConsoleWidget::applyFilters(const QModelIndex &index, const QString &filter, const QVector<Console::MessageCategory> &categories, quint64 activeWindow)
{
	bool hasMatch(true);

	if (!filter.isEmpty() && !(index.data(SourceRole).toString().contains(filter, Qt::CaseInsensitive) || m_model->index(0, 0, index).data(Qt::DisplayRole).toString().contains(filter, Qt::CaseInsensitive)))
	{
		hasMatch = false;
	}
	else
	{
		const quint64 identifier(index.data(WindowRole).toULongLong());

		hasMatch = (((identifier == 0 && m_messageScopes.testFlag(OtherSourcesScope)) || (identifier > 0 && ((identifier == activeWindow && m_messageScopes.testFlag(CurrentTabScope)) || m_messageScopes.testFlag(AllTabsScope)))) && categories.contains(static_cast<Console::MessageCategory>(index.data(CategoryRole).toInt())));
	}

	m_ui->consoleView->setRowHidden(index.row(), m_ui->consoleView->rootIndex(), !hasMatch);
}

void ErrorConsoleWidget::showContextMenu(const QPoint &position)
{
	QMenu menu(m_ui->consoleView);
	menu.addAction(ThemesManager::createIcon(QLatin1String("edit-copy")), tr("Copy"), this, [&]()
	{
		QGuiApplication::clipboard()->setText(m_ui->consoleView->currentIndex().data(Qt::DisplayRole).toString());
	});
	menu.addSeparator();
	menu.addAction(tr("Expand All"), m_ui->consoleView, &QTreeView::expandAll);
	menu.addAction(tr("Collapse All"), m_ui->consoleView, &QTreeView::collapseAll);
	menu.exec(m_ui->consoleView->mapToGlobal(position));
}

QVector<Console::MessageCategory> ErrorConsoleWidget::getCategories() const
{
	QVector<Console::MessageCategory> categories;

	if (m_ui->networkButton->isChecked())
	{
		categories.append(Console::NetworkCategory);
	}

	if (m_ui->securityButton->isChecked())
	{
		categories.append(Console::SecurityCategory);
	}

	if (m_ui->cssButton->isChecked())
	{
		categories.append(Console::CssCategory);
	}

	if (m_ui->javaScriptButton->isChecked())
	{
		categories.append(Console::JavaScriptCategory);
	}

	if (m_ui->otherButton->isChecked())
	{
		categories.append(Console::OtherCategory);
	}

	return categories;
}

quint64 ErrorConsoleWidget::getActiveWindow()
{
	const MainWindow *mainWindow(MainWindow::findMainWindow(this));
	const Window *window(mainWindow ? mainWindow->getActiveWindow() : nullptr);

	return (window ? window->getIdentifier() : 0);
}

}
