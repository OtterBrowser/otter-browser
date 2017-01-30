/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "MainWindow.h"
#include "WidgetFactory.h"
#include "../core/ActionsManager.h"
#include "../core/AddonsManager.h"
#include "../core/HistoryManager.h"
#include "../core/SettingsManager.h"
#include "../core/ThemesManager.h"
#include "../core/WindowsManager.h"
#include "../modules/widgets/panelChooser/PanelChooserWidget.h"

#include "ui_SidebarWidget.h"

#include <QtGui/QIcon>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolBar>

namespace Otter
{

SidebarWidget::SidebarWidget(QWidget *parent) : QWidget(parent),
	m_resizeTimer(0),
	m_ui(new Ui::SidebarWidget)
{
	m_ui->setupUi(this);

	QToolBar *toolbar(new QToolBar(this));
	toolbar->setIconSize(QSize(16, 16));
	toolbar->addWidget(new PanelChooserWidget(ActionsManager::ActionEntryDefinition(), this));
	toolbar->addAction(ActionsManager::getAction(ActionsManager::OpenPanelAction, this));

	QWidget *spacer(new QWidget(toolbar));
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	toolbar->addWidget(spacer);
	toolbar->addAction(ActionsManager::getAction(ActionsManager::ClosePanelAction, this));

	m_ui->panelLayout->addWidget(toolbar);
	m_ui->panelsButton->setPopupMode(QToolButton::InstantPopup);
	m_ui->panelsButton->setIcon(ThemesManager::getIcon(QLatin1String("list-add")));

	optionChanged(SettingsManager::Sidebar_CurrentPanelOption, SettingsManager::getValue(SettingsManager::Sidebar_CurrentPanelOption));
	optionChanged(SettingsManager::Sidebar_PanelsOption, SettingsManager::getValue(SettingsManager::Sidebar_PanelsOption));
	optionChanged(SettingsManager::Sidebar_ReverseOption, SettingsManager::getValue(SettingsManager::Sidebar_ReverseOption));

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(int,QVariant)), this, SLOT(optionChanged(int,QVariant)));
}

SidebarWidget::~SidebarWidget()
{
	delete m_ui;
}

void SidebarWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_resizeTimer)
	{
		SettingsManager::setValue(SettingsManager::Sidebar_WidthOption, width());

		killTimer(m_resizeTimer);

		m_resizeTimer = 0;
	}
}

void SidebarWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		QHash<QString, QToolButton*>::iterator iterator;

		for (iterator = m_buttons.begin(); iterator != m_buttons.end(); ++iterator)
		{
			iterator.value()->setToolTip(getPanelTitle(iterator.key()));
		}

		if (m_ui->panelsButton->menu())
		{
			QList<QAction*> actions(m_ui->panelsButton->menu()->actions());

			for (int i = 0; i < actions.count(); ++i)
			{
				if (!actions[i]->data().toString().isEmpty())
				{
					actions[i]->setText(getPanelTitle(actions[i]->data().toString()));
				}
			}
		}
	}
}

