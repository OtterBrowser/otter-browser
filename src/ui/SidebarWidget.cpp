/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2015 Piotr Wójcik <chocimier@tlen.pl>
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

#include "SidebarWidget.h"
#include "ContentsWidget.h"
#include "MainWindow.h"
#include "OpenAddressDialog.h"
#include "ResizerWidget.h"
#include "ToolBarWidget.h"
#include "WidgetFactory.h"
#include "../core/ActionsManager.h"
#include "../core/AddonsManager.h"
#include "../core/BookmarksModel.h"
#include "../core/HistoryManager.h"
#include "../core/ThemesManager.h"
#include "../modules/widgets/action/ActionWidget.h"
#include "../modules/widgets/panelChooser/PanelChooserWidget.h"

#include "ui_SidebarWidget.h"

#include <QtWidgets/QMenu>

namespace Otter
{

SidebarWidget::SidebarWidget(ToolBarWidget *parent) : QWidget(parent),
	m_toolBarWidget(parent),
	m_resizerWidget(nullptr),
	m_ui(new Ui::SidebarWidget)
{
	m_ui->setupUi(this);

	m_resizerWidget = new ResizerWidget(m_ui->containerWidget, this);

	ToolBarsManager::ToolBarDefinition::Entry definition;
	definition.parameters[QLatin1String("sidebar")] = m_toolBarWidget->getIdentifier();

	QToolBar *toolbar(new QToolBar(this));
	toolbar->setIconSize(QSize(16, 16));
	toolbar->addWidget(new PanelChooserWidget(definition, this));
	toolbar->addWidget(new ActionWidget(ActionsManager::OpenPanelAction, nullptr, definition, this));

	QWidget *spacer(new QWidget(toolbar));
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	toolbar->addWidget(spacer);
	toolbar->addWidget(new ActionWidget(ActionsManager::ShowPanelAction, nullptr, definition, this));

	m_ui->panelLayout->addWidget(toolbar);
	m_ui->panelsButton->setPopupMode(QToolButton::InstantPopup);
	m_ui->panelsButton->setIcon(ThemesManager::createIcon(QLatin1String("list-add")));
	m_ui->horizontalLayout->addWidget(m_resizerWidget);

	const int panelSize(m_toolBarWidget->getDefinition().panelSize);

	if (panelSize > 0)
	{
		m_ui->containerWidget->setMaximumWidth(panelSize);
	}

	updateLayout();
	updatePanels();

	connect(parent, &ToolBarWidget::toolBarModified, this, &SidebarWidget::updateLayout);
	connect(parent, &ToolBarWidget::toolBarModified, this, &SidebarWidget::updatePanels);
	connect(m_resizerWidget, &ResizerWidget::finished, this, &SidebarWidget::saveSize);
}

SidebarWidget::~SidebarWidget()
{
	delete m_ui;
}

void SidebarWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			{
				m_ui->retranslateUi(this);

				QHash<QString, QToolButton*>::iterator iterator;

				for (iterator = m_buttons.begin(); iterator != m_buttons.end(); ++iterator)
				{
					const QKeySequence shortcut(ActionsManager::getActionShortcut(ActionsManager::ShowPanelAction, {{QLatin1String("panel"), iterator.key()}}));

					iterator.value()->setToolTip(getPanelTitle(iterator.key()) + (shortcut.isEmpty() ? QString() : QLatin1String(" (") + shortcut.toString(QKeySequence::NativeText) + QLatin1Char(')')));
				}

				if (m_ui->panelsButton->menu())
				{
					const QList<QAction*> actions(m_ui->panelsButton->menu()->actions());

					for (int i = 0; i < actions.count(); ++i)
					{
						if (!actions.at(i)->data().toString().isEmpty())
						{
							const QString panel(actions.at(i)->data().toString());

							if (panel.startsWith(QLatin1String("web:")))
							{
								actions[i]->setText(Utils::elideText(getPanelTitle(panel), m_ui->panelsButton->menu()->fontMetrics(), nullptr, 300));
							}
							else
							{
								actions[i]->setText(getPanelTitle(panel));
							}
						}
					}
				}
			}

