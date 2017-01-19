/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ConsoleWidget.h"
#include "MainWindow.h"
#include "Window.h"
#include "WorkspaceWidget.h"
#include "../core/ThemesManager.h"

#include "ui_ConsoleWidget.h"

#include <QtGui/QClipboard>
#include <QtWidgets/QActionGroup>
#include <QtWidgets/QMenu>

namespace Otter
{

ConsoleWidget::ConsoleWidget(QWidget *parent) : QWidget(parent),
	m_model(nullptr),
	m_messageScopes(AllTabsScope | OtherSourcesScope),
	m_ui(new Ui::ConsoleWidget)
{
	m_ui->setupUi(this);

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

	connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(filterCategories()));
	connect(m_ui->networkButton, SIGNAL(clicked()), this, SLOT(filterCategories()));
	connect(m_ui->securityButton, SIGNAL(clicked()), this, SLOT(filterCategories()));
	connect(m_ui->cssButton, SIGNAL(clicked()), this, SLOT(filterCategories()));
	connect(m_ui->javaScriptButton, SIGNAL(clicked()), this, SLOT(filterCategories()));
	connect(m_ui->otherButton, SIGNAL(clicked()), this, SLOT(filterCategories()));
	connect(m_ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));
	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterMessages(QString)));
	connect(m_ui->consoleView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
}

ConsoleWidget::~ConsoleWidget()
{
	delete m_ui;
}

void ConsoleWidget::showEvent(QShowEvent *event)
{
	if (!m_model)
	{
		MainWindow *mainWindow(MainWindow::findMainWindow(this));

		if (mainWindow)
		{
			connect(mainWindow->getWindowsManager(), SIGNAL(currentWindowChanged(quint64)), this, SLOT(filterCategories()));
		}

		m_model = new QStandardItemModel(this);
		m_model->setSortRole(TimeRole);

		const QList<Console::Message> messages(Console::getMessages());

		for (int i = 0; i < messages.count(); ++i)
		{
			addMessage(messages.at(i));
		}

		m_ui->consoleView->setModel(m_model);

		connect(Console::getInstance(), SIGNAL(messageAdded(Console::Message)), this, SLOT(addMessage(Console::Message)));
	}

	QWidget::showEvent(event);
}

void ConsoleWidget::addMessage(const Console::Message &message)
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
			icon = ThemesManager::getIcon(QLatin1String("dialog-error"));

			break;
		case Console::WarningLevel:
			icon = ThemesManager::getIcon(QLatin1String("dialog-warning"));

			break;
		default:
			icon = ThemesManager::getIcon(QLatin1String("dialog-information"));

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
	QString entry(QStringLiteral("[%1] %2").arg(message.time.toString()).arg(category));

	if (!message.source.isEmpty())
	{
		entry.append(QStringLiteral(" - %1").arg(source));
	}

	QStandardItem *item(new QStandardItem(icon, entry));
	item->setData(message.time.toTime_t(), TimeRole);
	item->setData(message.category, CategoryRole);
	item->setData(source, SourceRole);
	item->setData(message.window, WindowRole);

	if (!message.note.isEmpty())
	{
		item->appendRow(new QStandardItem(message.note));
	}

	m_model->appendRow(item);
	m_model->sort(0, Qt::DescendingOrder);

	applyFilters(item, m_ui->filterLineEdit->text(), getCategories(), getCurrentWindow());
}

void ConsoleWidget::clear()
{
	if (m_model)
	{
		m_model->clear();
	}
}

void ConsoleWidget::copyText()
{
	QApplication::clipboard()->setText(m_ui->consoleView->currentIndex().data(Qt::DisplayRole).toString());
}

void ConsoleWidget::filterCategories()
{
	QMenu *menu(qobject_cast<QMenu*>(sender()));

	if (menu)
	{
		MessagesScopes messageScopes(NoScope);

		for (int i = 0; i < menu->actions().count(); ++i)
		{
			if (menu->actions().at(i) && menu->actions().at(i)->isChecked())
			{
				messageScopes |= static_cast<MessagesScope>(menu->actions().at(i)->data().toInt());
			}
		}

		m_messageScopes = messageScopes;
	}

	const QList<Console::MessageCategory> categories(getCategories());
	const quint64 currentWindow(getCurrentWindow());

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		applyFilters(m_model->item(i, 0), m_ui->filterLineEdit->text(), categories, currentWindow);
	}
}

void ConsoleWidget::filterMessages(const QString &filter)
{
	if (m_model)
	{
		const QList<Console::MessageCategory> categories(getCategories());
		const quint64 currentWindow(getCurrentWindow());

		for (int i = 0; i < m_model->rowCount(); ++i)
		{
			applyFilters(m_model->item(i, 0), filter, categories, currentWindow);
		}
	}
}

void ConsoleWidget::applyFilters(QStandardItem *item, const QString &filter, const QList<Console::MessageCategory> &categories, quint64 currentWindow)
{
	if (!item)
	{
		return;
	}

	bool matched(true);

	if (!filter.isEmpty() && !(item->data(SourceRole).toString().contains(filter, Qt::CaseInsensitive) || (item->child(0, 0) && item->child(0, 0)->text().contains(filter, Qt::CaseInsensitive))))
	{
		matched = false;
	}
	else
	{
		const quint64 window(item->data(WindowRole).toULongLong());

		matched = (((window == 0 && m_messageScopes.testFlag(OtherSourcesScope)) || (window > 0 && ((window == currentWindow && m_messageScopes.testFlag(CurrentTabScope)) || m_messageScopes.testFlag(AllTabsScope)))) && categories.contains(static_cast<Console::MessageCategory>(item->data(CategoryRole).toInt())));
	}

	m_ui->consoleView->setRowHidden(item->row(), m_ui->consoleView->rootIndex(), !matched);
}

void ConsoleWidget::showContextMenu(const QPoint position)
{
	QMenu menu(m_ui->consoleView);
	menu.addAction(ThemesManager::getIcon(QLatin1String("edit-copy")), tr("Copy"), this, SLOT(copyText()));
	menu.exec(m_ui->consoleView->mapToGlobal(position));
}

QList<Console::MessageCategory> ConsoleWidget::getCategories() const
{
	QList<Console::MessageCategory> categories;

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

quint64 ConsoleWidget::getCurrentWindow()
{
	MainWindow *mainWindow(MainWindow::findMainWindow(this));
	Window *currentWindow(mainWindow ? mainWindow->getWorkspace()->getActiveWindow() : nullptr);

	return (currentWindow ? currentWindow->getIdentifier() : 0);
}

}