void SidebarWidget::optionChanged(int identifier, const QVariant &value)
{
	if (identifier == SettingsManager::Sidebar_CurrentPanelOption)
	{
		selectPanel(value.toString());
	}
	else if (identifier == SettingsManager::Sidebar_PanelsOption)
	{
		qDeleteAll(m_buttons.begin(), m_buttons.end());

		m_buttons.clear();

		QMenu *menu(new QMenu(m_ui->panelsButton));
		const QStringList panels(value.toStringList());
		const QStringList specialPages(AddonsManager::getSpecialPages());

		for (int i = 0; i < specialPages.count(); ++i)
		{
			QAction *action(menu->addAction(getPanelTitle(specialPages.at(i))));
			action->setCheckable(true);
			action->setChecked(panels.contains(specialPages.at(i)));
			action->setData(specialPages.at(i));

			connect(action, SIGNAL(toggled(bool)), this, SLOT(choosePanel(bool)));
		}

		menu->addSeparator();

		for (int i = 0; i < panels.count(); ++i)
		{
			QToolButton *button(new QToolButton(this));
			QAction *action(new QAction(button));
			action->setData(panels.at(i));
			action->setToolTip(getPanelTitle(panels.at(i)));

			button->setDefaultAction(action);
			button->setAutoRaise(true);
			button->setCheckable(true);

			if (specialPages.contains(panels.at(i)))
			{
				button->setIcon(AddonsManager::getSpecialPage(panels.at(i)).icon);
			}
			else if (panels.at(i).startsWith(QLatin1String("web:")))
			{
				button->setIcon(HistoryManager::getIcon(QUrl(panels.at(i).mid(4))));

				QAction *action(menu->addAction(getPanelTitle(panels.at(i))));
				action->setCheckable(true);
				action->setChecked(true);
				action->setData(panels.at(i));

				connect(action, SIGNAL(toggled(bool)), this, SLOT(choosePanel(bool)));
			}
			else
			{
				button->deleteLater();

				continue;
			}

			m_ui->buttonsLayout->insertWidget(qMax(0, (m_ui->buttonsLayout->count() - 2)), button);

			m_buttons[panels.at(i)] = button;

			connect(button->defaultAction(), SIGNAL(triggered()), this, SLOT(selectPanel()));
		}

		menu->addSeparator();
		menu->addAction(tr("Add Web Panel…"), this, SLOT(addWebPanel()));

		if (m_ui->panelsButton->menu())
		{
			m_ui->panelsButton->menu()->deleteLater();
		}

		m_ui->panelsButton->setMenu(menu);
	}
	else if (identifier == SettingsManager::Sidebar_ReverseOption)
	{
		Qt::ToolBarArea area(QGuiApplication::isLeftToRight() ? Qt::LeftToolBarArea : Qt::RightToolBarArea);

		if (value.toBool())
		{
			if (area == Qt::LeftToolBarArea)
			{
				area = Qt::RightToolBarArea;
			}
			else
			{
				area = Qt::LeftToolBarArea;
			}
		}

		qobject_cast<QBoxLayout*>(layout())->setDirection(value.toBool() ? QBoxLayout::RightToLeft : QBoxLayout::LeftToRight);

		QToolBar *toolbar(findChild<QToolBar*>());

		if (toolbar)
		{
			toolbar->setLayoutDirection((area == Qt::LeftToolBarArea) ? Qt::LeftToRight : Qt::RightToLeft);

			QList<QWidget*> widgets(toolbar->findChildren<QWidget*>());

			for (int i = 0; i < widgets.count(); ++i)
			{
				widgets[i]->setLayoutDirection(QGuiApplication::isLeftToRight() ? Qt::LeftToRight : Qt::RightToLeft);
			}
		}

		ActionsManager::getAction(ActionsManager::OpenPanelAction, this)->setIcon(ThemesManager::getIcon((area == Qt::LeftToolBarArea) ? QLatin1String("arrow-right") : QLatin1String("arrow-left")));
	}
}

void SidebarWidget::scheduleSizeSave()
{
	if (isVisible() && !SettingsManager::getValue(SettingsManager::Sidebar_CurrentPanelOption).toString().isEmpty())
	{
		if (m_resizeTimer > 0)
		{
			killTimer(m_resizeTimer);
		}

		m_resizeTimer = startTimer(500);
	}
}

void SidebarWidget::addWebPanel()
{
	MainWindow *mainWindow(MainWindow::findMainWindow(this));
	QString url;

	if (mainWindow)
	{
		url = mainWindow->getWindowsManager()->getUrl().toString(QUrl::RemovePassword);
	}

	url = QInputDialog::getText(this, tr("Add web panel"), tr("Input address of web page to be shown in panel:"), QLineEdit::Normal, url);

	if (!url.isEmpty())
	{
		QStringList panels(SettingsManager::getValue(SettingsManager::Sidebar_PanelsOption).toStringList());
		panels.append(QLatin1String("web:") + url);

		SettingsManager::setValue(SettingsManager::Sidebar_PanelsOption, panels);
	}
}