			break;
		case QEvent::LayoutDirectionChange:
			updateLayout();

			break;
		default:
			break;
	}
}

void SidebarWidget::reload()
{
	const int panelSize(m_toolBarWidget->getDefinition().panelSize);

	if (panelSize > 0)
	{
		m_ui->containerWidget->setMaximumWidth(panelSize);
	}

	updateLayout();
	updatePanels();
}

void SidebarWidget::addWebPanel()
{
	const MainWindow *mainWindow(MainWindow::findMainWindow(this));
	OpenAddressDialog dialog(ActionExecutor::Object(), this);
	dialog.setWindowTitle(tr("Add web panel"));

	if (mainWindow)
	{
		dialog.setText(mainWindow->getUrl().toString(QUrl::RemovePassword));
	}

	QUrl url;

	if (dialog.exec() == QDialog::Accepted && dialog.getResult().isValid())
	{
		switch (dialog.getResult().type)
		{
			case InputInterpreter::InterpreterResult::BookmarkType:
				url = dialog.getResult().bookmark->getUrl();

				break;
			case InputInterpreter::InterpreterResult::UrlType:
				url = dialog.getResult().url;

				break;
			default:
				break;
		}
	}

	if (!url.isEmpty())
	{
		ToolBarsManager::ToolBarDefinition definition(m_toolBarWidget->getDefinition());
		definition.panels.append(QLatin1String("web:") + url.toString());

		ToolBarsManager::setToolBar(definition);
	}
}

void SidebarWidget::choosePanel(bool isChecked)
{
	const QAction *action(qobject_cast<QAction*>(sender()));

	if (!action)
	{
		return;
	}

	ToolBarsManager::ToolBarDefinition definition(m_toolBarWidget->getDefinition());
	const QString panel(action->data().toString());

	if (isChecked)
	{
		definition.panels.append(panel);
	}
	else
	{
		definition.panels.removeAll(panel);

		if (panel == definition.currentPanel)
		{
			definition.currentPanel.clear();
		}
	}

	ToolBarsManager::setToolBar(definition);
}

void SidebarWidget::selectPanel(const QString &identifier)
{
	if (!identifier.isEmpty() && identifier == m_currentPanel)
	{
		return;
	}

	ToolBarsManager::ToolBarDefinition definition(m_toolBarWidget->getDefinition());
	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	ContentsWidget *contentsWidget(nullptr);

	if (!identifier.isEmpty() && definition.panels.contains(identifier))
	{
		contentsWidget = ((m_panels.contains(identifier) && m_panels[identifier]) ? m_panels[identifier] : WidgetFactory::createSidebarPanel(identifier, m_toolBarWidget->getIdentifier(), mainWindow, this));
	}

	if (contentsWidget && mainWindow)
	{
		connect(contentsWidget, &ContentsWidget::requestedSearch, mainWindow, &MainWindow::search);
		connect(contentsWidget, &ContentsWidget::requestedNewWindow, mainWindow, &MainWindow::openWindow);
	}

	if (m_panels.contains(m_currentPanel) && m_panels[m_currentPanel])
	{
		m_ui->panelLayout->removeWidget(m_panels[m_currentPanel]);

		m_panels[m_currentPanel]->hide();
	}

	if (m_buttons.contains(m_currentPanel) && m_buttons[m_currentPanel])
	{
		m_buttons[m_currentPanel]->setChecked(false);
	}

	if (contentsWidget)
	{
		m_ui->panelLayout->addWidget(contentsWidget);

		contentsWidget->show();

		m_panels[identifier] = contentsWidget;

		if (m_buttons.contains(identifier) && m_buttons[identifier])
		{
			m_buttons[identifier]->setChecked(true);
		}
	}

	m_ui->containerWidget->setVisible(contentsWidget != nullptr);

	m_resizerWidget->setVisible(contentsWidget != nullptr);

	m_currentPanel = (definition.panels.contains(identifier) ? identifier : QString());

	if (m_currentPanel != definition.currentPanel)
	{
		definition.currentPanel = m_currentPanel;

		ToolBarsManager::setToolBar(definition);
	}
}

void SidebarWidget::saveSize()
{
	ToolBarsManager::ToolBarDefinition definition(m_toolBarWidget->getDefinition());
	int size(m_ui->containerWidget->maximumWidth());

	if (size == QWIDGETSIZE_MAX)
	{
		size = -1;
	}

	if (size != definition.panelSize)
	{
		definition.panelSize = size;

		ToolBarsManager::setToolBar(definition);
	}
}

void SidebarWidget::updateLayout()
{
	QToolBar *toolbar(findChild<QToolBar*>());
	const Qt::ToolBarArea area(m_toolBarWidget->getArea());
	QBoxLayout::Direction direction((area == Qt::RightToolBarArea) ? QBoxLayout::RightToLeft : QBoxLayout::LeftToRight);

	m_ui->toggleSpacer->changeSize((m_toolBarWidget->getDefinition().hasToggle ? 6 : 0), 0, QSizePolicy::Fixed);

	m_resizerWidget->setDirection((area == Qt::RightToolBarArea) ? ResizerWidget::RightToLeftDirection : ResizerWidget::LeftToRightDirection);

	if (QGuiApplication::isRightToLeft())
	{
		direction = ((direction == QBoxLayout::LeftToRight) ? QBoxLayout::RightToLeft : QBoxLayout::LeftToRight);
	}

	m_ui->horizontalLayout->setDirection(direction);

	if (!toolbar)
	{
		return;
	}

	toolbar->setLayoutDirection((area == Qt::LeftToolBarArea) ? Qt::LeftToRight : Qt::RightToLeft);

	const QList<QWidget*> widgets(toolbar->findChildren<QWidget*>());

	for (int i = 0; i < widgets.count(); ++i)
	{
		widgets[i]->setLayoutDirection(QGuiApplication::isLeftToRight() ? Qt::LeftToRight : Qt::RightToLeft);

		ActionWidget *widget(qobject_cast<ActionWidget*>(widgets.at(i)));

		if (widget && widget->getIdentifier() == ActionsManager::OpenPanelAction)
		{
			QVariantMap options(widget->getOptions());
			options[QLatin1String("icon")] = ((area == Qt::LeftToolBarArea) ? QLatin1String("arrow-right") : QLatin1String("arrow-left"));

			widget->setOptions(options);
		}
	}
}