void SidebarWidget::choosePanel(bool checked)
{
	QAction *action(qobject_cast<QAction*>(sender()));

	if (!action)
	{
		return;
	}

	QStringList chosenPanels(SettingsManager::getValue(SettingsManager::Sidebar_PanelsOption).toStringList());

	if (checked)
	{
		chosenPanels.append(action->data().toString());
	}
	else
	{
		chosenPanels.removeAll(action->data().toString());
	}

	SettingsManager::setValue(SettingsManager::Sidebar_PanelsOption, chosenPanels);
}

void SidebarWidget::selectPanel()
{
	QAction *action(qobject_cast<QAction*>(sender()));

	if (action)
	{
		selectPanel((action->data().toString() == m_currentPanel) ? QString() : action->data().toString());
	}
}

void SidebarWidget::selectPanel(const QString &identifier)
{
	if (!identifier.isEmpty() && identifier == m_currentPanel)
	{
		return;
	}

	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));
	QWidget *widget((m_panels.contains(identifier) && m_panels[identifier]) ? m_panels[identifier] : WidgetFactory::createSidebarPanel(identifier));

	if (widget && mainWindow)
	{
		connect(widget, SIGNAL(requestedOpenUrl(QUrl,WindowsManager::OpenHints)), mainWindow->getWindowsManager(), SLOT(open(QUrl,WindowsManager::OpenHints)));
		connect(widget, SIGNAL(requestedSearch(QString,QString,WindowsManager::OpenHints)), mainWindow->getWindowsManager(), SLOT(search(QString,QString,WindowsManager::OpenHints)));
		connect(widget, SIGNAL(requestedAddBookmark(QUrl,QString,QString)), mainWindow->getWindowsManager(), SIGNAL(requestedAddBookmark(QUrl,QString,QString)));
		connect(widget, SIGNAL(requestedNewWindow(ContentsWidget*,WindowsManager::OpenHints)), mainWindow->getWindowsManager(), SLOT(openWindow(ContentsWidget*,WindowsManager::OpenHints)));
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

	if (widget)
	{
		m_ui->panelLayout->addWidget(widget);

		widget->show();

		m_panels[identifier] = widget;

		if (m_buttons.contains(identifier) && m_buttons[identifier])
		{
			m_buttons[identifier]->setChecked(true);
		}

		setMinimumWidth(0);
		setMaximumWidth(QWIDGETSIZE_MAX);
	}
	else
	{
		setFixedWidth(m_ui->buttonsLayout->sizeHint().width());
	}

	m_ui->containerWidget->setVisible(widget != nullptr);

	m_currentPanel = identifier;

	SettingsManager::setValue(SettingsManager::Sidebar_CurrentPanelOption, identifier);
}

QWidget* SidebarWidget::getCurrentPanel()
{
	return m_panels.value(m_currentPanel);
}

QString SidebarWidget::getPanelTitle(const QString &identifier)
{
	if (identifier.startsWith(QLatin1String("web:")))
	{
		return identifier.mid(4);
	}

	if (AddonsManager::getSpecialPages().contains(identifier))
	{
		return AddonsManager::getSpecialPage(identifier).getTitle();
	}

	return identifier;
}

QSize SidebarWidget::sizeHint() const
{
	if (SettingsManager::getValue(SettingsManager::Sidebar_CurrentPanelOption).toString().isEmpty())
	{
		return m_ui->buttonsLayout->sizeHint();
	}

	return QSize(SettingsManager::getValue(SettingsManager::Sidebar_WidthOption).toInt(), m_ui->buttonsLayout->sizeHint().height());
}

}