void SidebarWidget::updatePanels()
{
	const ToolBarsManager::ToolBarDefinition definition(m_toolBarWidget->getDefinition());

	if (m_buttons.keys().toSet() == definition.panels.toSet())
	{
		selectPanel(definition.currentPanel);

		return;
	}

	const QStringList panels(definition.panels);

	qDeleteAll(m_buttons.begin(), m_buttons.end());

	m_buttons.clear();

	const QStringList currentPanels(m_panels.keys());

	for (int i = 0; i < currentPanels.count(); ++i)
	{
		const QString panel(currentPanels.at(i));

		if (!panels.contains(panel))
		{
			ContentsWidget *widget(m_panels[panel]);
			widget->hide();

			if (panel == m_currentPanel)
			{
				m_currentPanel.clear();

				m_ui->panelLayout->removeWidget(widget);
				m_ui->containerWidget->hide();

				m_resizerWidget->hide();
			}

			widget->deleteLater();

			m_panels.remove(panel);
		}
	}

	const QStringList specialPages(AddonsManager::getSpecialPages(AddonsManager::SpecialPageInformation::SidebarPanelType));
	QMenu *menu(nullptr);

	if (m_ui->panelsButton->menu())
	{
		menu = m_ui->panelsButton->menu();
		menu->clear();
	}
	else
	{
		menu = new QMenu(m_ui->panelsButton);

		m_ui->panelsButton->setMenu(menu);
	}

	for (int i = 0; i < specialPages.count(); ++i)
	{
		QAction *action(menu->addAction(getPanelTitle(specialPages.at(i))));
		action->setCheckable(true);
		action->setChecked(panels.contains(specialPages.at(i)));
		action->setData(specialPages.at(i));
		action->setShortcut(ActionsManager::getActionShortcut(ActionsManager::ShowPanelAction, {{QLatin1String("panel"), specialPages.at(i)}}));
		action->setShortcutContext(Qt::WidgetShortcut);

		connect(action, &QAction::toggled, this, &SidebarWidget::choosePanel);
	}

	menu->addSeparator();

	for (int i = 0; i < panels.count(); ++i)
	{
		const bool isWebPanel(panels.at(i).startsWith(QLatin1String("web:")));

		if (!isWebPanel && !specialPages.contains(panels.at(i)))
		{
			continue;
		}

		const QKeySequence shortcut(ActionsManager::getActionShortcut(ActionsManager::ShowPanelAction, {{QLatin1String("panel"), panels.at(i)}}));
		const QString title(getPanelTitle(panels.at(i)));
		QToolButton *button(new QToolButton(this));
		QAction *selectPanelButtonAction(new QAction(button));
		selectPanelButtonAction->setData(panels.at(i));
		selectPanelButtonAction->setIcon(getPanelIcon(panels.at(i)));
		selectPanelButtonAction->setToolTip(title + (shortcut.isEmpty() ? QString() : QLatin1String(" (") + shortcut.toString(QKeySequence::NativeText) + QLatin1Char(')')));

		button->setDefaultAction(selectPanelButtonAction);
		button->setAutoRaise(true);
		button->setCheckable(true);

		if (isWebPanel)
		{
			QAction *selectPanelMenuAction(menu->addAction(Utils::elideText(title, menu->fontMetrics(), nullptr, 300)));
			selectPanelMenuAction->setCheckable(true);
			selectPanelMenuAction->setChecked(true);
			selectPanelMenuAction->setData(panels.at(i));

			connect(selectPanelMenuAction, &QAction::toggled, this, &SidebarWidget::choosePanel);
		}

		m_ui->buttonsLayout->insertWidget(qMax(0, (m_ui->buttonsLayout->count() - 2)), button);

		m_buttons[panels.at(i)] = button;

		connect(selectPanelButtonAction, &QAction::triggered, this, [&]()
		{
			const QAction *action(qobject_cast<QAction*>(sender()));

			if (action)
			{
				selectPanel((action->data().toString() == m_currentPanel) ? QString() : action->data().toString());
			}
		});
	}

	menu->addSeparator();
	menu->addAction(tr("Add Web Panel…"), this, &SidebarWidget::addWebPanel);

	selectPanel(definition.currentPanel);
}

QString SidebarWidget::getPanelTitle(const QString &identifier)
{
	if (identifier.startsWith(QLatin1String("web:")))
	{
		return identifier.mid(4);
	}

	if (AddonsManager::getSpecialPages(AddonsManager::SpecialPageInformation::SidebarPanelType).contains(identifier))
	{
		return AddonsManager::getSpecialPage(identifier).getTitle();
	}

	return identifier;
}

QUrl SidebarWidget::getPanelUrl(const QString &identifier)
{
	if (identifier.startsWith(QLatin1String("web:")))
	{
		return QUrl(identifier.mid(4));
	}

	if (AddonsManager::getSpecialPages(AddonsManager::SpecialPageInformation::SidebarPanelType).contains(identifier))
	{
		return AddonsManager::getSpecialPage(identifier).url;
	}

	return {};
}

QIcon SidebarWidget::getPanelIcon(const QString &identifier)
{
	if (identifier.startsWith(QLatin1String("web:")))
	{
		return HistoryManager::getIcon(QUrl(identifier.mid(4)));
	}

	if (AddonsManager::getSpecialPages(AddonsManager::SpecialPageInformation::SidebarPanelType).contains(identifier))
	{
		return AddonsManager::getSpecialPage(identifier).icon;
	}

	return {};
}

QSize SidebarWidget::sizeHint() const
{
	if (m_currentPanel.isEmpty())
	{
		return m_ui->buttonsLayout->sizeHint();
	}

	return QSize((m_ui->buttonsLayout->sizeHint().width() + ((m_ui->containerWidget->maximumWidth() < QWIDGETSIZE_MAX) ? m_ui->containerWidget->maximumWidth() : 250)), m_ui->buttonsLayout->sizeHint().height());
}

}
